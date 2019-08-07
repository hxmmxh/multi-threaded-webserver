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
void removeConnection(EventLoop *loop, const TcpConnectionPtr &conn)
{
    loop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}

void removeConnector(const ConnectorPtr &connector)
{
    //connector->
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