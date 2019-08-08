#include "../Channel.h"
#include "../EventLoop.h"

#include <cstring>
#include <unistd.h>
#include <stdio.h>
#include <sys/timerfd.h>

using namespace hxmmxh;

EventLoop *g_loop;

void timeout(Timestamp receiveTime)
{
    printf("%s Timeout!\n", receiveTime.toFormattedString().c_str());
    g_loop->quit();
}

int main()
{
    printf("%s started\n", Timestamp::now().toFormattedString().c_str());
    EventLoop loop;
    g_loop = &loop;

    int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    Channel channel(&loop, timerfd);
    channel.setReadCallback(timeout);
    channel.enableReading();

    //5秒后到时
    struct itimerspec howlong;
    memset(&howlong, 0,sizeof howlong);
    howlong.it_value.tv_sec = 5;
    ::timerfd_settime(timerfd, 0, &howlong, NULL);

    loop.loop();

    ::close(timerfd);
}
