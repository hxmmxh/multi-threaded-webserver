#include "../Thread/CurrentThread.h"
#include "Logging.h"
#include "../Time/Timestamp.h"
//#include "TimeZone.h" 该头文件暂时未完成，先用系统的Struct timezone替代

#include <cerrno>
#include <cstring>  //strerror_r
#include <stdlib.h> //getnv
#include <sstream>
#include <iostream>

namespace hxmmxh
{
__thread char t_errnobuf[512];
__thread char t_time[64];
__thread time_t t_lastSecond;

//在多线程程序中，errno会按照每个线程来储存，因此具有线程安全性。
//返回错误提示的字符串
//对于函数strerror_r，第一个参数errnum是错误代码，第二个参数buf是用户提供的存储错误描述的缓存，第三个参数n是缓存的大小。
const char *strerror_tl(int savedErrno)
{
    return strerror_r(savedErrno, t_errnobuf, sizeof(t_errnobuf));
}

//搜索 name 所指向的环境字符串，并返回相关的值给字符串。
//返回一个以 null 结尾的字符串，该字符串为被请求环境变量的值。如果该环境变量不存在，则返回 NULL。
Logger::LogLevel initLogLevel()
{
    if (getenv("LOG_TRACE"))
        return Logger::TRACE;
    else if (getenv("LOG_DEBUG"))
        return Logger::DEBUG;
    else
        return Logger::INFO;
}

Logger::LogLevel g_logLevel = initLogLevel();

const char *LogLevelName[Logger::NUM_LOG_LEVELS] =
    {
        "TRACE ",
        "DEBUG ",
        "INFO  ",
        "WARN  ",
        "ERROR ",
        "FATAL ",
};

//方便传递string对象
class T
{
public:
    T(const char *str, unsigned len)
        : str_(str),
          len_(len)
    {
        assert(strlen(str) == len_);
    }

    const char *str_;
    const unsigned len_;
};

//重载T和SourceFile对象的输入
inline LogStream &operator<<(LogStream &s, T v)
{
    s.append(v.str_, v.len_);
    return s;
}

inline LogStream &operator<<(LogStream &s, const Logger::SourceFile &v)
{
    s.append(v.data_, v.size_);
    return s;
}
/*
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
ptr-- 这是指向要被写入的元素数组的指针。
size-- 这是要被写入的每个元素的大小，以字节为单位。
nmemb-- 这是元素的个数，每个元素的大小为 size 字节。
stream-- 这是指向 FILE 对象的指针，该 FILE 对象指定了一个输出流。
如果成功，该函数返回一个 size_t 对象，表示元素的总数，该对象是一个整型数据类型。如果该数字与 nmemb 参数不同，则会显示一个错误
*/

//把msg写入到stdout中
void defaultOutput(const char *msg, int len)
{
    size_t n = ::fwrite(msg, 1, len, stdout);
    (void)n;
}
//fflush()会强迫将缓冲区内的数据写回参数stream 指定的文件中。
void defaultFlush()
{
    ::fflush(stdout);
}

//定义两个函数指针
Logger::OutputFunc g_output = defaultOutput;
Logger::FlushFunc g_flush = defaultFlush;

//struct timezone g_logTimeZone;

} // namespace hxmmxh

using namespace hxmmxh;

//time()返回自纪元 Epoch（1970-01-01 00:00:00 UTC）起经过的时间，以秒为单位
Logger::Impl::Impl(LogLevel level, int savedErrno, const SourceFile &file, int line)
    : time_(Timestamp::now()),
      stream_(),
      level_(level),
      line_(line),
      basename_(file)
{
    formatTime();         //先记录时间
    CurrentThread::tid(); //取得该线程的tid
    //输入线程id
    stream_ << T(CurrentThread::tidString(), CurrentThread::tidStringLength());
    //输入日志级别
    stream_ << T(LogLevelName[level], 6);
    //如果出错则记录错误信息
    if (savedErrno != 0)
    {
        stream_ << strerror_tl(savedErrno) << " (errno=" << savedErrno << ") ";
    }
}
//如果和上次记录的时间差距一秒以上，则在日志中记录时间
void Logger::Impl::formatTime()
{
    int64_t microSecondsSinceEpoch = time_.microSecondsSinceEpoch();
    time_t seconds = static_cast<time_t>(microSecondsSinceEpoch / Timestamp::kMicroSecondsPerSecond);
    int microseconds = static_cast<int>(microSecondsSinceEpoch % Timestamp::kMicroSecondsPerSecond);
    if (seconds != t_lastSecond)
    {
        t_lastSecond = seconds;
        struct tm tm_time;
        //获取当前时间结构，本地时间，有时区转换
        ::localtime_r(&seconds, &tm_time);

        int len = snprintf(t_time, sizeof(t_time), "%4d%02d%02d %02d:%02d:%02d",
                           tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                           tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
        assert(len == 17);
        (void)len;
    }
    Fmt us(".%06d ", microseconds);
    assert(us.length() == 8);
    stream_ << T(t_time, 17) << T(us.data(), 8);
}

void Logger::Impl::finish()
{
    stream_ << " - " << basename_ << ':' << line_ << '\n';
}

//Loglevel默认是INFO
Logger::Logger(SourceFile file, int line)
    : impl_(INFO, 0, file, line)
{
}

Logger::Logger(SourceFile file, int line, LogLevel level, const char *func)
    : impl_(level, 0, file, line)
{
    impl_.stream_ << func << ' ';
}

Logger::Logger(SourceFile file, int line, LogLevel level)
    : impl_(level, 0, file, line)
{
}

Logger::Logger(SourceFile file, int line, bool toAbort)
    : impl_(toAbort ? FATAL : ERROR, errno, file, line)
{
}

//析构Loggger时，将stream里的内容通过g_output写入设置的文件中
Logger::~Logger()
{
    impl_.finish();
    const LogStream::Buffer &buf(stream().buffer());
    g_output(buf.data(), buf.length());
    if (impl_.level_ == FATAL)
    {
        g_flush();
        abort();
    }
}

void Logger::setLogLevel(Logger::LogLevel level)
{
    g_logLevel = level;
}

void Logger::setOutput(OutputFunc out)
{
    g_output = out;
}

void Logger::setFlush(FlushFunc flush)
{
    g_flush = flush;
}

/* 
void Logger::setTimeZone(const TimeZone &tz)
{
    g_logTimeZone = tz;
}
*/