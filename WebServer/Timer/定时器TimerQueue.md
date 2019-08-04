传统方法只能通过控制select，poll或e'poll的等待时间来实现定时，现在可以使用timerfd用处理IO事件相同的方式来处理定时

### timerfd简介
#### 调用创建一个新的定时器对象 
```cpp
#include <sys/timerfd.h>
int timerfd_create(int clockid, int flags);
//成功返回一个指代该对象的文件描述符, 失败返回-1以及设置errno
```
* clockid的可能取值如下
>CLOCK_REALTIME:                 系统实时时间,随系统实时时间改变而改变,即从UTC1970-1-1 0:0:0开始计时,如果系统时间被用户改成其他,则对应的时间相应改变  
>CLOCK_REALTIME_COARSE：          和CLOCK_REALTIME类似，但是执行速度快，精度低  
>CLOCK_MONOTONIC:                 从系统启动这一刻起开始计时,不受系统时间被用户改变的影响  
>CLOCK_MONOTONIC_COARSE ：        和CLOCK_MONOTONIC类似，但是执行速度快，精度低  
>CLOCK_BOOTTIME：                和CLOCK_MONOTONIC 类似，但是包括了系统休眠的时间。  
>CLOCK_PROCESS_CPUTIME_ID:       本进程到当前代码系统CPU花费的时间  
>CLOCK_THREAD_CPUTIME_ID:        本线程到当前代码系统CPU花费的时间  

* flags的可能值如下
>TFD_CLOEXEC 为新的文件描述符设置运行时关闭标志 （FD_CLOEXEC)
>TFD_NONBLOCK 为底层的打开文件描述符设置 O_NONBLOCK 标志， 随后的读操作将是非阻塞的， 这与调用 fcntl 效果相同
>0 采用默认设置

#### 启动或停止由文件描述符 fd 所指代的定时器
```c
#include <sys/timerfd.h>
int timerfd_settime(int fd, int flags, const struct itimerspec* new_value, struct itimerspec* old_value);
//成功返回0, 失败返回-1以及设置errno
struct itimerspec 
{
    struct timespec it_interval;   //间隔时间
    struct timespec it_value;      //第一次到期时间
};
struct timespec 
{
    time_t tv_sec;    //秒
    long tv_nsec;    //纳秒
}; 
int clock_gettime(clockid_t clk_id, struct timespec *tp);
//返回0成功， 1失败。得到的时间信息存储再tp中
```
* 参数flags为1代表设置的是绝对时间；为0代表相对时间
* 如果 timerfd_settime 设置为 TFD_TIMER_ABSTIME(绝对时间)，则后面的时间必须用 clock_gettime 来获取，获取时设置 CLOCK_REALTIME 还是 CLOCK_MONOTONIC 取决于 timerfd_create 设置的值
* new_value： 指定新的超时时间，若 newValue.it_value非 0 则启动定时器，否则关闭定时器。若 newValue.it_interval 为 0 则定时器只定时一次，否则之后每隔设定时间超时一次。
* old_value：不为 NULL 时则返回定时器这次设置之前的超时时间。


#### 返回文件描述符 fd 所标识定时器的间隔及剩余时间
```cpp
#include <sys/timerfd.h>
int timerfd_gettime(int fd, struct itimerspec *curr_value);
//成功返回0, 失败返回-1和errno
```
间隔和距离下次到期的时间均返回到 curr_value 指向的结构体.
如果返回的结构中 curr_value.it_value 中所有字段值均为0, 那么该定时器已经解除, 如果返回的结构 curr_value.it_interval 中两字段值均为0, 那么该定时器只会到期一次, 到期时间在 curr_value.it_value 中给出

#### read一个timerfd
read函数可以读timerfd，读的内容为uint_64，表示超时次数。