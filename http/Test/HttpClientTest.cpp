#include "../../WebServer/Reactor/EventLoop.h"
#include "../../WebServer/TcpClient.h"
#include "../../Log/Logging.h"

#include <utility>

#include <stdio.h>
#include <unistd.h>

using namespace hxmmxh;
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

void onConnection(const TcpConnectionPtr &conn)
{
    std::string message1 = "GET /hello HTTP/1.1\r\nConnection: Keep-Alive\r\n\r\n";
    std::string message2 = "GET /hello HTTP/1.0\r\n";
    if (conn->connected())
    {
        printf("onConnection(): new connection [%s] from %s\n",
               conn->name().c_str(),
               conn->peerAddress().toIpPort().c_str());
        conn->send(message1);
        sleep(5);
        conn->send(message2);
    }
    else
    {
        printf("onConnection(): connection [%s] is down\n",
               conn->name().c_str());
    }
}

int main()
{
    EventLoop loop;
    InetAddress serverAddr("127.0.0.1", 8000);
    TcpClient client(&loop, serverAddr, "httpClient");
    client.setConnectionCallback(onConnection);
    client.setMessageCallback(onMessage);
    client.connect();
    loop.loop();
}
