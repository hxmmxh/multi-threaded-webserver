#include "TcpServer.h"

#include "Acceptor.h"
#include "EventLoop.h"
#include "Socket.h"

#include <functional>
#include <stdio.h> // snprintf
#include <iostream>
#include <strings.h>

TcpServer::TcpServer(EventLoop *loop, const InetAddress &listenAddr)
    : loop_(loop),
      name_(listenAddr.toHostPort()),
      acceptor_(new Acceptor(loop, listenAddr)),
      started_(false),
      nextConnId_(1)
{
  acceptor_->setNewConnectionCallback(
      std::bind(&TcpServer::newConnection, this, std::placeholders::_1, std::placeholders::_2));
}

TcpServer::~TcpServer()
{
}

void TcpServer::start()
{
  if (!started_)
  {
    started_ = true;
  }

  if (!acceptor_->listenning())
  {
    loop_->runInLoop(
        std::bind(&Acceptor::listen, acceptor_.get()));
  }
}

void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr)
{
  loop_->assertInLoopThread();
  char buf[32];
  snprintf(buf, sizeof buf, "#%d", nextConnId_);
  ++nextConnId_;
  std::string connName = name_ + buf;
#ifdef LOG
  std::cout << "TcpServer::newConnection [" << name_
            << "] - new connection [" << connName
            << "] from " << peerAddr.toHostPort();
#endif
  struct sockaddr_in localaddr;
  bzero(&localaddr, sizeof localaddr);
  socklen_t addrlen = sizeof(localaddr);
  getsockname(sockfd, (struct sockaddr *)(&localaddr), &addrlen);
  InetAddress localAddr(localaddr);
  TcpConnectionPtr conn(
      std::make_shared<TcpConnection>(loop_, connName, sockfd, localAddr, peerAddr));
  connections_[connName] = conn;
  conn->setConnectionCallback(connectionCallback_);
  conn->setMessageCallback(messageCallback_);
  conn->setCloseCallback(
      std::bind(&TcpServer::removeConnection, this, _1));
  conn->connectEstablished();
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
  loop_->assertInLoopThread();
#ifdef LOG
  LOG_INFO << "TcpServer::removeConnection [" << name_
           << "] - connection " << conn->name();
#endif
  size_t n = connections_.erase(conn->name());
  assert(n == 1); (void)n;
  loop_->queueInLoop(
      std::bind(&TcpConnection::connectDestroyed, conn));
}