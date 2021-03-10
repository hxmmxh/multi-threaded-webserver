#include "ThreadPool.h"

#include <cassert>
#include <cstdio>
#include <utility>  //std::move
#include <unistd.h> //usleep

using namespace hxmmxh;

ThreadPool::ThreadPool(const std::string &nameArg)
    : name_(nameArg),
      maxQueueSize_(0),
      running_(false),
      mutex_(),
      notEmpty_(),
      notFull_()
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
        char id[32];
        snprintf(id, sizeof id, "%d", i + 1);
        // 创建线程，线程里运行的函数是runInThread,线程名是线程池名字+id
        threads_.emplace_back(new Thread(std::bind(&ThreadPool::runInThread, this), name_ + id));
        // 创建完后立刻运行
        threads_[i]->start();
    }
    // 如果没有创建线程池，那么主线程要调用线程开始时的运行函数
    if (numThreads == 0 && threadInitCallback_)
    {
        threadInitCallback_();
    }
}

void ThreadPool::stop()
{
    // 为了让所有的工作线程都已经开始运行
    usleep(10);
    {
        std::lock_guard<std::mutex> lock(mutex_);
        running_ = false;
        //take()会阻塞在notEmpty中，条件为任务队列为空且线程池正在运行
        notEmpty_.notify_all();
    }
    // 等待所有工作线程结束
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

//往任务队列中添加任务task，随后task在函数take中取出，在函数runInThread中被执行
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
    //线程池不运行时，即使任务队列为空，也不要再等待
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
    //只有在已取得锁的情况下才会调用isFull
    //可以加个条件判度语句，判度此时mutex已被锁
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
        //线程停止时会取出空的task
        if (task)
        {
            task();
        }
    }
}
