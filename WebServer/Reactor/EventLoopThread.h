#ifndef HXMMXH_EVENTLOOPTHREAD_H
#define HXMMXH_EVENTLOOPTHREAD_H

#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>

namespace hxmmxh
{
class EventLoop;

class EventLoopThread
{
public:
    typedef std::function<void(EventLoop *)> ThreadInitCallback;

    EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback());
    ~EventLoopThread();
    EventLoop *startLoop();

private:
    void threadFunc();

    ThreadInitCallback callback_;
    bool exiting_;
    std::thread thread_;

    std::mutex mutex_;
    EventLoop *loop_;
    std::condition_variable cond_;
};

} // namespace hxmmxh
#endif