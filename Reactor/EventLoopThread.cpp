
#include "EventLoopThread.h"
#include "EventLoop.h"
#include "Logging.h"

#include <cassert>
#include <functional>
#include <unistd.h>

using namespace hxmmxh;

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb, const std::string &name)
    : callback_(cb),
      exiting_(false),
      thread_(std::bind(&EventLoopThread::threadFunc, this), name),
      mutex_(),
      loop_(NULL),
      cond_()
{
  //刚初始化的EventLoopThread对象不拥有EventLoop对象
  //只有在调用了startLoop后才在stack上定义一个EventLoop对象，然后将其地址赋予给loop_
}

EventLoopThread::~EventLoopThread()
{
  exiting_ = true;
  if (loop_)
  {
    usleep(10);
    loop_->quit();
    LOG_TRACE << "dtor of EventLoopThread, wait for join";
    //有几次测试时发现析构函数阻塞在等待线程结束上
    //运行的是threadFunc，猜测是阻塞在loop上，即上一步的quit()没有执行？
    //分析可能是在thread_进入loop前，quit就已经调用，导致之后没有操作能终止loop
    //快速地调用析构函数会出现问题
    //这里采取一个治标不治本的方法，在析构函数中usleep(10)再调用quit()
    thread_.join();
  }
}

EventLoop *EventLoopThread::startLoop()
{
  assert(!thread_.started());
  thread_.start();

  EventLoop *loop = NULL;
  // 等待线程创建好了EventLoop后再返回
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
  loop.loop(); //这里会无限循环直到quit=true
  //EventLoopThread析构时会让loop quit
  std::lock_guard<std::mutex> lock(mutex_);
  loop_ = NULL;
}