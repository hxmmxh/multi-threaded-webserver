#ifndef HXMMXH_EVENTLOOPTHREADPOOL_H
#define HXMMXH_EVENTLOOPTHREADPOOL_H

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace hxmmxh
{
class EventLoop;
class EventLoopThread;

class EventLoopThreadPool
{
public:
    typedef std::function<void(EventLoop *)> ThreadInitCallback;

    EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg="Default EventLoopThreadPool");
    ~EventLoopThreadPool();
    void setThreadNum(int numThreads) { numThreads_ = numThreads; }
    void start(const ThreadInitCallback &cb = ThreadInitCallback());

    EventLoop *getNextLoop();
    std::vector<EventLoop *> getAllLoops();

    bool started() const
    {
        return started_;
    }

    const std::string &name() const
    {
        return name_;
    }

private:
    EventLoop *baseLoop_;
    std::string name_;
    bool started_;
    int numThreads_; //线程池的大小
    int next_;       //下一个被取出的loop序号
    std::vector<std::unique_ptr<EventLoopThread>> threads_;
    std::vector<EventLoop *> loops_;
};

} // namespace hxmmxh

#endif