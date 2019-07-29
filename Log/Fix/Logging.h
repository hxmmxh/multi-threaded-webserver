#ifndef HXMMXH_LOGGING_H
#define HXMMXH_LOGGING_H

#include "LogStream.h"
#include "Timestamp.h"

#include <stdio.h>
#include <time.h>

namespace hxmmxh
{

class Logger
{
public:
  //日志等级
  enum LogLevel
  {
    TRACE,
    DEBUG,
    INFO,  //打印一些感兴趣的或者重要的信息
    WARN,  //警告,进行一些修复性的工作，应该还可以把系统恢复到正常状态中来，系统应该可以继续运行下去。
    ERROR, //错误,可以进行一些修复性的工作，但无法确定系统会正常的工作下去
    FATAL, //严重错误，无法修复的错误，尽可能地保留系统有效数据并停止运行
    NUM_LOG_LEVELS,
  };
  //嵌套类，用作外层类的实现
  //封装记录LOG的文件名
  class SourceFile
  {
  public:
    template <int N>
    SourceFile(const char (&arr)[N]) : data_(arr), size_(N - 1)
    {
      //通过传入的地址找到记录LOG的文件名
      //strrchr 查找一个字符c在另一个字符串str中末次出现的位置（也就是从str的右侧开始查找字符c首次出现的位置），并返回这个位置的地址
      //如果未能找到指定字符，那么函数将返回NULL
      //slash 斜线
      //地址中最后一个/后的就是文件名，前面的是路径
      const char *slash = strrchr(data_, '/');
      if (slash)
      {
        data_ = slash + 1;
        size_ -= static_cast<int>(data_ - arr);
      }
    }

    explicit SourceFile(const char *filename) : data_(filename)
    {
      const char *slash = strrchr(filename, '/');
      if (slash)
      {
        data_ = slash + 1;
      }
      size_ = static_cast<int>(strlen(data_));
    }

    const char *data_;
    int size_;
  };

  //file的第line行输出了等级为level的日志
  Logger(SourceFile file, int line);
  Logger(SourceFile file, int line, LogLevel level);
  Logger(SourceFile file, int line, LogLevel level, const char *func);
  Logger(SourceFile file, int line, bool toAbort); //toAbort,是否终止程序运行
  ~Logger();

  LogStream &stream() { return impl_.stream_; }

  static LogLevel logLevel();
  static void setLogLevel(LogLevel level);

  typedef void (*OutputFunc)(const char *msg, int len);
  typedef void (*FlushFunc)();
  static void setOutput(OutputFunc);
  static void setFlush(FlushFunc);
  static void setTimeZone(const struct timezone &tz);

private:
  //实际工作都交割Imple类去完成
  //Imple代表一条日志消息
  //作用是为了更好的封装
  class Impl
  {
  public:
    typedef Logger::LogLevel LogLevel;
    Impl(LogLevel level, int old_errno, const SourceFile &file, int line);
    void formatTime();
    void finish();

    Timestamp time_;
    LogStream stream_;
    LogLevel level_;
    int line_;
    SourceFile basename_;
  };

  Impl impl_;
};

//大于等于g_logLevel级别的日志才输出
extern Logger::LogLevel g_logLevel;

inline Logger::LogLevel Logger::logLevel()
{
  return g_logLevel;
}

//下列宏先建立一个Logger对象，再返回其stream()成员函数，即一个LogStream对象
//把日志记录在LogStream中
//__LINE__：在源代码中插入当前源代码行号；
//__FILE__：在源文件中插入当前源文件名；
//__func__指示当前函数名
#define LOG_TRACE                                          \
  if (hxmmxh::Logger::logLevel() <= hxmmxh::Logger::TRACE) \
  hxmmxh::Logger(__FILE__, __LINE__, hxmmxh::Logger::TRACE, __func__).stream()
#define LOG_DEBUG                                          \
  if (hxmmxh::Logger::logLevel() <= hxmmxh::Logger::DEBUG) \
  hxmmxh::Logger(__FILE__, __LINE__, hxmmxh::Logger::DEBUG, __func__).stream()
#define LOG_INFO                                          \
  if (hxmmxh::Logger::logLevel() <= hxmmxh::Logger::INFO) \
  hxmmxh::Logger(__FILE__, __LINE__).stream()
#define LOG_WARN hxmmxh::Logger(__FILE__, __LINE__, hxmmxh::Logger::WARN).stream()
#define LOG_ERROR hxmmxh::Logger(__FILE__, __LINE__, hxmmxh::Logger::ERROR).stream()
#define LOG_FATAL hxmmxh::Logger(__FILE__, __LINE__, hxmmxh::Logger::FATAL).stream()

#define LOG_SYSERR hxmmxh::Logger(__FILE__, __LINE__, false).stream()  //系统错误
#define LOG_SYSFATAL hxmmxh::Logger(__FILE__, __LINE__, true).stream() //系统严重错误

const char *strerror_tl(int savedErrno);

//检查输入是否为空
// #是“字符串化”的意思。出现在宏定义中的#是把跟在后面的参数转换成一个字符串
//##是一个连接符号，用于把参数连在一起
//邻近字符串连接 "'" #val "' Must be non NULL"会合并成一个大字符串
#define CHECK_NOTNULL(val) \
  hxmmxh::CheckNotNull(__FILE__, __LINE__, "'" #val "' Must be non NULL", (val))

template <typename T>
T *CheckNotNull(Logger::SourceFile file, int line, const char *names, T *ptr)
{
  if (ptr == NULL)
  {
    Logger(file, line, Logger::FATAL).stream() << names;
  }
  return ptr;
}

} // namespace hxmmxh

#endif
