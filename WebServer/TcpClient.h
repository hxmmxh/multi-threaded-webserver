#ifndef HXMMXH_TCPCLIENT_H
#define HXMMXH_TCPCLIENT_H

#include "TcpConnection.h"

#include <mutex>

namespace hxmmxh
{
class Connector;
typedef std::shared_ptr<Connector> ConnectorPtr;

class TcpClient
{
    TcpClient(EventLoop *loop,
              const InetAddress &serverAddr,
              const string &nameArg);
    ~TcpClient();

    void connect();
    void disconnect();
    void stop();

 private:
  /// Not thread safe, but in loop
  void newConnection(int sockfd);
  /// Not thread safe, but in loop
  void removeConnection(const TcpConnectionPtr& conn);

  EventLoop* loop_;
  ConnectorPtr connector_; // avoid revealing Connector
  const string name_;
  ConnectionCallback connectionCallback_;
  MessageCallback messageCallback_;
  WriteCompleteCallback writeCompleteCallback_;
  bool retry_;   // atomic
  bool connect_; // atomic
  // always in loop thread
  int nextConnId_;
  mutable std::mutex mutex_;
  TcpConnectionPtr connection_ ;
}
} // namespace hxmmxh