#include "../HttpServer.h"
#include "../../WebServer/Reactor/EventLoop.h"

using namespace hxmmxh;

int main()
{
    EventLoop loop;
    HttpServer server(&loop, InetAddress(8000), "dummy");
    if (argc > 1)
    {
        server.setThreadNum(atoi(argv[1]));
    }
    server.start();
    loop.loop();
}