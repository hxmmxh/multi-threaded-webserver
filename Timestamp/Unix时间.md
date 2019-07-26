

## 定义
GMT和UTC

GMT，即格林尼治标准时间，也就是世界时。GMT的正午是指当太阳横穿格林尼治子午线（本初子午线）时的时间。但由于地球自转不均匀不规则，导致GMT不精确，现在已经不再作为世界标准时间使用。

UTC，即协调世界时。UTC是以原子时秒长为基础，在时刻上尽量接近于GMT的一种时间计量系统。为确保UTC与GMT相差不会超过0.9秒，在有需要的情况下会在UTC内加上正或负闰秒。UTC现在作为世界标准时间使用。
### Unix时间戳(Unix timestamp)
定义为从格林威治时间1970年01月01日00时00分00秒起至现在的总秒数
### 溢出问题
如果32位二进制数字表示时间。此类系统的Unix时间戳最多可以使用到格林威治时间2038年01月19日03时14分07秒（二进制：01111111 11111111 11111111 11111111）。其后一秒，二进制数字会变为10000000 00000000 00000000 00000000，发生溢出错误，造成系统将时间误解为1901年12月13日20时45分52秒。这很可能会引起软件故障，甚至是系统瘫痪。使用64位二进制数字表示时间的系统（最多可以使用到格林威治时间292,277,026,596年12月04日15时30分08秒）则基本不会遇到这类溢出问题。

* [gettimeofday](https://www.cnblogs.com/xiaojianliu/p/8477461.html)
*[时间函数](https://blog.csdn.net/u010507799/article/details/52288190)
[11](https://blog.csdn.net/u014260855/article/details/44403287)

## 常用函数
### 常用结构体
```cpp
#include<time.h>
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
struct timeval
{
  long tv_sec;  //秒
  long tv_usec; //微秒
};
struct timezone
{
  int tz_minuteswest; //和格林威治时间差了多少分钟
  int tz_dsttime;     //夏令时标识符
};
```

### 常用函数

```cpp
#include<time.h>
//返回UTC标准秒数，没有时区转换,即1970年01月01日00时00分00秒起至现在的总秒数
time_t time(time_t *t);
//获取当前时间结构，UTC时间，无时区转换
//将timep这个秒数转换成以UTC时区为标准的年月日时分秒时间
//gmtime_r除了返回结果外，还会把结果保存在传入的内存中
struct tm *gmtime(const time_t *timep)；
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
