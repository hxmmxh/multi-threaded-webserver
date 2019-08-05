#ifndef HXMMXH_EVENTLOOP_H
#define HXMMXH_EVENTLOOP_H

#include "../../Time/Timestamp.h"
#include "../Callbacks.h"
#include "../../Thread/CurrentThread.h"
#include "../Timer/TimerQueue.h"

#include <atomic>
#include <functional>
#include <vector>

#include <mutex>
#include <condition_variable>

namespace hxmmxh
{
//提前声明，不包含头文件
class Poller;
class Channel;
class TimerQueue;

//每个线程只能有一个EventLoop对象，one loop per thread
//创建了EventLoop对象的线程是IO线程，其主要功能是运行事件循环loop()
class EventLoop
{
public:
    typedef std::function<void()> Functor;
    EventLoop();
    ~EventLoop();

    Timestamp pollReturnTime() const { return pollReturnTime_; }

    void loop();
    void quit();

    void assertInLoopThread()
    {
        if (!isInLoopThread())
        {
            abortNotInLoopThread();
        }
    }
    bool isInLoopThread() const
    {
        return threadId_ == CurrentThread::tid();
    }

    static EventLoop *getEventLoopOfCurrentThread();

    void updateChannel(Channel *channel);

    TimerId runAt(const Timestamp &time, const TimerCallback &cb);
    TimerId runAfter(double delay, const TimerCallback &cb);
    TimerId runEvery(double interval, const TimerCallback &cb);
    void cancel(TimerId timerId);

    void runInLoop(const Functor &cb);
    void queueInLoop(const Functor &cb);
    size_t queueSize() const;

    void wakeup();
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);
    bool hasChannel(Channel *channel);

    bool eventHandling() const { return eventHandling_; }
    Timestamp pollReturnTime() const { return pollReturnTime_; }
    int64_t iteration() const { return iteration_; }

private:
    void abortNotInLoopThread();
    void handleRead(); //eventfd可读时调用的函数
    void doPendingFunctors();

    void printActiveChannels() const; // DEBUG

    typedef std::vector<Channel *> ChannelList;

    std::atomic_bool looping_;
    std::atomic_bool quit_;
    std::atomic_bool eventHandling_;         //是否正在处理活跃事件，loop()中修改
    std::atomic_bool callingPendingFunctor_; //是否在处理任务队列
    int64_t iteration_;
    const pid_t threadId_;
    Timestamp pollReturnTime_; //loop()中poll函数返回的时间
    std::unique_ptr<Poller> poller_;
    std::unique_ptr<TimerQueue> timerQueue_;
    int wakeupFd_;
    std::unique_ptr<Channel> wakeupChannel_;
    //boost::any context_ 可用于保存任何类型的对象
    ChannelList activeChannels_;    //收到信号的Channel，loop()中修改
    Channel *currentActiveChannel_; //正在处理的活跃事件，loop()中修改

    mutable std::mutex mutex_;
    std::vector<Functor> pendingFunctors_; //任务队列
};
} // namespace hxmmxh
#endif