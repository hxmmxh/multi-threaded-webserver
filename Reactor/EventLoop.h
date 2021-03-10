#ifndef HXMMXH_EVENTLOOP_H
#define HXMMXH_EVENTLOOP_H

#include "Timestamp.h"
#include "Callbacks.h"
#include "CurrentThread.h"
#include "TimerQueue.h"

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

    // 开始事件循环
    void loop();
    // 退出事件循环
    void quit();

    void assertInLoopThread()
    {
        if (!isInLoopThread())
        {
            abortNotInLoopThread();
        }
    }
    // 判断是否是在创建EventLoop的线程中
    bool isInLoopThread() const
    {
        return threadId_ == CurrentThread::tid();
    }
    // 获取当前线程的EventLoop对象指针
    static EventLoop *getEventLoopOfCurrentThread();

    // 注册和注销定时器
    TimerId runAt(const Timestamp &time, const TimerCallback &cb);
    TimerId runAfter(double delay, const TimerCallback &cb);
    TimerId runEvery(double interval, const TimerCallback &cb);
    void cancel(TimerId timerId);

    // 在事件循环中执行函数cb
    void runInLoop(const Functor &cb);
    // 把函数加入到任务队列中
    void queueInLoop(const Functor &cb);
    // 返回任务数
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
    void doPendingFunctors(); // 开始执行任务队列，在每个loop循环结尾调用

    // 打印活动的Channel中发生的事件
    void printActiveChannels() const; // DEBUG

    

    std::atomic_bool looping_; //是否开始事件循环
    std::atomic_bool quit_;// 是否退出事件循环
    std::atomic_bool eventHandling_;         //是否正在处理活跃事件，loop()中修改
    std::atomic_bool callingPendingFunctors_; //是否在处理任务队列

    int64_t iteration_; //记录循环了多少次
    const pid_t threadId_;// 创建这个EventPool的线程id

    Timestamp pollReturnTime_; //loop()中poll函数返回的时间
    std::unique_ptr<Poller> poller_;

    std::unique_ptr<TimerQueue> timerQueue_;

    int wakeupFd_;// 用于事件通知的文件描述符
    std::unique_ptr<Channel> wakeupChannel_;

    //boost::any context_ 可用于保存任何类型的对象
    typedef std::vector<Channel *> ChannelList;
    ChannelList activeChannels_;    //收到信号的Channel，loop()中修改
    Channel *currentActiveChannel_; //正在处理的活跃事件，loop()中修改

    mutable std::mutex mutex_;
    std::vector<Functor> pendingFunctors_; //任务队列
};
} // namespace hxmmxh
#endif