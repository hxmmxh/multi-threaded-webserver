#ifndef HXMMXH_THREAD_THREAD_H
#define HXMMXH_THREAD_THREAD_H

#include "CountDownLatch.h"

#include <atomic>
#include <functional>
#include <memory>
#include <pthread.h>
#include <string>
namespace hxmmxh
{
class Thread
{
public:
    // 一个函数类型，没有参数也没有返回值
    typedef std::function<void()> ThreadFunc;
    //第一个参数为线程中要运行的函数，第二个参数为线程的名字
    explicit Thread(ThreadFunc, const std::string &name = std::string());
    ~Thread();
    //开始执行函数
    void start();
    //类似::join()
    int join();
    bool started() const { return started_; }
    //返回线程标识符
    pthread_t pthreadId() const { return pthreadId_; }
    //返回线程ID
    pid_t tid() const { return tid_; }
    //返回线程名
    const std::string &name() const { return name_; }
    //返回创造的线程数
    static int numCreated() { return numCreated_; }

private:
    void setDefaultName();//创建一个默认的线程名字

    bool started_;//是否已经开始
    bool joined_;//是否是joined,

    pthread_t pthreadId_; // pthread中的线程ID
    pid_t tid_;// 系统的线程唯一ID
    ThreadFunc func_;// 线程中要运行的函数
    std::string name_;// 线程的名字
    CountDownLatch latch_;// 计数器
    
    static std::atomic_int32_t numCreated_;// 用来记录创建的线程数
};
}
#endif