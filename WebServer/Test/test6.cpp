#include "../Connector.h"
#include "../Reactor/EventLoop.h"

#include <stdio.h>
#include <memory>

using namespace hxmmxh;

EventLoop *g_loop;

void connectCallback(int sockfd)
{
    printf("connected.\n");
    g_loop->quit();
}

int main(int argc, char *argv[])
{
    EventLoop loop;
    g_loop = &loop;
    InetAddress addr("127.0.0.1", 9981);
    std::shared_ptr<Connector> connector(new Connector(&loop, addr));
    connector->setNewConnectionCallback(connectCallback);
    connector->start();

    loop.loop();
}
