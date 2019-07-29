#include "ThreadPool.h"
#include "CurrentThread.h"

#include <iostream>
#include <unistd.h>
#include <utility>

using namespace hxmmxh;
using namespace std;
void print()
{
  cout << "tid= " << CurrentThread::tid() << '\n';
}

void printString(const std::string &str)
{
  std::cout << str << '\n';
  usleep(100 * 1000); //微秒
}

void test(int maxSize)
{
  std::cout << "Test ThreadPool with max queue size = " << maxSize << '\n';
  ThreadPool pool("MainThreadPool");
  pool.setMaxQueueSize(maxSize);
  //pool.setThreadInitCallback(print);
  pool.start(5);

  std::cout << "Adding\n";
  pool.run(print);
  pool.run(print);
  for (int i = 0; i < 100; ++i)
  {
    char buf[32];
    snprintf(buf, sizeof(buf), "task %d", i);
    pool.run(std::bind(printString, std::string(buf)));
  }

  CountDownLatch latch(1);
  //pool.run([] { cout << "run!\n"; });
  pool.run(std::bind(&CountDownLatch::countDown, &latch));
  latch.wait();
  pool.stop();
}
/* 
void testMove()
{
  ThreadPool pool;
  pool.start(2);

  std::unique_ptr<int> x(new int(42));
  pool.run([y = std::move(x)] { printf("%d: %d\n", CurrentThread::tid(), *y); });
  pool.stop();
}
*/
int main()
{
  test(0);
  test(1);
  test(5);
  test(10);
  test(50);
  //testMove();
}
