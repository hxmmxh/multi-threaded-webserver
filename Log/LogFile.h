//存储日志的文件

#ifndef HXMMXH_LOGFILE_H
#define HXMMXH_LOGFILE_H

#include "StringPiece.h" //StringArg

#include <mutex>
#include <string>
#include <memory>

namespace hxmmxh
{
using std::string;

namespace FileUtil
{
//为读小于64kB大小的文件设置的缓冲区
class ReadSmallFile
{
public:
    ReadSmallFile(StringArg filename);
    ~ReadSmallFile();

    // return errno
    template <typename String>
    int readToString(int maxSize,//读取的最大字节数
                     String *content,//读取的内容传入content
                     int64_t *fileSize,//读取的文件大小
                     int64_t *modifyTime,//文件最后修改的时间
                     int64_t *createTime);//文件创建时间
    //读最多kBufferSize的字节进缓冲区
    int readToBuffer(int *size);

    const char *buffer() const { return buf_; }

    static const int kBufferSize = 64 * 1024;

private:
    int fd_;
    int err_;
    char buf_[kBufferSize];
};

//读文件内容，出错则返回错误代码errno
template <typename String>
int readFile(StringArg filename,
             int maxSize,
             String *content,
             int64_t *fileSize = NULL,
             int64_t *modifyTime = NULL,
             int64_t *createTime = NULL)
{
    ReadSmallFile file(filename);
    return file.readToString(maxSize, content, fileSize, modifyTime, createTime);
}

//写入文件
class AppendFile
{
public:
    explicit AppendFile(StringArg filename);

    ~AppendFile();

    void append(const char *logline, size_t len);

    void flush();

    off_t writtenBytes() const { return writtenBytes_; }

private:
    size_t write(const char *logline, size_t len);

    FILE *fp_; //要写入的文件
    char buffer_[64 * 1024];//缓冲区大小
    off_t writtenBytes_; //已经写入的字节数
};
} // namespace FileUtil

class LogFile
{
public:
    LogFile(const string &basename,
            //off_t指示文件的偏移量
            off_t rollSize, //文件滚动，超过大小rollsize就换下一个文件
            bool threadSafe = true,
            int flushInterval = 3,   //每隔3秒自动fluah
            int checkEveryN = 1024); //每写入1024次，检查一下
    ~LogFile();

    void append(const char *logline, int len);
    void flush();
    bool rollFile();

private:
    void append_unlocked(const char *logline, int len);

    static string getLogFileName(const string &basename, time_t *now);

    const string basename_;
    const size_t rollSize_;
    const int flushInterval_;
    const int checkEveryN_;

    int count_; //写入的次数

    std::unique_ptr<std::mutex> mutex_;
    time_t startOfPeriod_;//单位为天
    time_t lastRoll_;//单位为秒
    time_t lastFlush_;//单位为秒
    std::unique_ptr<FileUtil::AppendFile> file_;//功能实现转移给AppendFile

    //一天有多少秒
    //默认每过一天换一个日志文件
    const static int kRollPerSeconds_ = 60 * 60 * 24;
};
} // namespace hxmmxh

#endif