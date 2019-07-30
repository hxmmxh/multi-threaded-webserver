#include "LogFile.h"
#include "Logging.h"

#include <cassert> //assert
#include <time.h>
#include <cstdio>  //FILE,setvbuf,fopen
#include <cstring> //strerror_r
#include <errno.h>
#include <fcntl.h>    //open
#include <sys/stat.h> //open
#include <unistd.h>   //gethostname

using namespace hxmmxh;

//a:打开或新建一个文本文件，只允许在文件末尾追写
//e:O_CLOEXEC
//子进程默认是继承父进程打开的所有fd,如果句柄加入了这个设置,在execve替换进程时就会关闭设置这个选项的所有fd.
FileUtil::AppendFile::AppendFile(StringArg filename)
    : fp_(::fopen(filename.c_str(), "ae")), // 'e' for
      writtenBytes_(0)
{
  assert(fp_);
  //设置fp_文件的缓冲区
  //当Buffer类型是Block Buffered时，往该Stream中写入的信息会先暂存在Buffer中，每次实际的写入都是以块(Block)大小为单位写入目标文件
  // _IOFBF表示Fully Buffered
  //int setvbuf(FILE *stream, char *buf, int mode, size_t size);
  ::setvbuf(fp_, buffer_, _IOFBF, sizeof(buffer_));
}

FileUtil::AppendFile::~AppendFile()
{
  ::fclose(fp_);
}

size_t FileUtil::AppendFile::write(const char *logline, size_t len)
{
  //行为和fwrite一样
  //返回确切写入的字节数，可能比传入的字节数要少
  //不加锁流操作,非线程安全
  //do not use locking (theydo not set locks themselves, and do not test for the presence of locksset by others) and hence are thread-unsafe
  return ::fwrite_unlocked(logline, 1, len, fp_);
}

void FileUtil::AppendFile::flush()
{
  //强迫将缓冲区内的数据写回参数stream 指定的文件中。
  ::fflush(fp_);
}

//写入长度为len的logline日志
void FileUtil::AppendFile::append(const char *logline, const size_t len)
{
  size_t n = write(logline, len);
  size_t remain = len - n;
  //如果一次性没有写完
  while (remain > 0)
  {
    //从已经写完的部分继续写
    size_t x = write(logline + n, remain);
    //还是无法写入则出错
    if (x == 0)
    {
      //如果ferror返回值为0（假），表示未出错。如果返回一个非零值，表示出错
      int err = ferror(fp_);
      if (err)
      {
        //strerror_tl定义在Logging文件
        fprintf(stderr, "AppendFile::append() failed %s\n", strerror_tl(err));
      }
      break;
    }
    n += x;
    remain = len - n; // remain -= x
  }

  writtenBytes_ += len;
}

// 不用缓冲区，采用直接IO操作
//只能读写二进制文件，但效率高、速度 快
//以只读方式和O_CLOEXEC方式打开一个文件
FileUtil::ReadSmallFile::ReadSmallFile(StringArg filename)
    : fd_(::open(filename.c_str(), O_RDONLY | O_CLOEXEC)),
      err_(0)
{
  buf_[0] = '\0';
  if (fd_ < 0)
  {
    err_ = errno;
  }
}

FileUtil::ReadSmallFile::~ReadSmallFile()
{
  if (fd_ >= 0)
  {
    ::close(fd_);
  }
}

//返回的是出错号，errno
template <typename String>
int FileUtil::ReadSmallFile::readToString(int maxSize,
                                          String *content,
                                          int64_t *fileSize,
                                          int64_t *modifyTime,
                                          int64_t *createTime)
{
  static_assert(sizeof(off_t) == 8, "_FILE_OFFSET_BITS = 64");
  assert(content != NULL);
  int err = err_;
  if (fd_ >= 0)
  {
    content->clear();

    if (fileSize)
    {
      struct stat statbuf;
      //获取文件状态信息
      if (::fstat(fd_, &statbuf) == 0)
      {
        //测试是否是普通文件
        if (S_ISREG(statbuf.st_mode))
        {
          //文件的总大小，字节为单位
          *fileSize = statbuf.st_size;
          //如果文件大小大于maxsize，也只读maxsize的字节
          content->reserve(static_cast<int>(std::min(static_cast<int64_t>(maxSize), *fileSize)));
        }
        //测试是否是目录
        else if (S_ISDIR(statbuf.st_mode))
        {
          err = EISDIR;
        }
        if (modifyTime)
        {
          //最后修改时间
          *modifyTime = statbuf.st_mtime;
        }
        if (createTime)
        {
          // 最后状态改变时间
          *createTime = statbuf.st_ctime;
        }
      }
      else
      {
        err = errno;
      }
    }
    while (content->size() < static_cast<size_t>(maxSize))
    {
      size_t toRead = std::min(static_cast<size_t>(maxSize) - content->size(), sizeof(buf_));
      //先读入缓冲区buf_中，再读入content中
      ssize_t n = ::read(fd_, buf_, toRead);
      if (n > 0)
      {
        content->append(buf_, n);
      }
      else
      {
        if (n < 0)
        {
          err = errno;
        }
        break;
      }
    }
  }
  return err;
}

