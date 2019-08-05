#include "../LogFile.h"
#include "../Logging.h"
#include "../../Thread/ThreadPool.h"

#include <stdio.h>
#include <unistd.h>
#include <iostream>

using namespace std;
using namespace hxmmxh;

int g_total;//日志的总字节数
FILE *g_file;//存储日志的普通文件
unique_ptr<LogFile> g_logFile;//通过LogFile存储日志内容

void dummyOutput(const char *msg, int len)
{
    g_total += len;
    if (g_file)
    {
        fwrite(msg, 1, len, g_file);
    }
    else if (g_logFile)
    {
        g_logFile->append(msg, len);
    }
}

void bench(const char *type)
{
    //通过dummyOutput输出日志内容
    Logger::setOutput(dummyOutput);
    Timestamp start(Timestamp::now());
    g_total = 0;
    int n = 1000 * 1000;
    const bool kLongLog = false;
    string empty = " ";
    string longStr(3000, 'X');
    longStr += " ";
    for (int i = 0; i < n; ++i)
    {
        LOG_INFO << "Hello 0123456789"
                 << " abcdefghijklmnopqrstuvwxyz"
                 << (kLongLog ? longStr : empty)
                 << i;
    }
    Timestamp end(Timestamp::now());
    double seconds = timeDifference(end, start);
    printf("%12s: %f seconds, %d bytes, %10.2f msg/s, %.2f MiB/s\n",
           type, seconds, g_total, n / seconds, g_total / seconds / (1024 * 1024));
}

void logInThread()
{
    LOG_INFO << "logInThread";
    usleep(1000);
}

int main()
{
    ThreadPool pool("pool");
    pool.start(5);
    pool.run(logInThread);
    pool.run(logInThread);
    pool.run(logInThread);
    pool.run(logInThread);
    pool.run(logInThread);
    LOG_TRACE << "trace"; //默认Trace和DEBUG不输出
    LOG_DEBUG << "debug";
    LOG_INFO << "Hello";
    LOG_WARN << "World";
    LOG_ERROR << "Error";
    LOG_INFO << sizeof(Logger);
    LOG_INFO << sizeof(LogStream);
    LOG_INFO << sizeof(Fmt);

    sleep(1);
    bench("nop");

    char buffer[64 * 1024];

    g_file = fopen("/dev/null", "w");
    setbuffer(g_file, buffer, sizeof(buffer));
    bench("/dev/null");
    fclose(g_file);

    g_file = fopen("/tmp/log", "w");
    setbuffer(g_file, buffer, sizeof(buffer));
    bench("/tmp/log");
    fclose(g_file);

    g_file = NULL;
    g_logFile.reset(new LogFile("test_log_st", 500 * 1000 * 1000, false));
    bench("test_log_st");

    g_logFile.reset(new LogFile("test_log_mt", 500 * 1000 * 1000, true));
    bench("test_log_mt");
    g_logFile.reset();
}
