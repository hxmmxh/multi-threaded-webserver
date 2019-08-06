#include "TimerQueue.h"

#include "../../Log/Logging.h"
#include "../EventLoop.h" //TimerQueue.h中有Eventloop的提前声明，不包含头文件

#include <cstring>
#include <sys/timerfd.h>
#include <unistd.h>

//修改下内置timerfd函数，使其更适用我们的定时器
namespace hxmmxh
{

int createTimerfd()
{
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC,
                                   TFD_NONBLOCK | TFD_CLOEXEC);
    if (timerfd < 0)
    {
        //写完log后会直接abort()
        LOG_SYSFATAL << "Failed in timerfd_create";
    }
    return timerfd;
}
/*
struct timespec 
{
    time_t tv_sec;    //秒
    long tv_nsec;    //纳秒
}; 
 */
//计算when到Now之间的相对时间
struct timespec howMuchTimeFromNow(Timestamp when)
{
    int64_t microseconds = when.microSecondsSinceEpoch() - Timestamp::now().microSecondsSinceEpoch();
    if (microseconds < 100)
    {
        microseconds = 100;
    }
    struct timespec ts;
    ts.tv_sec = static_cast<time_t>(
        microseconds / Timestamp::kMicroSecondsPerSecond);
    ts.tv_nsec = static_cast<long>(
        (microseconds % Timestamp::kMicroSecondsPerSecond) * 1000);
    return ts;
}

//读取并记录timerfd的超时次数
void readTimerfd(int timerfd, Timestamp now)
{
    uint64_t howmany;
    //read函数可以读timerfd，读的内容为uint_64，表示超时次数。
    ssize_t n = ::read(timerfd, &howmany, sizeof(howmany));
    LOG_TRACE << "TimerQueue::handleRead() " << howmany << " at " << now.toString();
    if (n != sizeof(howmany))
    {
        LOG_ERROR << "TimerQueue::handleRead() reads " << n << " bytes instead of 8";
    }
}
/*
struct itimerspec 
{
    struct timespec it_interval;   //间隔时间
    struct timespec it_value;      //第一次到期时间
};
 */
//到expiration这一时刻到期
void resetTimerfd(int timerfd, Timestamp expiration)
{
    struct itimerspec newValue;
    struct itimerspec oldValue;
    memset(&newValue, 0, sizeof(newValue));
    memset(&oldValue, 0, sizeof(oldValue));
    newValue.it_value = howMuchTimeFromNow(expiration);
    //第二个参数为0代表相对时间
    //成功返回0, 失败返回-1以及设置errno
    int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
    if (ret)
    {
        LOG_SYSERR << "timerfd_settime()";
    }
}

} // namespace hxmmxh

using namespace hxmmxh;

//Timer类的静态对象，初始化为0
std::atmoic<int64_t> Timer::s_numCreated_;

void Timer::restart(Timestamp now)
{
    if (repeat_)
    {
        //addTime定义在Timestamp.h，返回now+interval_的时间
        //下一次到期时间为现在的时间加上间隔
        expiration_ = addTime(now, interval_);
    }
    else
    {
        expiration_ = Timestamp::invalid();
    }
}

TimerQueue::TimerQueue(EventLoop *loop)
    : loop_(loop),
      timerfd_(createTimerfd()),
      timerfdChannel_(loop, timerfd_), //监控timerfd_的Channel
      timers_(),
      callingExpiredTimers_(false)
{
    //在Channel中注册timerfd可读回调函数，并开始监控可读事件
    //timerfd_可读则说明有定时器到期
    timerfdChannel_.setReadCallback(
        std::bind(&TimerQueue::handleRead, this));
    timerfdChannel_.enableReading();
}

TimerQueue::~TimerQueue()
{
    //先关闭监控这个timerfd的Channel
    timerfdChannel_.disableAll();
    timerfdChannel_.remove();
    ::close(timerfd_);
    //注意回收资源，delete Timers*
    for (const Entry &timer : timers_)
    {
        delete timer.second;
    }
}

//添加一个定时器，并返回它的ID
TimerId TimerQueue::addTimer(TimerCallback cb,
                             Timestamp when,
                             double interval)
{
    Timer *timer = new Timer(std::move(cb), when, interval);
    //注册任务，何时运行由EventLoop决定
    loop_->runInLoop(
        std::bind(&TimerQueue::addTimerInLoop, this, timer));
    return TimerId(timer, timer->sequence());
}

void TimerQueue::addTimerInLoop(Timer *timer)
{
    //先保证是在当前线程
    //因为每个EventLoop只能在创建它的线程中被调用
    loop_->assertInLoopThread();
    bool earliestChanged = insert(timer);
    //如果最早到期的定时器发生了改变
    //则在timerfd中要修改到期时间
    if (earliestChanged)
    {
        resetTimerfd(timerfd_, timer->expiration());
    }
}

