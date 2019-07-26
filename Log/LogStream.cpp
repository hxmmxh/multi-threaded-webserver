#include "LogStream.h"



//开启PRId64
//将<inttypes.h>改成<cinttypes>就不用加下面的宏了
/* 
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
*/
#include <algorithm>
#include <cassert>
#include <cstring>
#include <stdio.h>    //snprintf
#include <cinttypes> //PRId64
#include <limits> //numeric_limits
using namespace hxmmxh;
using namespace hxmmxh::detail;

namespace hxmmxh
{
namespace detail
{

const char digits[] = "9876543210123456789";
const char *zero = digits + 9;
static_assert(sizeof(digits) == 20, "wrong number of digits");
const char digitsHex[] = "0123456789ABCDEF";
static_assert(sizeof(digitsHex) == 17, "wrong number of digitsHex");

// int到string转换函数,value->buf，10进制
template <typename T>
size_t convert(char buf[], T value)
{
    T i = value;
    char *p = buf;
    //当value为0时，也会往buf中加入一个字符0
    //如果有while()，则会得到一个空的Buf
    do
    {
        //T的类型可能不是int，只有int能有%运算符
        int lsd = static_cast<int>(i % 10);
        i /= 10;
        *p++ = zero[lsd];
    } while (i != 0);

    if (value < 0)
    {
        *p++ = '-';
    }
    *p = '\0';
    std::reverse(buf, p);
    //返回得到的字符串的长度
    return p - buf;
}

//16进制转化，int->char[]
size_t convertHex(char buf[], uintptr_t value)
{
    uintptr_t i = value;
    char *p = buf;

    do
    {
        int lsd = static_cast<int>(i % 16);
        i /= 16;
        *p++ = digitsHex[lsd];
    } while (i != 0);

    *p = '\0';
    std::reverse(buf, p);

    return p - buf;
}

//显式实例化 C++ Primer P597
template class FixedBuffer<kSmallBuffer>;
template class FixedBuffer<kLargeBuffer>;
} // namespace detail

/*
 把一个数转换成能用5个字符表示(包括后面的单位和小数点)
 [0,     999]
 [1.00k, 999k]
 [1.00M, 999M]
 [1.00G, 999G]
 [1.00T, 999T]
 [1.00P, 999P]
 [1.00E, inf)
*/
std::string formatSI(int64_t s)
{
    double n = static_cast<double>(s);
    char buf[64];
    if (s < 1000)
        //"%" PRId64用来输出64位整数
        //在32位系统中是long long int，在64位系统中是long int
        //PRId64在32位系统中替换成ll,在64位系统中替换成l,实现跨平台效果
        snprintf(buf, sizeof(buf), "%" PRId64, s);
    //四舍五入留双，为了控制字符串长度，不同大小的数保留的小数位不同
    //100.5->100，而101.5->102
    else if (s < 9995)//0-9.99k
        snprintf(buf, sizeof(buf), "%.2fk", n / 1e3);
    else if (s < 99950)//10.0k-99.9k
        snprintf(buf, sizeof(buf), "%.1fk", n / 1e3);
    else if (s < 999500)//100k-999k
        snprintf(buf, sizeof(buf), "%.0fk", n / 1e3);
    else if (s < 9995000)
        snprintf(buf, sizeof(buf), "%.2fM", n / 1e6);
    else if (s < 99950000)
        snprintf(buf, sizeof(buf), "%.1fM", n / 1e6);
    else if (s < 999500000)
        snprintf(buf, sizeof(buf), "%.0fM", n / 1e6);
    else if (s < 9995000000)
        snprintf(buf, sizeof(buf), "%.2fG", n / 1e9);
    else if (s < 99950000000)
        snprintf(buf, sizeof(buf), "%.1fG", n / 1e9);
    else if (s < 999500000000)
        snprintf(buf, sizeof(buf), "%.0fG", n / 1e9);
    else if (s < 9995000000000)
        snprintf(buf, sizeof(buf), "%.2fT", n / 1e12);
    else if (s < 99950000000000)
        snprintf(buf, sizeof(buf), "%.1fT", n / 1e12);
    else if (s < 999500000000000)
        snprintf(buf, sizeof(buf), "%.0fT", n / 1e12);
    else if (s < 9995000000000000)
        snprintf(buf, sizeof(buf), "%.2fP", n / 1e15);
    else if (s < 99950000000000000)
        snprintf(buf, sizeof(buf), "%.1fP", n / 1e15);
    else if (s < 999500000000000000)
        snprintf(buf, sizeof(buf), "%.0fP", n / 1e15);
    else
        snprintf(buf, sizeof(buf), "%.2fE", n / 1e18);
    return buf;
}

/*
 [0, 1023]
 [1.00Ki, 9.99Ki]
 [10.0Ki, 99.9Ki]
 [ 100Ki, 1023Ki]
 [1.00Mi, 9.99Mi]
*/
std::string formatIEC(int64_t s)
{
    double n = static_cast<double>(s);
    char buf[64];
    const double Ki = 1024.0;
    const double Mi = Ki * 1024.0;
    const double Gi = Mi * 1024.0;
    const double Ti = Gi * 1024.0;
    const double Pi = Ti * 1024.0;
    const double Ei = Pi * 1024.0;

    if (n < Ki)//0-1023
        snprintf(buf, sizeof(buf), "%" PRId64, s);
    else if (n < Ki * 9.995)//1-9.99Ki
        snprintf(buf, sizeof(buf), "%.2fKi", n / Ki);
    else if (n < Ki * 99.95)
        snprintf(buf, sizeof(buf), "%.1fKi", n / Ki);
    else if (n < Ki * 1023.5)
        snprintf(buf, sizeof(buf), "%.0fKi", n / Ki);

    else if (n < Mi * 9.995)
        snprintf(buf, sizeof(buf), "%.2fMi", n / Mi);
    else if (n < Mi * 99.95)
        snprintf(buf, sizeof(buf), "%.1fMi", n / Mi);
    else if (n < Mi * 1023.5)
        snprintf(buf, sizeof(buf), "%.0fMi", n / Mi);

    else if (n < Gi * 9.995)
        snprintf(buf, sizeof(buf), "%.2fGi", n / Gi);
    else if (n < Gi * 99.95)
        snprintf(buf, sizeof(buf), "%.1fGi", n / Gi);
    else if (n < Gi * 1023.5)
        snprintf(buf, sizeof(buf), "%.0fGi", n / Gi);

    else if (n < Ti * 9.995)
        snprintf(buf, sizeof(buf), "%.2fTi", n / Ti);
    else if (n < Ti * 99.95)
        snprintf(buf, sizeof(buf), "%.1fTi", n / Ti);
    else if (n < Ti * 1023.5)
        snprintf(buf, sizeof(buf), "%.0fTi", n / Ti);

    else if (n < Pi * 9.995)
        snprintf(buf, sizeof(buf), "%.2fPi", n / Pi);
    else if (n < Pi * 99.95)
        snprintf(buf, sizeof(buf), "%.1fPi", n / Pi);
    else if (n < Pi * 1023.5)
        snprintf(buf, sizeof(buf), "%.0fPi", n / Pi);

    else if (n < Ei * 9.995)
        snprintf(buf, sizeof(buf), "%.2fEi", n / Ei);
    else
        snprintf(buf, sizeof(buf), "%.1fEi", n / Ei);
    return buf;
}

} // namespace hxmmxh

