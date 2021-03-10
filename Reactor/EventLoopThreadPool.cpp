#include "EventLoop.h"
#include "EventLoopThread.h"
#include "EventLoopThreadPool.h"

#include <cassert>

using namespace hxmmxh;

EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg)
    : baseLoop_(baseLoop),
      name_(nameArg),
      started_(false),
      numThreads_(0),
      next_(0)
{
}

EventLoopThreadPool::~EventLoopThreadPool()
{
  
}

void EventLoopThreadPool::start(const ThreadInitCallback &cb)
{
  assert(!started_);
  //需要在TcpServer所在的线程启动IO线程池
  baseLoop_->assertInLoopThread();

  started_ = true;

  for (int i = 0; i < numThreads_; ++i)
  {
    char buf[name_.size() + 32];
    snprintf(buf, sizeof buf, "%s%d", name_.c_str(), i);
    
    EventLoopThread *t = new EventLoopThread(cb, buf);
    //EventLoopThread对象还属于这个线程
    threads_.push_back(std::unique_ptr<EventLoopThread>(t));
    //但是EventLoopThread::startLoop会新建一个线程运行EventLoop
    loops_.push_back(t->startLoop());
  }
  //如果单线程，则直接运行
  if (numThreads_ == 0 && cb)
  {
    cb(baseLoop_);
  }
}

//采用round-robin算法来获取下一个EventLoop
//轮询调度，每次把来自用户的请求轮流分配给pool
EventLoop *EventLoopThreadPool::getNextLoop()
{
  baseLoop_->assertInLoopThread();
  assert(started_);
  EventLoop *loop = baseLoop_;
  if (!loops_.empty())
  {
    loop = loops_[next_];
    next_ = (next_ + 1) % numThreads_;
  }
  return loop;
}


std::vector<EventLoop *> EventLoopThreadPool::getAllLoops()
{
  baseLoop_->assertInLoopThread();
  assert(started_);
  if (loops_.empty())
  {
    return std::vector<EventLoop *>(1, baseLoop_);
  }
  else
  {
    return loops_;
  }
}
