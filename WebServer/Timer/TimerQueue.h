#ifndef HXMMXH_TIMEQUEUE_H
#define HXMMXH_TIMEQUEUE_H

#include "../../Time/Timestamp.h"
#include "../Callbacks.h"
#include "../Reactor/Channel.h" //TimeQueue类中有Channel对象

#include <atomic>
#include <cinttypes>
#include <set>
#include <vector>

namespace hxmmxh
{
//定时器
class Timer
{
public:
    Timer(const TimerCallback &cb, Timestamp when, double interval)
        : callback_(cb),
          expiration_(when),
          interval_(interval),
          repeat_(interval > 0.0),
          sequence_(++s_numCreated_) {}

    void run() const { callback_(); }
    Timestamp expiration() const { return expiration_; }
    bool repeat() const { return repeat_; }
    int64_t sequence() const { return sequence_; }
    void restart(Timestamp now); //重新启动这个定时器

    static int64_t numCreated() { return s_numCreated_; }

private:
    //typedef std::function<void()> TimerCallback;
    const TimerCallback callback_; //定时器到期时调用的函数
    Timestamp expiration_;         //第一次到期时间，单位为微秒，是绝对时间
    const double interval_;        //间隔时间，单位为秒
    const bool repeat_;            //是否重复，单位为秒
    const int64_t sequence_;       //序列号

    static std::atomic<int64_t> s_numCreated_; //原子计数器，用于计算序列号
};

//表示一个定时器的ID
//不负责timer_的生命期，其中保存的timer_可能失效
class TimerId
{
public:
    TimerId() : timer_(NULL), sequence_(0) {}
    TimerId(Timer *timer, int64_t seq) : timer_(timer), sequence_(seq) {}
    friend class TimerQueue;

private:
    Timer *timer_;
    int64_t sequence_; //timer_对象的序列号
};

class EventLoop; //后面用到EventLoop指针，只需提前声明就好了

//定时器队列
class TimerQueue
{
public:
    TimerQueue(EventLoop *);
    ~TimerQueue();
    //添加定时器
    TimerId addTimer(TimerCallback cb, Timestamp when, double interval);
    //注销定时器
    void cancel(TimerId timerId);

private:
    //需要高效地组织尚未到期的timer，找到已经到期的timer
    //查找，添加，删除，综合考虑选用set
    //set是有序的，能按照Timer的到期时间排序
    typedef std::pair<Timestamp, Timer *> Entry;
    typedef std::set<Entry> TimerList;
    //组织还未到期的Timer，按对象地址排序
    //和TimerList保存着相同的数据，只是排序不同
    typedef std::pair<Timer *, int64_t> ActiveTimer;
    typedef std::set<ActiveTimer> ActiveTimerSet;

    void addTimerInLoop(Timer *timer);
    void cancelInLoop(TimerId timerId);

    //定时器到期，即timerfd可读时应调用的函数
    void handleRead();
    //从timers_和activetimers_中移除已到期的Timer,并通过vector返回
    std::vector<Entry> getExpired(Timestamp now);
    //expired中的定时器有些可能是重复的，找出并重新加入timers_和activetimers_中
    //并重新设置timerfd
    void reset(const std::vector<Entry> &expired, Timestamp now);

    bool insert(Timer *timer);

    EventLoop *loop_;
    const int timerfd_;
    Channel timerfdChannel_; //负责这个timerfd的IO事件的Channel
    TimerList timers_;

    // 按Timer*地址排序的集合，保存着目前有效的Timer指针
    ActiveTimerSet activeTimers_;
    std::atomic<bool> callingExpiredTimers_;
    ActiveTimerSet cancelingTimers_;
};
} // namespace hxmmxh
#endif