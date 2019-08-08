#include "../TcpServer.h"
#include "../Reactor/EventLoop.h"
#include "../Sockets/InetAddress.h"
#include <stdio.h>
#include <unistd.h>

std::string message1;
std::string message2;

using namespace hxmmxh;

void onConnection(const TcpConnectionPtr &conn)
{
    if (conn->connected())
    {
        printf("onConnection(): tid=%d new connection [%s] from %s\n",
               CurrentThread::tid(),
               conn->name().c_str(),
               conn->peerAddress().toIpPort().c_str());
        if (!message1.empty())
            conn->send(message1);
        if (!message2.empty())
            conn->send(message2);
        conn->shutdown();
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

    buf->retrieveAll();
}

int main(int argc, char *argv[])
{
    printf("main(): pid = %d\n", getpid());

    int len1 = 100;
    int len2 = 200;

    if (argc > 2)
    {
        len1 = atoi(argv[1]);
        len2 = atoi(argv[2]);
    }

    message1.resize(len1);
    message2.resize(len2);
    std::fill(message1.begin(), message1.end(), 'A');
    std::fill(message2.begin(), message2.end(), 'B');

    InetAddress listenAddr(9981);
    EventLoop loop;

    TcpServer server(&loop, listenAddr,"test4",TcpServer::kReusePort);
    server.setConnectionCallback(onConnection);
    server.setMessageCallback(onMessage);
    if (argc > 3)
    {
        server.setThreadNum(atoi(argv[3]));
    }
    server.start();

    loop.loop();
}
