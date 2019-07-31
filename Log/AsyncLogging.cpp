#include "AsyncLogging.h"
#include "LogFile.h"
#include "../Time/Timestamp.h"

#include <stdio.h>
#include <utility>//move
#include <chrono>

using namespace hxmmxh;

AsyncLoggingDoubleBuffering::AsyncLoggingDoubleBuffering(const string &basename, size_t rollSize, int flushInterval)
    : flushInterval_(flushInterval),
      running_(false),
      basename_(basename),
      rollSize_(rollSize),
      latch_(1),
      currentBuffer_(new Buffer),
      nextBuffer_(new Buffer),
      buffers_()
{
    currentBuffer_->bzero();
    nextBuffer_->bzero();
    buffers_.reserve(16);
}

void AsyncLoggingDoubleBuffering::append(const char *logline, int len)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (currentBuffer_->avail() > len)
    {
        currentBuffer_->append(logline, len);
    }
    else
    {
        buffers_.push_back(std::move(currentBuffer_));

        if (nextBuffer_)
        {
            currentBuffer_ = std::move(nextBuffer_);
        }
        else
        {
            currentBuffer_.reset(new Buffer); // Rarely happens
        }
        currentBuffer_->append(logline, len);
        cond_.notify_one();
    }
}

void AsyncLoggingDoubleBuffering::threadFunc()
{
  assert(running_ == true);
  latch_.countDown();//计数器减一
  LogFile output(basename_, rollSize_, false);//创建输出文件
  //后端的两个缓冲区
  BufferPtr newBuffer1(new Buffer);
  BufferPtr newBuffer2(new Buffer);
  newBuffer1->bzero();
  newBuffer2->bzero();
  //后端的待写入文件缓冲块数组
  BufferVector buffersToWrite;
  //预留存储空间
  buffersToWrite.reserve(16);
  while (running_)
  {
    //初始状态，后端的两个缓冲块存在且为空，待写入缓冲块数组为空
    assert(newBuffer1 && newBuffer1->length() == 0);
    assert(newBuffer2 && newBuffer2->length() == 0);
    assert(buffersToWrite.empty());

    {
      std::unique_lock<std::mutex> lock(mutex_);
      //前端写频率不高，buffers_为空，则超时三秒后交换，写入文件
      if (buffers_.empty())  // unusual usage!
      {
          //等待超时或收到信号
          cond_.wait_for(lock, std::chrono::seconds(flushInterval_));
      }
      buffers_.push_back(std::move(currentBuffer_));
      currentBuffer_ = std::move(newBuffer1);
      buffersToWrite.swap(buffers_);
      if (!nextBuffer_)
      {
        nextBuffer_ = std::move(newBuffer2);
      }
    }

    assert(!buffersToWrite.empty());

    //写入的buffer过多，可能出现了问题
    //处理日志堆积，直接丢弃多余的日志buffer
    if (buffersToWrite.size() > 25)
    {
      char buf[256];
      snprintf(buf, sizeof buf, "Dropped log messages at %s, %zd larger buffers\n",
               Timestamp::now().toFormattedString().c_str(),
               buffersToWrite.size()-2);
      fputs(buf, stderr);
      output.append(buf, static_cast<int>(strlen(buf)));
      buffersToWrite.erase(buffersToWrite.begin()+2, buffersToWrite.end());
    }

    //写入LOG文件
    for (const auto& buffer : buffersToWrite)
    {
      output.append(buffer->data(), buffer->length());
    }

    //正常情况只要两个元素空间
    if (buffersToWrite.size() > 2)
    {
      buffersToWrite.resize(2);
    }

    if (!newBuffer1)
    {
      assert(!buffersToWrite.empty());
      newBuffer1 = std::move(buffersToWrite.back());
      buffersToWrite.pop_back();
      newBuffer1->reset();
    }

    if (!newBuffer2)
    {
      assert(!buffersToWrite.empty());
      newBuffer2 = std::move(buffersToWrite.back());
      buffersToWrite.pop_back();
      newBuffer2->reset();
    }

    buffersToWrite.clear();
    output.flush();
  }
  output.flush();
}