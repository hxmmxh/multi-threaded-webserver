#ifndef HXMMXH_EVENTLOOPTHREAD_H
#define HXMMXH_EVENTLOOPTHREAD_H

#include "../../Thread/Thread.h"

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
    EventLoop *startLoop();

private:
    void threadFunc();

    ThreadInitCallback callback_;
    bool exiting_;
    Thread thread_;

    std::mutex mutex_;
    EventLoop *loop_;
    std::condition_variable cond_;
};

} // namespace hxmmxh
#endif