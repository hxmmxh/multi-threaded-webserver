#ifndef HXMMXH_TCPSERVER_H
#define HXMMXH_TCPSERVER_H

#include "TcpConnection.h"

#include <atomic>
#include <functional>
#include <map>
#include <string>

namespace hxmmxh
{

class Acceptor;
class EventLoop;
class EventLoopThreadPool;

class TcpServer
{
public:
    typedef std::function<void(EventLoop *)> ThreadInitCallback;
    enum Option
    {
        kNoReusePort,
        kReusePort,
    };

    TcpServer(EventLoop *loop,
              const InetAddress &listenAddr,
              const std::string &nameArg,
              Option option = kNoReusePort);
    ~TcpServer();

    const std::string &ipPort() const { return ipPort_; }
    const std::string &name() const { return name_; }
    EventLoop *getLoop() const { return loop_; }
    
    // 设置线程池的大小
    void setThreadNum(int numThreads);
    // 设置每个线程初始化的操作
    void setThreadInitCallback(const ThreadInitCallback &cb)
    {
        threadInitCallback_ = cb;
    }
    
    void start(); //开始监听套接字
    //都要传递给TcpConnection类使用
    void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb) { writeCompleteCallback_ = cb; }

private:
    void newConnection(int sockfd, const InetAddress &peerAddr);
    void removeConnection(const TcpConnectionPtr &conn);
    void removeConnectionInLoop(const TcpConnectionPtr &conn);

    //第一个参数存储着连接的名字
    typedef std::map<std::string, TcpConnectionPtr> ConnectionMap;

    EventLoop *loop_;
    const std::string ipPort_; //监听套接字的地址端口号
    const std::string name_;   //指定的名字
    std::unique_ptr<Acceptor> acceptor_;
    std::shared_ptr<EventLoopThreadPool> threadPool_;

    ConnectionCallback connectionCallback_; //TcpConnection建立和移除时调用的函数
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    ThreadInitCallback threadInitCallback_; //线程池每个线程创建时调用的函数

    std::atomic_int32_t started_; //是否开始监听
    int nextConnId_;              //接收到的套接字连接的序号
    ConnectionMap connections_;
};
} // namespace hxmmxh
#endif