#ifndef HXMMXH_THREAD_CURRENTTHREAD_H
#define HXMMXH_THREAD_CURRENTTHREAD_H

#include <string>

namespace hxmmxh
{
namespace CurrentThread
{
// __thread表示每个当前线程有一个该变量的实例.
// __thread变量每一个线程有一份独立实体，各个线程的值互不干扰
// 只能修饰POD类型(不带自定义的构造、拷贝、赋值、析构的类型，二进制内容可以任意复制memset,memcpy,且内容可以复原)
// 不能修饰class类型
// 要在*.c文件中引用另一个文件中的一个全局的变量，那就应该放在*.h中用extern来声明这个全局变量。
extern __thread int t_cachedTid;
//用string保存线程ID
extern __thread char t_tidString[32];
extern __thread int t_tidStringLength;
extern __thread const char *t_threadName;

void cacheTid();//缓存该线程的tid，即修改t_cachedTid的值

//返回线程ID
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

inline const char *tidString() 
{
    return t_tidString;
}

inline int tidStringLength() 
{
    return t_tidStringLength;
}

inline const char *name()
{
    return t_threadName;
}

bool isMainThread();

void sleepUsec(int64_t usec); 

} // namespace CurrentThread
} // namespace hxmmxh

#endif
