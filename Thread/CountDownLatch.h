//hxm,2019-8-4,17:55,finish

#ifndef HXMMXH_THREAD_COUNTDOWNLATCH_H
#define HXMMXH_THREAD_COUNTDOWNLATCH_H

#include <mutex>
#include <condition_variable>

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

#endif