void TimerQueue::cancel(TimerId timerId)
{
    loop_->runInLoop(
        std::bind(&TimerQueue::cancelInLoop, this, timerId));
}

void TimerQueue::cancelInLoop(TimerId timerId)
{
    loop_->assertInLoopThread();
    //timers_和activeTimers_里保存的应该是相同的数据
    assert(timers_.size() == activeTimers_.size());
    //通过TimerId找到要注销的定时器
    ActiveTimer timer(timerId.timer_, timerId.sequence_);
    ActiveTimerSet::iterator it = activeTimers_.find(timer);
    if (it != activeTimers_.end())
    {
        //先在timers_中删除这个定时器
        size_t n = timers_.erase(Entry(it->first->expiration(), it->first));
        assert(n == 1);
        (void)n;
        delete it->first;
        //在activeTimers_中也同样删除它
        activeTimers_.erase(it);
    }
    //自注销，即在定时器的可读回调函数中注销当前定时器
    else if (callingExpiredTimers_)
    {
        //cancelingTimers_中的定时器在随后的reset函数中不会被添加回timers_
        cancelingTimers_.insert(timer);
    }
    assert(timers_.size() == activeTimers_.size());
}

//可读事件的回调函数
void TimerQueue::handleRead()
{
    loop_->assertInLoopThread();
    Timestamp now(Timestamp::now());
    readTimerfd(timerfd_, now);
    //得到已到期的定时器
    std::vector<Entry> expired = getExpired(now);
    //表明此时正在执行定时器回调
    //回调函数可能会注销自身
    callingExpiredTimers_ = true;
    cancelingTimers_.clear();
    for (const Entry &it : expired)
    {
        //执行定时器到期时调用的函数
        it.second->run();
    }
    callingExpiredTimers_ = false;

    reset(expired, now);
}

//从timers_和activetimers_中移除已到期的Timer,并通过vector返回
std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now)
{
    assert(timers_.size() == activeTimers_.size());
    std::vector<Entry> expired;
    //UINTPTR表示指针的最大值
    Entry sentry(now, reinterpret_cast<Timer *>(UINTPTR_MAX));
    //返回第一个未到期的Timer的迭代器
    //可以插入sentry的第一个位置
    TimerList::iterator end = timers_.lower_bound(sentry);
    //如果找到了，就说明now肯定小于end->first
    assert(end == timers_.end() || now < end->first);
    //相当于调用expired.push_back()，把已到期的Timer加入vector中
    std::copy(timers_.begin(), end, back_inserter(expired));
    //在timers_中删除已到期的Timer
    timers_.erase(timers_.begin(), end);
    //在acticvetimers_中也删除这些已到期的定时器
    for (const Entry &it : expired)
    {
        ActiveTimer timer(it.second, it.second->sequence());
        size_t n = activeTimers_.erase(timer);
        assert(n == 1);
        (void)n;
    }
    //永远保持timers_和activeTimers_的内容是一样的
    assert(timers_.size() == activeTimers_.size());
    return expired;
}

//expired中的定时器有些可能是重复的，找出并重新加入timers_和activetimers_中
//并重新设置timerfd
void TimerQueue::reset(const std::vector<Entry> &expired, Timestamp now)
{
    Timestamp nextExpire;

    for (const Entry &it : expired)
    {
        ActiveTimer timer(it.second, it.second->sequence());
        if (it.second->repeat()                                        //该定时器是重复的
            && cancelingTimers_.find(timer) == cancelingTimers_.end()) //且没有在定时回调函数中注销自己
        {
            //如果是可重复的，且不在重新加入序列
            it.second->restart(now);
            insert(it.second);
        }
        else
        {
            //这些定时器已经被移出timers_和activeTimers_，所以只用简单delete
            delete it.second;
        }
    }
    //找到最早到期的定时器，并修改timerfd
    if (!timers_.empty())
    {
        //set是有序的，第一个元素就是最早到期的定时器
        nextExpire = timers_.begin()->second->expiration();
    }
    if (nextExpire.valid())
    {
        resetTimerfd(timerfd_, nextExpire);
    }
}

//往序列中新插入定时器
bool TimerQueue::insert(Timer *timer)
{
    loop_->assertInLoopThread();
    assert(timers_.size() == activeTimers_.size());
    bool earliestChanged = false;
    Timestamp when = timer->expiration();
    TimerList::iterator it = timers_.begin();
    //如果队列为空或者到期时间是最早的
    if (it == timers_.end() || when < it->first)
    {
        earliestChanged = true;
    }
    {
        //插入返回一个pair,成功的话，second是true
        std::pair<TimerList::iterator, bool> result = timers_.insert(Entry(when, timer));
        assert(result.second);
        (void)result;
    }
    {
        std::pair<ActiveTimerSet::iterator, bool> result = activeTimers_.insert(ActiveTimer(timer, timer->sequence()));
        assert(result.second);
        (void)result;
    }

    assert(timers_.size() == activeTimers_.size());
    return earliestChanged;
}
