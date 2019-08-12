#include "../HttpServer.h"
#include "../../WebServer/Reactor/EventLoop.h"
#include "../../Log/Logging.h"

#include <signal.h>

using namespace hxmmxh;

int main(int argc, char *argv[])
{
    ::signal(EPIPE, SIG_IGN);
    Logger::setLogLevel(Logger::WARN);
    EventLoop loop;
    HttpServer server(&loop, InetAddress(8000), "dummy");
    if (argc > 1)
    {
        server.setThreadNum(atoi(argv[1]));
    }
    server.start();
    loop.loop();
}