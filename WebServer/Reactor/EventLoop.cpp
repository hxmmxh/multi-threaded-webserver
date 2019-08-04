#include "EventLoop.h"
#include "../Log/Logging.h"
#include "Poller.h"
#include "TimerQueue.h"
#include "Channel.h"
#include "../Sockets/SocketsOps.h"

#include <algorithm>
#include <sys/eventfd.h>
#include <cstdlib>

using namespace hxmmxh;

//作用相当于C语言中全局static声明
//未命名的名字空间的定义局部于一个特定的文件，不能跨越多个文本文件  。
//不应在C ++中创建 static 全局变量。应该使用未命名的命名空间。
/*
在全局变量前加上关键字static，全局变量就定义成一个全局静态变量. 
静态存储区，在整个程序运行期间一直存在。 
初始化：未经初始化的全局静态变量会被自动初始化为0（自动对象的值是任意的，除非他被显式初始化）； 
作用域：全局静态变量在声明他的文件之外是不可见的，准确地说是从定义之处开始，到文件结尾。 
 */
namespace
{
__thread EventLoop *t_loopInThisThread = 0;
const int kPollTimeMs = 10000;
//函数的定义和声明在默认情况下都是extern的，但静态函数只是在声明他的文件当中可见，不能被其他文件所用。
int createEventfd()
{
    //计数器初始化为0，非阻塞，exec后关闭
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
    {
        LOG_SYSERR << "Failed in eventfd";
        abort();
    }
    return evtfd;
}

//pragma GCC diagnostic [error|warning|ignored] "-W<警告选项>"
//关闭"-old-style-cast"警告
//-old-style-cast ，程序使用C风格的类型转换时给出警告
#pragma GCC diagnostic ignoed "-Wold-style-cast"
class IgnoreSigPipe
{
public:
    IgnoreSigPipe()
    {
        ::signal(SIGPIPE, SIG_IGN);
        LOG_TRACE << "Ignore SIGPIPE";
    }
};
//开启警告，升级为错误
#pragma GCC diagnostic error "-Wold-style-cast"

IgnoreSigPipe initObj;
} // namespace

EventLoop::EventLoop()
    : looping_(false),
      quit_(false),
      eventHandling_(false),
      callingPendingFunctors_(false),
      iteration_(0), //
      threadId_(CurrentThread::tid()),
      poller_(Poller::newDefaultPoller(this)),
      timerQueue_(new TimerQueue(this)),
      wakeupFd_(createEventfd()),
      wakeupChannel_(new Channel(this, wakeupFd_)),
      currentActiveChannel_(NULL)
{
    LOG_DEBUG << "EventLoop created " << this << " in thread " << threadId_;
    if (t_loopInThisThread)
    {
        LOG_FATAL << "Another EventLoop " << t_loopInThisThread
                  << " exists in this thread " << threadId_;
    }
    else
    {
        t_loopInThisThread = this;
    }
    //开始监控wakeupChannel
    //eventfd_可读时调用handleRead()
    wakeupChannel_->setReadCallback(
        std::bind(&EventLoop::handleRead, this));
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
    LOG_DEBUG << "EventLoop " << this << " of thread " << threadId_
              << " destructs in thread " << CurrentThread::tid();
    //停止监控wakeupChannel，并关闭对应的eventfd
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = NULL;
}

EventLoop *EventLoop::getEventLoopOfCurrentThread()
{
    return t_loopInThisThread;
}

void EventLoop::loop()
{
    assert(!looping_);
    assertInLoopThread();
    looping_ = true;
    quit_ = false;
    LOG_TRACE << "EventLoop " << this << " start looping";

    while (!quit_)
    {
        activeChannels_.clear();
        //调用Poller::poll，获得活跃事件
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        ++iteration_;
        if (Logger::logLevel() <= Logger::TRACE)
        {
            printActiveChannels();
        }
        eventHandling_ = true;
        //处理活跃事件，交给Channel去完成
        for (Channel *channel : activeChannels_)
        {
            currentActiveChannel_ = channel;
            currentActiveChannel_->handleEvent(pollReturnTime_);
        }
        currentActiveChannel_ = NULL;
        eventHandling_ = false;
        //每个循环结尾后处理任务队列
        doPendingFunctors();
    }

    LOG_TRACE << "EventLoop " << this << " stop looping";
    looping_ = false;
}

void EventLoop::abortNotInLoopThread()
{
    LOG_FATAL << "EventLoop::abortNotInLoopThread - EventLoop " << this
              << " was created in threadId_ = " << threadId_
              << ", current thread id = " << CurrentThread::tid();
}

void EventLoop::quit()
{
    quit_ = true;
    //如果是在其他线程中，IO线程可能阻塞在poll中，在退出之前需要唤醒
    if (!isInLoopThread())
        wakeup();
}

void EventLoop::runInLoop(const Functor &cb)
{
    if (isInLoopThread())
    {
        cb();
    }
    else
    {
        queueInLoop(cb);
    }
}

//有个地方会不通过runInLoop直接调用queueInLoop
void EventLoop::queueInLoop(const Functor &cb)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pendingFunctors_.push_back(cb);
    }
    if (!isInLoopThread() || callingPendingFunctor_)
    {
        wakeup();
    }
}

size_t EventLoop::queueSize() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return pendingFunctors_.size();
}

TimerId EventLoop::runAt(const Timestamp &time, const TimerCallback &cb)
{
    return timerQueue_->addTimer(cb, time, 0.0);
}

TimerId EventLoop::runAfter(double delay, const TimerCallback &cb)
{
    Timestamp time(addTime(Timestamp::now(), delay));
    return runAt(time, cb);
}

TimerId EventLoop::runEvery(double interval, const TimerCallback &cb)

{
    Timestamp time(addTime(Timestamp::now(), interval));
    return timerQueue_->addTimer(cb, time, interval);
}

void EventLoop::cancel(TimerId timerId)
{
    return timerQueue_->cancel(timerId);
}

//Channel由Poller管理
void EventLoop::updateChannel(Channel *channel)
{
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel)
{
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    //不能移除正在执行的活跃事件，除非是自身要求移除自己
    if (eventHandling_)
    {
        assert(currentActiveChannel_ == channel ||
               std::find(activeChannels_.begin(), activeChannels_.end(), channel) == activeChannels_.end());
    }
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel *channel)
{
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    return poller_->hasChannel(channel);
}

void EventLoop::wakeup()
{
    uint64_t one = 1;
    //eventfd内部包含了由内核维护的无符号64位整数计数器 count
    //write就相当于累加 eventfd_中的count值，这里是使其加1
    ssize_t n = sockets::write(wakeupFd_, &one, sizeof(one));
    if (n != sizeof(one))
    {
        LOG_ERROR << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
    }
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
    //读取 count 值后置 0
    ssize_t n = sockets::read(wakeupFd_, &one, sizeof(one));
    if (n != sizeof (one))
    {
        LOG_ERROR << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
    }
}


void EventLoop::printActiveChannels() const
{
    for (const Channel *channel : activeChannels_)
    {
        LOG_TRACE << "{" << channel->reventsToString() << "} ";
    }
}

void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctor_ = true;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }
    for (size_t i = 0; i < functors.size(); ++i)
        functors[i]();
    callingPendingFunctor_ = false;
}