template <int SIZE>
const char *FixedBuffer<SIZE>::debugString()
{
    *cur_ = '\0';
    return data_;
}

template <int SIZE>
void FixedBuffer<SIZE>::cookieStart()
{
}

template <int SIZE>
void FixedBuffer<SIZE>::cookieEnd()
{
}

void LogStream::staticCheck()
{
    // digits10返回对应类型十进制数的个数 
    //类型 T 能无更改地表示的底 10 位数，即任何拥有这么多十进制有效数字的数能转换成 T 的值并转换回十进制形式，而不因舍入或上溢而更改
    //float为6，double为15，long double为18
    static_assert(kMaxNumericSize - 10 > std::numeric_limits<double>::digits10,
                  "kMaxNumericSize is large enough");
    static_assert(kMaxNumericSize - 10 > std::numeric_limits<long double>::digits10,
                  "kMaxNumericSize is large enough");
    static_assert(kMaxNumericSize - 10 > std::numeric_limits<long>::digits10,
                  "kMaxNumericSize is large enough");
    static_assert(kMaxNumericSize - 10 > std::numeric_limits<long long>::digits10,
                  "kMaxNumericSize is large enough");
}

//把intz转换成字符串再输入
template<typename T>
void LogStream::formatInteger(T v)
{
  if (buffer_.avail() >= kMaxNumericSize)
  {
    size_t len = convert(buffer_.current(), v);
    buffer_.add(len);
  }
}

