#ifndef HXMMXH_THREAD_THREADPOOL_H
#define HXMMXH_THREAD_THREADPOOL_H

#include "Thread.h"

#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

namespace hxmmxh
{

class ThreadPool
{
public:
    typedef std::function<void()> Task;

    explicit ThreadPool(const std::string &nameArg = std::string("ThreadPool"));
    ~ThreadPool();

    // 设置任务队列的最大数
    void setMaxQueueSize(int maxSize) { maxQueueSize_ = maxSize; }
    // 设置每次线程启动时，要运行的函数
    void setThreadInitCallback(const Task &cb)
    {
        threadInitCallback_ = cb;
    }

    // 开始运行线程池
    void start(int numThreads);
    // 停止线程池
    void stop();

    const std::string &name() const
    {
        return name_;
    }

    size_t queueSize() const;
    // 往线程池中添加任务，会被随机一个空闲的线程完成
    void run(Task f);

private:
    void runInThread();
    Task take();
    std::string name_;
    Task threadInitCallback_;
    
    // 用智能指针是为了能自动调用detach释放线程资源
    std::vector<std::unique_ptr<Thread>> threads_; //保存的线程
    size_t maxQueueSize_;

    bool running_;
    bool isFull() const;

    // 加任务，取任务，需要加锁
    mutable std::mutex mutex_;
    std::condition_variable notEmpty_;
    std::condition_variable notFull_;
    std::deque<Task> queue_; //任务队列
};

} // namespace hxmmxh

#endif