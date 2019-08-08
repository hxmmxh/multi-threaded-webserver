#include "../EventLoop.h"
#include "../../../Thread/Thread.h"
#include <stdio.h>

using namespace hxmmxh;

void threadFunc()
{
  printf("threadFunc(): pid = %d, tid = %d\n",
         getpid(), CurrentThread::tid());

  EventLoop loop;
  loop.loop();
}

int main()
{
  printf("main(): pid = %d, tid = %d\n",
         getpid(), CurrentThread::tid());

  EventLoop loop;

  Thread thread(threadFunc);
  thread.start();

  loop.loop();
  pthread_exit(NULL);
}
