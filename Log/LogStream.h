#ifndef HXMMXH_LOGSTREAM_H
#define HXMMXH_LOGSTREAM_H

#include "StringPiece.h"

#include <cassert>
#include <string>
#include <cstring> //memset,bzero

namespace hxmmxh
{

using std::string;
namespace detail
{

const int kSmallBuffer = 4000;
const int kLargeBuffer = 4000 * 1000;

template <int SIZE>
class FixedBuffer
{
public:
    FixedBuffer() : cur_(data_) { setCookie(cookieStart); }
    ~FixedBuffer() { setCookie(cookieEnd); }

    //插入长度为len的buf字符数组
    void append(const char *buf, int len)
    {
        if (avail() > len)
        {
            memcpy(cur_, buf, len);
            cur_ += len;
        }
    }

    const char *data() const { return data_; }
    int length() const { return cur_ - data_; } //数据长度

    char *current() { return cur_; }
    int avail() const { return static_cast<int>(end() - cur_); }
    void add(size_t len) { cur_ += len; }

    //reset只移动cur_，不修改里面存储的数据
    void reset() { cur_ = data_; }
    void bzero() { ::memset(data_, 0, sizeof(data_)); }

    //转换成标准的string，在末尾加上‘\0'终止符
    const char *debugString();
    void setCookie(void (*cookie)()) { cookie_ = cookie; }
    string toString() const { return string(data_, length()); }
    StringPiece toStringPiece() const { return StringPiece(data_, length()); }

private:
    const char *end() const { return data_ + sizeof(data_); }
    static void cookieStart();
    static void cookieEnd();
    void (*cookie_)(); //函数指针
    char data_[SIZE];  //内部存储字符的数字
    char *cur_;        //类似尾后指针，可插入的第一个位置
};
} // namespace detail

class LogStream
{
public:
    typedef detail::FixedBuffer<detail::kSmallBuffer> Buffer;
    typedef LogStream self;
    //C++内置类型，整形(字符，整形，bool)，浮点型，分别重载输出运算符
    //重载布尔值输出
    self &operator<<(bool v)
    {
        buffer_.append(v ? "1" : "0", 1);
        return *this;
    }
    //重载4中整形输出
    self &operator<<(short);
    self &operator<<(unsigned short);
    self &operator<<(int);
    self &operator<<(unsigned int);
    self &operator<<(long);
    self &operator<<(unsigned long);
    self &operator<<(long long);
    self &operator<<(unsigned long long);
    //重载指针输出
    self &operator<<(const void *);
    //重载浮点输出
    self &operator<<(float v)
    {
        *this << static_cast<double>(v);
        return *this;
    }
    self &operator<<(double);
    //重载字符输出
    self &operator<<(char v)
    {
        buffer_.append(&v, 1);
        return *this;
    }
    self &operator<<(const char *str)
    {
        if (str)
        {
            buffer_.append(str, strlen(str));
        }
        else
        {
            buffer_.append("(null)", 6);
        }
        return *this;
    }
    //重载字符串输出
    self &operator<<(const string &v)
    {
        buffer_.append(v.c_str(), v.size());
        return *this;
    }
    self &operator<<(const StringPiece &v)
    {
        buffer_.append(v.data(), v.size());
        return *this;
    }

    self &operator<<(const Buffer &v)
    {
        *this << v.toStringPiece();
        return *this;
    }
    //往data里加入len长的data
    void append(const char *data, int len) { buffer_.append(data, len); }
    const Buffer &buffer() const { return buffer_; }
    void resetBuffer() { buffer_.reset(); }

private:
    void staticCheck();
    template <typename T>
    void formatInteger(T);

    Buffer buffer_;

    //每个输入的数字最大的显示长度
    static const int kMaxNumericSize = 32;
};

//格式化输出
//以fmt的格式输出T
//fmt类似printf函数的输出格式
class Fmt
{
public:
    template <typename T>
    Fmt(const char *fmt, T val);

    const char *data() const { return buf_; }
    int length() const { return length_; }

private:
    char buf_[32];
    int length_;
};

inline LogStream &operator<<(LogStream &s, const Fmt &fmt)
{
    s.append(fmt.data(), fmt.length());
    return s;
}

//将大整数加上单位，格式化成长度不超过5的字符串(包含单位，即最多4个数字)
// (k, M, G, T, P, E)
//SI单位制，分别为1000，10^6, 10^9, 10^12, 10^15, 10^18
//要求n大于0
string formatSI(int64_t n);

////将大整数加上单位，格式化成长度不超过6的字符串
// IEC (binary) (Ki, Mi, Gi, Ti, Pi, Ei).
//IEC单位制，2^10, 2^20, 2^30, 2^40 ,2^50, 2^60
//要求n大于0
string formatIEC(int64_t n);

} // namespace hxmmxh

#endif