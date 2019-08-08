#include "../Reactor/EventLoop.h"
#include "../Sockets/InetAddress.h"
#include "../TcpClient.h"

#include "../../Log/Logging.h"

#include <utility>

#include <stdio.h>
#include <unistd.h>

using namespace hxmmxh;

std::string message = "Hello\n";

void onConnection(const TcpConnectionPtr &conn)
{
    if (conn->connected())
    {
        printf("onConnection(): new connection [%s] from %s\n",
               conn->name().c_str(),
               conn->peerAddress().toIpPort().c_str());
        conn->send(message);
    }
    else
    {
        printf("onConnection(): connection [%s] is down\n",
               conn->name().c_str());
    }
}

void onMessage(const TcpConnectionPtr &conn,
               Buffer *buf,
               Timestamp receiveTime)
{
    printf("onMessage(): received %zd bytes from connection [%s] at %s\n",
           buf->readableBytes(),
           conn->name().c_str(),
           receiveTime.toFormattedString().c_str());

    printf("onMessage(): [%s]\n", buf->retrieveAllAsString().c_str());
}

int main()
{
    EventLoop loop;
    InetAddress serverAddr("localhost", 9981);
    TcpClient client(&loop, serverAddr,"test7");

    client.setConnectionCallback(onConnection);
    client.setMessageCallback(onMessage);
    client.enableRetry();
    client.connect();
    loop.loop();
}
