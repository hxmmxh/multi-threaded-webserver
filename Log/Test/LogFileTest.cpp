#include "../LogFile.h"
#include "../Logging.h"

#include <memory>
#include <unistd.h>

using namespace hxmmxh;
std::unique_ptr<LogFile> g_logFile;



void outputFunc(const char* msg, int len)
{
  g_logFile->append(msg, len);
}

void flushFunc()
{
  g_logFile->flush();
}

int main(int argc, char* argv[])
{

  //200*1000是指定每个日志文件的最大字节数
  g_logFile.reset(new LogFile("hxmlogtest", 200*1000));
  Logger::setOutput(outputFunc);
  Logger::setFlush(flushFunc);

  string line = "1234567890 abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

  for (int i = 0; i < 10000; ++i)
  {
    LOG_INFO << line << i;
    //把进程挂起一段时间，单位是微秒（百万分之一秒）
    usleep(1000);
  }
}
