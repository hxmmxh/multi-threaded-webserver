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

    void setMaxQueueSize(int maxSize) { maxQueueSize_ = maxSize; }
    void setThreadInitCallback(const Task &cb)
    {
        threadInitCallback_ = cb;
    }

    void start(int numThreads);
    void stop();

    const std::string &name() const
    {
        return name_;
    }

    size_t queueSize() const;
    void run(Task f);

private:
    void runInThread();
    Task take();
    std::string name_;
    Task threadInitCallback_;
    std::vector<std::unique_ptr<Thread>> threads_; //保存的线程
    size_t maxQueueSize_;
    bool running_;

    mutable std::mutex mutex_;
    bool isFull() const;
    std::condition_variable notEmpty_;
    std::condition_variable notFull_;
    std::deque<Task> queue_; //任务队列
};

} // namespace hxmmxh

#endif