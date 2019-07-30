//使用双缓冲技术实现异步日志

#ifndef HXMMXH_ASYNCLOGGINGDOUBLEBUFFERING_H
#define HXMMXH_ASYNCLOGGINGDOUBLEBUFFERING_H

#include "LogStream.h"
#include "../Thread/ThreadPool.h"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <memory>
#include <thread>
#include <vector>

namespace hxmmxh
{
class AsyncLoggingDoubleBuffering 
{
public:
    AsyncLoggingDoubleBuffering(const string &basename,
                                size_t rollSize,
                                int flushInterval = 3);

    //前端和后端的接口
    //logline代表一条完整的日志消息
    void append(const char *logline, int len);

    void start()
    {
        running_ = true;
        thread_=std::thread(std::bind(&AsyncLoggingDoubleBuffering::threadFunc, this));
        latch_.wait();//等待直到计数器为0
    }

    void stop() 
    {
        running_ = false;
        cond_.notify_one();
        thread_.join();
    }

private:
    void threadFunc();

    typedef detail::FixedBuffer<detail::kLargeBuffer> Buffer;
    typedef std::unique_ptr<Buffer> BufferPtr;
    typedef std::vector<BufferPtr> BufferVector;

    const int flushInterval_; //每隔flushInterval_秒进行一个交换写入操作
    std::atomic<bool> running_;
    const std::string basename_;
    size_t rollSize_;
    std::thread thread_;
    CountDownLatch latch_; //计数器
    std::mutex mutex_;
    std::condition_variable cond_;
    BufferPtr currentBuffer_; //当前缓冲
    BufferPtr nextBuffer_;    //预备缓冲
    BufferVector buffers_;    //待后端写入文件的缓冲块
};
} // namespace hxmmxh

#endif