int FileUtil::ReadSmallFile::readToBuffer(int *size)
{
  int err = err_;
  if (fd_ >= 0)
  {
    //pread简单来说就是在指定偏移offset位置开始读取count个字节
    //于对多线程读写比较有意义，不会相互影响读写文件时的offset
    //原子操作,不会改变当前文件偏移量
    ssize_t n = ::pread(fd_, buf_, sizeof(buf_) - 1, 0);
    if (n >= 0)
    {
      if (size)
      {
        *size = static_cast<int>(n);
      }
      buf_[n] = '\0';
    }
    else
    {
      err = errno;
    }
  }
  return err;
}

//显示实例化string作为模板参数的readFile
template int FileUtil::readFile(StringArg filename,
                                int maxSize,
                                string *content,
                                int64_t *, int64_t *, int64_t *);

template int FileUtil::ReadSmallFile::readToString(
    int maxSize,
    string *content,
    int64_t *, int64_t *, int64_t *);

LogFile::LogFile(const string &basename,
                 off_t rollSize,
                 bool threadSafe,
                 int flushInterval,
                 int checkEveryN)
    : basename_(basename),
      rollSize_(rollSize),
      flushInterval_(flushInterval),
      checkEveryN_(checkEveryN),
      count_(0),
      mutex_(threadSafe ? new std::mutex() : NULL),
      startOfPeriod_(0),
      lastRoll_(0),
      lastFlush_(0)
{
  //验证basename是文件名，不是路径名
  assert(basename.find('/') == string::npos);
  rollFile(); //新建一个文件
}

LogFile::~LogFile() = default;

void LogFile::append(const char *logline, int len)
{
  if (mutex_)
  {
    std::lock_guard<std::mutex> lock(*mutex_);
    append_unlocked(logline, len);
  }
  else
  {
    append_unlocked(logline, len);
  }
}

//file_是Append类型对象
//LogFile的工作都转交给它完成

void LogFile::append_unlocked(const char *logline, int len)
{
  //写入长度为len的logline日志
  file_->append(logline, len);

  if (file_->writtenBytes() > rollSize_) //文件写满
  {
    rollFile(); //换一个文件
  }
  else
  {
    ++count_; //写入次数加1
    //写入次数达到一定值时进行check工作
    if (count_ >= checkEveryN_)
    {
      count_ = 0;
      //返回自纪元 Epoch（1970-01-01 00:00:00 UTC）起经过的时间，以秒为单位
      time_t now = ::time(NULL);
      //算出日期，如果过了一天，thisPeroid会加1
      time_t thisPeriod_ = now / kRollPerSeconds_ * kRollPerSeconds_;
      //如果过了一天，则日志滚动，换个新文件写入日志
      if (thisPeriod_ != startOfPeriod_)
      {
        rollFile();
      }
      //和上一次flush的间隔超过过了设定的flushInterval_
      else if (now - lastFlush_ > flushInterval_)
      {
        lastFlush_ = now;
        //把缓冲区中内容写入文件中
        file_->flush();
      }
    }
  }
}

void LogFile::flush()
{
  if (mutex_)
  {
    std::lock_guard<std::mutex> lock(*mutex_);
    file_->flush();
  }
  else
  {
    file_->flush();
  }
}

//日志滚动
bool LogFile::rollFile()
{
  time_t now = 0;
  //获取新建日志的文件名
  string filename = getLogFileName(basename_, &now);
  //转换成天数
  time_t start = now / kRollPerSeconds_ * kRollPerSeconds_;

  if (now > lastRoll_)
  {
    lastRoll_ = now;
    lastFlush_ = now;
    startOfPeriod_ = start;
    //换一个文件
    //释放file_目前指向的对象，并将file_指向新构造的对象
    file_.reset(new FileUtil::AppendFile(filename));
    return true;
  }
  return false;
}

//获取日志名
//格式为
string LogFile::getLogFileName(const string &basename, time_t *now)
{
  string filename;
  //
  filename.reserve(basename.size() + 64);
  filename = basename;

  char timebuf[32];
  struct tm tm;
  *now = time(NULL);
  //把UTC秒数转换成时间结构，有时区转换，得到本地时间
  localtime_r(now, &tm);
  strftime(timebuf, sizeof(timebuf), ".%Y%m%d-%H%M%S.", &tm);
  filename += timebuf;

  char hostname[256];
  if (::gethostname(hostname, sizeof(hostname)) == 0)
  {
    hostname[sizeof(hostname) - 1] = '\0';
    filename += hostname;
  }
  else
      filename += "unknownhost";

  char pidbuf[32];
  snprintf(pidbuf, sizeof pidbuf, ".%d", ::getpid());
  filename += pidbuf;

  filename += ".log";

  return filename;
}
