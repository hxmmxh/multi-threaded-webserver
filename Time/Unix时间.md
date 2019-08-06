

## 基础定义概念
### GMT和UTC
GMT，即格林尼治标准时间，也就是世界时。GMT的正午是指当太阳横穿格林尼治子午线（本初子午线）时的时间。但由于地球自转不均匀不规则，导致GMT不精确，现在已经不再作为世界标准时间使用。

UTC，即协调世界时。UTC是以原子时秒长为基础，在时刻上尽量接近于GMT的一种时间计量系统。为确保UTC与GMT相差不会超过0.9秒，在有需要的情况下会在UTC内加上正或负闰秒。UTC现在作为世界标准时间使用。

### JulianDay儒略日
* 在儒略周期内以连续的日数计算时间的计时法
*儒略日数（Julian Day Number，JDN）：从格林威治标准时间的中午开始，包含一个整天的时间，起点的时间（0日）回溯至儒略历的公元前4713年1月1日中午12点
* 儒略日期（Julian date，JD）：是以格林威治标准时中午12:00的儒略日加上那一天的瞬时时间的分数。儒略日期是儒略日添加小数部分所表示的儒略日数
* 儒略周期（Julian Period）: 开始于公元前4713年，每一周期长达7980年，下一个儒略周期将开始于公元3268年
[计算公式](https://blog.csdn.net/Solstice/article/details/5814486)

### Unix时间戳(Unix timestamp)
定义为从格林威治时间1970年01月01日00时00分00秒起至现在的总秒数
### 溢出问题
如果32位二进制数字表示时间。此类系统的Unix时间戳最多可以使用到格林威治时间2038年01月19日03时14分07秒（二进制：01111111 11111111 11111111 11111111）。其后一秒，二进制数字会变为10000000 00000000 00000000 00000000，发生溢出错误，造成系统将时间误解为1901年12月13日20时45分52秒。这很可能会引起软件故障，甚至是系统瘫痪。使用64位二进制数字表示时间的系统（最多可以使用到格林威治时间292,277,026,596年12月04日15时30分08秒）则基本不会遇到这类溢出问题。

*[时间函数](https://blog.csdn.net/u010507799/article/details/52288190)
[11](https://blog.csdn.net/u014260855/article/details/44403287)

## <time.h>常用函数
### 常用结构体
```cpp
#include<time.h>
//时间结构体
struct tm
{
    int tm_sec;   //代表目前秒数，正常范围为0-59，但允许至61秒
    int tm_min;   //代表目前分数，范围0-59
    int tm_hour;  //从午夜算起的时数，范围为0-23
    int tm_mday;  //目前月份的日数，范围01-31
    int tm_mon;   //代表目前月份，从一月算起，范围从0-11
    int tm_year;  //从1900年算起至今的年数
    int tm_wday;  //一星期的日数，从星期天算起，范围为0-6
    int tm_yday;  //从今年1月1日算起至今的天数，范围为0-365
    int tm_isdst; //夏令时标识符，实行夏令时的时候，tm_isdst为正。不实行夏令时的进候，tm_isdst为0；不了解情况时，tm_isdst()为负
};
//更高精度的timr_t，精确到微妙
struct timeval
{
  long tv_sec;  //秒
  long tv_usec; //微秒
};
//精确到纳秒
struct timespec
{
  time_t  tv_sec;         /* 秒seconds */
  long    tv_nsec;        /* 纳秒nanoseconds */
};
//时区
struct timezone
{
  int tz_minuteswest; //和格林威治时间差了多少分钟
  int tz_dsttime;     //夏令时标识符
};
```

### 常用函数

```c
#include<time.h>
//返回UTC标准秒数，没有时区转换,即1970年01月01日00时00分00秒起至现在的总秒数
time_t time(time_t *t);
//获取当前时间结构，UTC时间，无时区转换
//将timep这个秒数转换成以UTC时区为标准的年月日时分秒时间
//gmtime_r除了返回结果外，还会把结果保存在传入的内存中
struct tm *gmtime(const time_t *timep);
struct tm *gmtime_r(const time_t *timep, struct tm *result);  
//获取当前时间结构，本地时间，有时区转换
struct tm *localtime(const time_t * timep);
struct tm *localtime_r(const time_t *timep, struct tm *result);
//将时间结构转换为UTC秒数，有时区转换
//将已经根据时区信息计算好的structtm转换成time_t的秒数。计算出的秒数是以UTC时间为标准的
time_t mktime(struct tm *tm);
//获取当前时间，UTC时间，精度微妙，无时区转换
//结果储存在传入的参数tv中
//timezone一般传入NULL
int gettimeofday(struct timeval *tv, struct timezone *tz);
//将时间转换为本地时间字符串， 有时区转换
char *ctime(const time_t *timep);
//将时间转换为字符串， 无时区转换
char * asctime(const struct tm * timeptr)
```
* 获取系统当前时间
```cpp
int clock_gettime(clockid_t clk_id, struct timespec *tp);
//返回0成功， 1失败。得到的时间信息存储再tp中
//clk_id的可能取值如下
CLOCK_REALTIME:                 系统实时时间,随系统实时时间改变而改变,即从UTC1970-1-1 0:0:0开始计时,如果系统时间被用户改成其他,则对应的时间相应改变
CLOCK_REALTIME_COARSE：          和CLOCK_REALTIME类似，但是执行速度快，精度低
CLOCK_MONOTONIC:                 从系统启动这一刻起开始计时,不受系统时间被用户改变的影响
CLOCK_MONOTONIC_COARSE ：        和CLOCK_MONOTONIC类似，但是执行速度快，精度低
CLOCK_BOOTTIME：                和CLOCK_MONOTONIC 类似，但是包括了系统休眠的时间。
CLOCK_PROCESS_CPUTIME_ID:       本进程到当前代码系统CPU花费的时间
CLOCK_THREAD_CPUTIME_ID:        本线程到当前代码系统CPU花费的时间
```
## sleep相关函数
* [参考](https://www.jianshu.com/p/42abcc2c9e50)
* 秒的换算：ms(毫秒),μs(微秒),ns(纳秒),ps(皮秒)  
* 1s = 1000ms = 1000 * 1000us = 1000 * 1000 * 1000ns = 1000 * 1000 * 1000* 1000ps
* 函数的精确度与时钟的频率有关系：例如假设时钟中断是10纳秒一次，如果tv_sec = 0, tv_nsec = 2,那么时钟中断一定是在10纳秒后来唤醒这个进程的
```cpp
//sleep()-------以秒为单位
#include<unistd.h>
unsigned int sleep(unsigned int seconds);
//若进程暂停到参数seconds 所指定的时间，成功则返回0，若有信号中断则返回剩余秒数。
//在linux中，sleep是通过nanosleep实现的。在一些其他系统中（例如POSIX.1），它是通过alarm()来实现的。

//usleep()----以微秒为单位
#include<unistd.h>
unsigned int usleep(unsigned int useconds);
//若进程暂停到参数seconds 所指定的时间，成功则返回0，若有信号中断则返回剩余微秒数。

//nanosleep( )---------以纳秒为单位
#include<time.h>
int nanosleep(const struct timespec *req, struct timespec *rem);
//若进程暂停到参数*req所指定的时间，成功则返回0，若有信号中断则返回-1，并且将剩余微秒数记录在*rem中。
```