
#include "EventLoopThread.h"
#include "EventLoop.h"

#include <cassert>
#include <functional>

using namespace hxmmxh;

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb,const std::string &name)
    : loop_(NULL),
      exiting_(false),
      thread_(std::bind(&EventLoopThread::threadFunc, this), name),
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
  assert(!thread_.started());
  thread_.start();

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
  loop.loop();//这里会无限循环直到quit=true
  //EventLoopThread析构时会让loop quit
  std::lock_guard<std::mutex> lock(mutex_);
  loop_ = NULL;
}