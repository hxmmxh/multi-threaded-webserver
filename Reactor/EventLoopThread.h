#ifndef HXMMXH_EVENTLOOPTHREAD_H
#define HXMMXH_EVENTLOOPTHREAD_H

#include "Thread.h"

#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>

namespace hxmmxh
{
class EventLoop;

class EventLoopThread
{
public:
    typedef std::function<void(EventLoop *)> ThreadInitCallback;

    EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback(), const std::string &name = std::string());
    ~EventLoopThread();
    // 开始事件循环
    EventLoop *startLoop();

private:
    void threadFunc(); // 每个线程要运行的函数
    ThreadInitCallback callback_; // 每个线程开始时调用的函数
    bool exiting_;
    Thread thread_;

    std::mutex mutex_;
    EventLoop *loop_;
    std::condition_variable cond_;
};

} // namespace hxmmxh
#endif