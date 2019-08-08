#include "../Acceptor.h"
#include "../Reactor/EventLoop.h"
#include "../Sockets/InetAddress.h"
#include "../Sockets/SocketsOps.h"
#include <unistd.h>
#include <stdio.h>

using namespace hxmmxh;

void newConnection1(int sockfd, const InetAddress &peerAddr)
{
    printf("newConnection(): accepted a new connection from %s\n",
           peerAddr.toIpPort().c_str());
    ::write(sockfd, "How are you?\n", 13);
    sockets::close(sockfd);
}

void newConnection2(int sockfd, const InetAddress &peerAddr)
{
    printf("newConnection(): accepted a new connection from %s\n",
           peerAddr.toIpPort().c_str());
    ::write(sockfd, "I am fine,thank you!\n", 21);
    sockets::close(sockfd);
}

int main()
{
    printf("main(): pid = %d\n", getpid());

    InetAddress listenAddr1(9981);
    InetAddress listenAddr2(9982);
    EventLoop loop;

    Acceptor acceptor1(&loop, listenAddr1,1);
    acceptor1.setNewConnectionCallback(newConnection1);
    acceptor1.listen();

    Acceptor acceptor2(&loop, listenAddr2,1);
    acceptor2.setNewConnectionCallback(newConnection2);
    acceptor2.listen();

    loop.loop();
}