//short提升到相应的int
LogStream& LogStream::operator<<(short v)
{
  *this << static_cast<int>(v);
  return *this;
}

LogStream& LogStream::operator<<(unsigned short v)
{
  *this << static_cast<unsigned int>(v);
  return *this;
}
//int,long,long long转换成字符串后插入
LogStream& LogStream::operator<<(int v)
{
  formatInteger(v);
  return *this;
}

LogStream& LogStream::operator<<(unsigned int v)
{
  formatInteger(v);
  return *this;
}

LogStream& LogStream::operator<<(long v)
{
  formatInteger(v);
  return *this;
}

LogStream& LogStream::operator<<(unsigned long v)
{
  formatInteger(v);
  return *this;
}

LogStream& LogStream::operator<<(long long v)
{
  formatInteger(v);
  return *this;
}

LogStream& LogStream::operator<<(unsigned long long v)
{
  formatInteger(v);
  return *this;
}

//输出指针
LogStream& LogStream::operator<<(const void* p)
{
  uintptr_t v = reinterpret_cast<uintptr_t>(p);
  if (buffer_.avail() >= kMaxNumericSize)
  {
    char* buf = buffer_.current();
    buf[0] = '0';
    buf[1] = 'x';
    size_t len = convertHex(buf+2, v);
    buffer_.add(len+2);
  }
  return *this;
}

LogStream& LogStream::operator<<(double v)
{
  if (buffer_.avail() >= kMaxNumericSize)
  {
    //%g 把输出的值按照%e或者%f类型中输出长度较小的方式输出
    //%e 指数形式的浮点数
    int len = snprintf(buffer_.current(), kMaxNumericSize, "%.12g", v);
    buffer_.add(len);
  }
  return *this;
}

//以fmt的格式输出T
template<typename T>
Fmt::Fmt(const char* fmt, T val)
{
  static_assert(std::is_arithmetic<T>::value == true, "Must be arithmetic type");

  length_ = snprintf(buf_, sizeof(buf_), fmt, val);
  assert(static_cast<size_t>(length_) < sizeof(buf_));
}

// 显式实例化
template Fmt::Fmt(const char* fmt, char);

template Fmt::Fmt(const char* fmt, short);
template Fmt::Fmt(const char* fmt, unsigned short);
template Fmt::Fmt(const char* fmt, int);
template Fmt::Fmt(const char* fmt, unsigned int);
template Fmt::Fmt(const char* fmt, long);
template Fmt::Fmt(const char* fmt, unsigned long);
template Fmt::Fmt(const char* fmt, long long);
template Fmt::Fmt(const char* fmt, unsigned long long);

template Fmt::Fmt(const char* fmt, float);
template Fmt::Fmt(const char* fmt, double);
