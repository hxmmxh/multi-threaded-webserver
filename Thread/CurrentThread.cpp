#include "CurrentThread.h"

#include <unistd.h>      //syscall
#include <sys/syscall.h> //SYS-gettid

namespace hxmmxh
{
namespace CurrentThread
{

pid_t gettid()
{
  //#define SYS_gettid __NR_gettid
  return static_cast<pid_t>(::syscall(SYS_gettid));
}

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
    t_cachedTid = gettid();
    t_tidStringLength = snprintf(t_tidString, sizeof(t_tidString), "%5d ", t_cachedTid);
  }
}

bool isMainThread()
{
  //getpid获取的是进程号，每个进程的第一个线程号等于进程号
  return tid() == ::getpid();
}

} // namespace CurrentThread
} // namespace hxmmxh
