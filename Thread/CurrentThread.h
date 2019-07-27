#ifndef HXMMXH_CURRENTTHREAD_H
#define HXMMXH_CURRENTTHREAD_H

#include <string>

namespace hxmmxh
{
namespace CurrentThread
{
// 要在*.c文件中引用另一个文件中的一个全局的变量，那就应该放在*.h中用extern来声明这个全局变量。
extern __thread int t_cachedTid;
extern __thread char t_tidString[32];
extern __thread int t_tidStringLength;
extern __thread const char *t_threadName;
void cacheTid();//缓存该线程的tid，即修改t_cachedTid的值

inline int tid()
{
    //允许程序员将最有可能执行的分支告诉编译器
    //__builtin_expect((x),1)表示 x 的值为真的可能性更大；
    //__builtin_expect((x),0)表示 x 的值为假的可能性更大。
    //一般情况下都不用调用cacheTid
    if (__builtin_expect(t_cachedTid == 0, 0))
    {
        cacheTid();
    }
    return t_cachedTid;
}

inline const char *tidString() // for logging
{
    return t_tidString;
}

inline int tidStringLength() // for logging
{
    return t_tidStringLength;
}

inline const char *name()
{
    return t_threadName;
}

bool isMainThread();


} // namespace CurrentThread
} // namespace hxmmxh

#endif
