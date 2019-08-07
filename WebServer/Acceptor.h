#ifndef HXMMXH_ACCEPTOR_H
#define HXMMXH_ACCEPTOR_H

#include "Reactor/Channel.h"
#include "Sockets/Socket.h"

#include <functional>

namespace hxmmxh
{

class EventLoop;
class InetAddress;

class Acceptor
{
public:
    typedef std::function<void(int sockfd, const InetAddress &)> NewConnectionCallback;
    Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport);
    ~Acceptor();
    void setNewConnectionCallback(const NewConnectionCallback &cb) { newConnectionCallback_ = cb; }
    
    bool listenning() const { return listenning_; }
    void listen();

private:
    void handleRead();//监听套接字可读时采取的操作，会调用 newConnectionCallback_
    EventLoop *loop_;
    Socket acceptSocket_;//监听套接字
    Channel acceptChannel_;
    NewConnectionCallback newConnectionCallback_;//接受新连接的回调函数
    bool listenning_;//是否开始监听
    int idleFd_;//为了处理EMFILE错误而占用的一个空文件描述符
};

} // namespace hxmmxh
#endif