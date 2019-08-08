#ifndef HXMMXH_TCPCLIENT_H
#define HXMMXH_TCPCLIENT_H

#include "TcpConnection.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

namespace hxmmxh
{
class Connector;
typedef std::shared_ptr<Connector> ConnectorPtr;

class TcpClient
{
  public:
  TcpClient(EventLoop *loop,
            const InetAddress &serverAddr,
            const std::string &nameArg);
  ~TcpClient();

  void connect();    //开始建立连接
  void disconnect(); //断开连接(已经连接成功后)
  void stop();       //停止建立连接

  TcpConnectionPtr connection() const
  {
    std::lock_guard<std::mutex> lock(mutex_);
    return connection_;
  }

  EventLoop *getLoop() const { return loop_; }
  bool retry() const { return retry_; }
  void enableRetry() { retry_ = true; }

  const std::string &name() const
  {
    return name_;
  }

  void setConnectionCallback(ConnectionCallback cb)
  {
    connectionCallback_ = std::move(cb);
  }

  void setMessageCallback(MessageCallback cb)
  {
    messageCallback_ = std::move(cb);
  }

  void setWriteCompleteCallback(WriteCompleteCallback cb)
  {
    writeCompleteCallback_ = std::move(cb);
  }

private:
  void newConnection(int sockfd);                      //只能在本线程内调用
  void removeConnection(const TcpConnectionPtr &conn); //只能在本线程内调用

  EventLoop *loop_;
  ConnectorPtr connector_;
  const std::string name_;

  ConnectionCallback connectionCallback_;
  MessageCallback messageCallback_;
  WriteCompleteCallback writeCompleteCallback_;

  std::atomic_bool retry_;
  std::atomic_bool connect_; 
  int nextConnId_;

  mutable std::mutex mutex_;
  TcpConnectionPtr connection_;
};
} // namespace hxmmxh

#endif