#ifndef HXMMXH_TCPSERVER_H
#define HXMMXH_TCPSERVER_H

#include "TcpConnection.h"

#include <atomic>
#include <map>

namespace hxmmxh
{

class Acceptor;
class EventLoop;
class EventLoopThreadPool;


class TcpServer
{
public:
    TcpServer(EventLoop *loop, const InetAddress &listenAddr);
    ~TcpServer();

    void start();
    void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }

private:
    void newConnection(int sockfd, const InetAddress &peerAddr);
    void removeConnection(const TcpConnectionPtr &conn);

    typedef std::map<std::string, TcpConnectionPtr> ConnectionMap;

    EventLoop *loop_;
    const std::string name_;
    std::unique_ptr<Acceptor> acceptor_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    bool started_;
    int nextConnId_;
    ConnectionMap connections_;
};
} // namespace hxmmxh
#endif