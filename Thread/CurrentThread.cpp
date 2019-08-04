#include "CurrentThread.h"
#include "../Time/Timestamp.h"

#include <sys/syscall.h> //syscall
#include <time.h>        //nanosleep,timespec
#include <unistd.h>      //getpid

namespace hxmmxh
{
namespace detail
{
pid_t gettid()
{
  return static_cast<pid_t>(::syscall(SYS_gettid));
}

} // namespace detail
namespace CurrentThread
{

__thread int t_cachedTid = 0;
__thread char t_tidString[32];
__thread int t_tidStringLength = 6;
__thread const char *t_threadName = "unknown";
//在本人系统上pid_t是int64_t类型，先注释
//static_assert(std::is_same<int, pid_t>::value, "pid_t should be int64_t");
void cacheTid()
{
  if (t_cachedTid == 0)
  {
    t_cachedTid = detail::gettid();
    t_tidStringLength = snprintf(t_tidString, sizeof(t_tidString), "%5d ", t_cachedTid);
  }
}

bool isMainThread()
{
  //getpid获取的是进程号，每个进程的第一个线程号等于进程号
  return tid() == ::getpid();
}

void sleepUsec(int64_t usec)
{
  struct timespec ts = {0, 0};
  ts.tv_sec = static_cast<time_t>(usec / Timestamp::kMicroSecondsPerSecond);
  ts.tv_nsec = static_cast<long>(usec % Timestamp::kMicroSecondsPerSecond * 1000);
  ::nanosleep(&ts, NULL);
}
} // namespace CurrentThread
} // namespace hxmmxh
