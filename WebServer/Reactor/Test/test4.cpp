
#include "../EventLoop.h"

#include <stdio.h>
#include <unistd.h>
#include <functional>

using namespace std;
using namespace hxmmxh;

int cnt = 0;
EventLoop *g_loop;

void printTid()
{
    printf("pid = %d, tid = %d\n", getpid(), CurrentThread::tid());
    printf("now %s\n", Timestamp::now().toString().c_str());
}

void print(const char *msg)
{
    printf("msg %s %s\n", Timestamp::now().toString().c_str(), msg);
    if (++cnt == 20)
    {
        g_loop->quit();
    }
}

TimerId toCancel;
void cancelSelf()
{
    print("cancelSelf()");
    g_loop->cancel(toCancel);
}

int main()
{
    printTid();
    EventLoop loop;
    g_loop = &loop;

    print("main");
    loop.runAfter(1, bind(print, "once1"));
    loop.runAfter(1.5, bind(print, "once1.5"));
    loop.runAfter(2.5, bind(print, "once2.5"));
    loop.runAfter(3.5, bind(print, "once3.5"));
    TimerId t = loop.runEvery(2, bind(print, "every2"));
    loop.runEvery(3, bind(print, "every3"));
    loop.runAfter(10, bind(&EventLoop::cancel, &loop, t));
    toCancel = loop.runEvery(5, cancelSelf);

    loop.loop();
    print("main loop exits");
    sleep(1);
}
