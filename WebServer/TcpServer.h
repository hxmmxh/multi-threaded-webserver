#ifndef HXMMXH_TCPSERVER_H
#define HXMMXH_TCPSERVER_H

#include "TcpConnection.h"

#include <atomic>
#include <functional>
#include <map>

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
              const string &nameArg,
              Option option = kNoReusePort);
    ~TcpServer();

    const string &ipPort() const { return ipPort_; }
    const string &name() const { return name_; }
    EventLoop *getLoop() const { return loop_; }

    void setThreadNum(int numThreads);
    void setThreadInitCallback(const ThreadInitCallback &cb)
    {
        threadInitCallback_ = cb;
    }
    std::shared_ptr<EventLoopThreadPool> threadPool()
    {
        return threadPool_;
    }

    void start();
    void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb) { writeCompleteCallback_ = cb; }

private:
    void newConnection(int sockfd, const InetAddress &peerAddr);
    void removeConnection(const TcpConnectionPtr &conn);
    void removeConnectionInLoop(const TcpConnectionPtr &conn);

    typedef std::map<std::string, TcpConnectionPtr> ConnectionMap;

    EventLoop *loop_;
    const std::string ipPort_; //监听套接字的地址端口号
    const std::string name_;   //指定的名字
    std::unique_ptr<Acceptor> acceptor_;
    ConnectionCallback connectionCallback_;
    std::shared_ptr<EventLoopThreadPool> threadPool_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    ThreadInitCallback threadInitCallback_;
    std::atomic_int32_t started_;
    int nextConnId_; //接收到额连接的序号
    ConnectionMap connections_;
};
} // namespace hxmmxh
#endif