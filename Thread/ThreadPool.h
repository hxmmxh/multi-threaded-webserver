#ifndef HXMMXH_THREADPOOL_H
#define HXMMXH_THREADPOOL_H

#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace hxmmxh
{
class CountDownLatch
{
public:
    explicit CountDownLatch(int count) : count_(count) {}
    void wait()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (count_ > 0)
            cond_.wait(lock);
    }

    void countDown()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        --count_;
        if (count_ == 0)
            cond_.notify_all();
    }
    int getCount()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return count_;
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable cond_;
    int count_;
};

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
    bool isFull() const;
    void runInThread();
    Task take();

    mutable std::mutex mutex_;
    std::condition_variable notEmpty_;
    std::condition_variable notFull_;
    std::string name_;
    Task threadInitCallback_;
    std::vector<std::unique_ptr<std::thread>> threads_; //保存的线程
    std::deque<Task> queue_;                            //任务队列
    size_t maxQueueSize_;
    bool running_;
};

} // namespace hxmmxh

#endif