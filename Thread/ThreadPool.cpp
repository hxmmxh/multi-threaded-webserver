#include "ThreadPool.h"

#include <cassert>
#include <cstdio>
#include <utility> //std::move
#include <stdexcept>
#include <iostream>

using namespace hxmmxh;

ThreadPool::ThreadPool(const std::string &nameArg)
    : mutex_(),
      notEmpty_(),
      notFull_(),
      name_(nameArg),
      maxQueueSize_(0),
      running_(false)
{
}

ThreadPool::~ThreadPool()
{
    if (running_)
    {
        stop();
    }
}

void ThreadPool::start(int numThreads)
{
    assert(threads_.empty());
    running_ = true;
    threads_.reserve(numThreads);
    for (int i = 0; i < numThreads; ++i)
    {
        threads_.emplace_back(new std::thread(std::bind(&ThreadPool::runInThread, this)));
    }
    if (numThreads == 0 && threadInitCallback_)
    {
        threadInitCallback_();
    }
}

void ThreadPool::stop()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        running_ = false;
        notEmpty_.notify_all();
    }
    for (auto &thr : threads_)
    {
        thr->join();
    }
}

size_t ThreadPool::queueSize() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

void ThreadPool::run(Task task)
{
    //线程池为空，直接运行
    if (threads_.empty())
    {
        task();
    }
    else
    {
        std::unique_lock<std::mutex> lock(mutex_);
        //等待任务队列不满
        while (isFull())
        {
            notFull_.wait(lock);
        }
        assert(!isFull());
        //将任务加入队列
        queue_.push_back(std::move(task));
        //队列不为空发送信号
        notEmpty_.notify_one();
    }
}

//取出任务队列的第一个任务
ThreadPool::Task ThreadPool::take()
{
    std::unique_lock<std::mutex> lock(mutex_);
    while (queue_.empty() && running_)
        notEmpty_.wait(lock);
    Task task;
    if (!queue_.empty())
    {
        task = queue_.front();
        queue_.pop_front();
        if (maxQueueSize_ > 0)
        {
            notFull_.notify_one();
        }
    }
    return task;
}

bool ThreadPool::isFull() const
{
    return maxQueueSize_ > 0 && queue_.size() >= maxQueueSize_;
}

void ThreadPool::runInThread()
{
    if (threadInitCallback_)
    {
        threadInitCallback_();
    }
    while (running_)
    {
        Task task(take());
        if (task)
        {
            task();
        }
    }
}
