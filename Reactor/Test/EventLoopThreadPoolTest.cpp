#include "EventLoop.h"
#include "EventLoopThread.h"
#include "EventLoopThreadPool.h"
#include "Logging.h"

#include <cassert>
#include <stdio.h>
#include <unistd.h>

using namespace hxmmxh;

void print(EventLoop* p = NULL)
{
  printf("main(): pid = %d, tid = %d, loop = %p\n",
         getpid(), CurrentThread::tid(), p);
}

void init(EventLoop* p)
{
  printf("init(): pid = %d, tid = %d, loop = %p\n",
         getpid(), CurrentThread::tid(), p);
}

int main()
{
  //Logger::setLogLevel(Logger::TRACE);
  print();

  EventLoop loop;
  loop.runAfter(11, std::bind(&EventLoop::quit, &loop));

  {
    printf("Single thread %p:\n", &loop);
    EventLoopThreadPool model(&loop, "single");
    model.setThreadNum(0);
    model.start(init);
    assert(model.getNextLoop() == &loop);
    assert(model.getNextLoop() == &loop);
    assert(model.getNextLoop() == &loop);
  }

  {
    printf("Another thread:\n");
    EventLoopThreadPool model(&loop, "another");
    model.setThreadNum(1);
    model.start(init);
    EventLoop* nextLoop = model.getNextLoop();
    nextLoop->runAfter(2, std::bind(print, nextLoop));
    assert(nextLoop != &loop);
    assert(nextLoop == model.getNextLoop());
    assert(nextLoop == model.getNextLoop());
    ::sleep(3);
  }

  {
    printf("Three threads:\n");
    EventLoopThreadPool model(&loop, "three");
    model.setThreadNum(3);
    model.start(init);
    EventLoop* nextLoop = model.getNextLoop();
    printf("get:%p\n",nextLoop);
    nextLoop->runInLoop(std::bind(print, nextLoop));
    assert(nextLoop != &loop);
    assert(nextLoop != model.getNextLoop());
    assert(nextLoop != model.getNextLoop());
    assert(nextLoop == model.getNextLoop());
  }
  loop.loop();
}
