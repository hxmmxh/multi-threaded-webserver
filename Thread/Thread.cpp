#include "Thread.h"
//#include "../Log/Logging.h"
#include "CurrentThread.h"

#include <cassert>
#include <stdio.h>     //snprintf
#include <sys/prctl.h> //prctl
#include <utility>     //move

using std::string;

namespace hxmmxh
{
namespace detail
{

// fork一个新进程后
void afterFork()
{
  CurrentThread::t_cachedTid = 0;
  CurrentThread::t_threadName = "main";
  // 先把t_cachedTid设为0，tid()内部才会重新去获得线程ID
  CurrentThread::tid();
}

class ThreadNameInitializer
{
public:
  ThreadNameInitializer()
  {
    CurrentThread::t_threadName = "main";
    CurrentThread::tid();
    //作用是fork之后在子进程中立马调用afterfork
    pthread_atfork(NULL, NULL, &afterFork);
  }
};

ThreadNameInitializer init;

struct ThreadData
{
  typedef Thread::ThreadFunc ThreadFunc;
  ThreadFunc func_;
  string name_;
  // 下面两个设为指针因为需要被修改
  pid_t *tid_;
  CountDownLatch *latch_;

  ThreadData(ThreadFunc func,
             const string &name,
             pid_t *tid,
             CountDownLatch *latch)
      : func_(std::move(func)),
        name_(name),
        tid_(tid),
        latch_(latch)
  {
  }

  //在线程中运行函数
  void runInThread()
  {
    *tid_ = CurrentThread::tid();
    tid_ = NULL;
    //每执行一个线程，倒计时减一
    latch_->countDown();
    latch_ = NULL;
    CurrentThread::t_threadName = name_.empty() ? "DefaultThread" : name_.c_str();
    //设置线程名字
    ::prctl(PR_SET_NAME, CurrentThread::t_threadName);
    //运行函数
    func_();
    //运行结束
    CurrentThread::t_threadName = "finished";
  }
};

//传入的obj一定要是new创建的
void *startThread(void *obj)
{
  ThreadData *data = static_cast<ThreadData *>(obj);
  data->runInThread();
  delete data;
  return NULL;
}

} // namespace detail

//初始化为0
std::atomic_int32_t Thread::numCreated_;

Thread::Thread(ThreadFunc func, const string &n)
    : started_(false),
      joined_(false),
      pthreadId_(0),
      tid_(0),
      func_(std::move(func)),
      name_(n),
      latch_(1)
{
  setDefaultName();
}

// 如果开始运行且没有被join，则分离
Thread::~Thread()
{
  if (started_ && !joined_)
  {
    pthread_detach(pthreadId_);
  }
}

void Thread::setDefaultName()
{
  int num = ++numCreated_;
  //如果没有显示指定线程名，则简单的用序号表示
  if (name_.empty())
  {
    char buf[32];
    snprintf(buf, sizeof buf, "Thread%d", num);
    name_ = buf;
  }
}

void Thread::start()
{
  assert(!started_);
  started_ = true;
  detail::ThreadData *data = new detail::ThreadData(func_, name_, &tid_, &latch_);
  //成功返回0,失败返回正的错误码
  //运行data->startThread
  if (pthread_create(&pthreadId_, NULL, &detail::startThread, data))
  {
    started_ = false;
    delete data; 
    //LOG_SYSFATAL << "Failed in pthread_create";
    abort();
  }
  else
  {
    //创建成功后，等待倒计时变成0,即创建的线程开始执行
    latch_.wait();
    assert(tid_ > 0);
  }
}

//等待线程执行完毕
int Thread::join()
{
  assert(started_);
  assert(!joined_);
  joined_ = true;
  return pthread_join(pthreadId_, NULL);
}
} // namespace hxmmxh