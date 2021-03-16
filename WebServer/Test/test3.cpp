#include "TcpServer.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include <stdio.h>
#include <unistd.h>

using namespace hxmmxh;

void onConnection(const TcpConnectionPtr &conn)
{
    if (conn->connected())
    {
        printf("onConnection(): tid=%d new connection [%s] from %s\n",
               CurrentThread::tid(),
               conn->name().c_str(),
               conn->peerAddress().toIpPort().c_str());
    }
    else
    {
        printf("onConnection(): tid=%d connection [%s] is down\n",
               CurrentThread::tid(),
               conn->name().c_str());
    }
}

void onMessage(const TcpConnectionPtr &conn,
               Buffer *buf,
               Timestamp receiveTime)
{
    printf("onMessage(): tid=%d received %zd bytes from connection [%s] at %s\n",
           CurrentThread::tid(),
           buf->readableBytes(),
           conn->name().c_str(),
           receiveTime.toFormattedString().c_str());

    conn->send(buf->retrieveAllAsString());
}

int main(int argc, char *argv[])
{
    printf("main(): pid = %d\n", getpid());

    InetAddress listenAddr(9981);
    EventLoop loop;

    TcpServer server(&loop, listenAddr,"test3",TcpServer::kReusePort);
    server.setConnectionCallback(onConnection);
    server.setMessageCallback(onMessage);
    if (argc > 1)
    {
        server.setThreadNum(atoi(argv[1]));
    }
    server.start();

    loop.loop();
}
