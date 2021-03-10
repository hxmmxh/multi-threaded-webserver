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
    // 获取下一个线程中的事件循环
    EventLoop *getNextLoop();
    // 获取所有的事件循环
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
    EventLoop *baseLoop_; //主线程中的事件循环，只有在主线程中才能启动线程池
    std::string name_;
    bool started_; // 线程池是否启动
    int numThreads_; //线程池的大小
    int next_;       //下一个被取出的loop序号
    std::vector<std::unique_ptr<EventLoopThread>> threads_; //保存所有的工作线程
    std::vector<EventLoop *> loops_;//保存所有工作线程中的事件循环
};

} // namespace hxmmxh

#endif