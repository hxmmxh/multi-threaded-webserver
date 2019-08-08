#include "TcpClient.h"
#include "../Log/Logging.h"
#include "Connector.h"
#include "Reactor/EventLoop.h"
#include "Sockets/SocketsOps.h"

#include <stdio.h>

using namespace hxmmxh;

namespace hxmmxh
{
namespace detail
{
//在客户端移除连接也是调用TcpConnection::connectDestroyed
void removeConnection(EventLoop *loop, const TcpConnectionPtr &conn)
{
    loop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}

void removeConnector(const ConnectorPtr &connector)
{
    //什么也不做
}

} // namespace detail

} // namespace hxmmxh

TcpClient::TcpClient(EventLoop *loop,
                     const InetAddress &serverAddr,
                     const string &nameArg)
    : loop_(loop),
      connector_(new Connector(loop, serverAddr)),
      name_(nameArg),
      connectionCallback_(defaultConnectionCallback),
      messageCallback_(defaultMessageCallback),
      retry_(false),
      connect_(true),
      nextConnId_(1)
{
    //connector在建立连接后要做的事
    connector_->setNewConnectionCallback(
        std::bind(&TcpClient::newConnection, this, _1));
    LOG_INFO << "TcpClient::TcpClient[" << name_
             << "] - connector " << connector_.get();
}

TcpClient::~TcpClient()
{
    LOG_INFO << "TcpClient::~TcpClient[" << name_
             << "] - connector " << get_pointer(connector_);
    TcpConnectionPtr conn;
    bool unique = false;
    {
        MutexLockGuard lock(mutex_);
        //指针为空时，返回的也是false
        //只有指针管理着一个对象，且该对象只被该指针管理时才返回true
        unique = connection_.unique();
        conn = connection_;
    }
    if (conn)
    {
        assert(loop_ == conn->getLoop());
        //注册close函数为移除这个连接，让TcpConnection在收到信号后关闭
        CloseCallback cb = std::bind(&detail::removeConnection, loop_, _1);
        loop_->runInLoop(
            std::bind(&TcpConnection::setCloseCallback, conn, cb));
        if (unique)
        {
            //如果只有一个客户在使用这个Tcpconnection，则可以直接关闭它
            conn->forceClose();
            //forceClose()也是调用handleClose()，最终调用closeCallback_
        }
    }
    //说明还没有建立好的TCP连接，析构connector就行了
    else
    {
        connector_->stop();
        loop_->runAfter(1, std::bind(&detail::removeConnector, connector_));
    }
}

//发起连接，工作都交给Connector
void TcpClient::connect()
{
    LOG_INFO << "TcpClient::connect[" << name_ << "] - connecting to "
             << connector_->serverAddress().toIpPort();
    connect_ = true;
    connector_->start();
}

//断开连接，调用TcpConnection的断开连接，默认用shutdown仅关闭写端
void TcpClient::disconnect()
{
    connect_ = false;
    {
        MutexLockGuard lock(mutex_);
        if (connection_)
        {
            connection_->shutdown();
        }
    }
}

void TcpClient::stop()
{
    connect_ = false;
    connector_->stop();
}

//Connector在建立一个新连接后要调用的函数
void TcpClient::newConnection(int sockfd)
{
    loop_->assertInLoopThread();
    InetAddress peerAddr(sockets::getPeerAddr(sockfd));
    //给这个连接命名
    char buf[32];
    snprintf(buf, sizeof buf, ":%s#%d", peerAddr.toIpPort().c_str(), nextConnId_);
    ++nextConnId_;
    string connName = name_ + buf;

    InetAddress localAddr(sockets::getLocalAddr(sockfd));
    //建立一个TcpConnectionPtr对象
    //客户端不用多线程，不必从线程池中取出loop，用自身的loop就可以了
    TcpConnectionPtr conn(new TcpConnection(loop_,
                                            connName,
                                            sockfd,
                                            localAddr,
                                            peerAddr));

    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(
        std::bind(&TcpClient::removeConnection, this, _1));
    {
        MutexLockGuard lock(mutex_);
        connection_ = conn;
    }
    conn->connectEstablished();
}

void TcpClient::removeConnection(const TcpConnectionPtr& conn)
{
  loop_->assertInLoopThread();
  assert(loop_ == conn->getLoop());

  {
    MutexLockGuard lock(mutex_);
    assert(connection_ == conn);
    connection_.reset();
  }

  loop_->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
  if (retry_ && connect_)
  {
    LOG_INFO << "TcpClient::connect[" << name_ << "] - Reconnecting to "
             << connector_->serverAddress().toIpPort();
    connector_->restart();
  }
}