
#include "EventLoopThread.h"
#include "EventLoop.h"

#include <functional>
EventLoopThread::EventLoopThread(const ThreadInitCallback &cb)
    : loop_(NULL),
      exiting_(false),
      thread_(),
      mutex_(),
      cond_(),
      callback_(cb)
{
}

EventLoopThread::~EventLoopThread()
{
  exiting_ = true;
  loop_->quit();
  thread_.join();
}

EventLoop *EventLoopThread::startLoop()
{
  thread_ = std::thread(std::bind(&EventLoopThread::threadFunc, this));
  EventLoop *loop = NULL;
  {
    std::unique_lock<std::mutex> lock(mutex_);
    while (loop_ == NULL)
    {
      cond_.wait(lock);
    }
    loop = loop_;
  }
  return loop;
}

void EventLoopThread::threadFunc()
{
  EventLoop loop;
  if (callback_)
  {
    callback_(&loop);
  }
  {
    std::lock_guard<std::mutex> lock(mutex_);
    loop_ = &loop;
    cond_.notify_one();
  }
  loop.loop();
  std::lock_guard<std::mutex> lock(mutex_);
  loop_ = NULL;
}