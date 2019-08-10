#include "../HttpParse.h"
#include "../HttpRequest.h"
#include "../../WebServer/Buffer.h"

using namespace std;
using namespace hxmmxh;
static int test_count = 0;
static int test_pass = 0;

#define CHECKEQUAL(expect, actual)                                                            \
    do                                                                                        \
    {                                                                                         \
        test_count++;                                                                         \
        if ((expect) == (actual))                                                             \
            test_pass++;                                                                      \
        else                                                                                  \
        {                                                                                     \
            std::cout << "line: " << __LINE__ << "error:   " << #expect << " != " << #actual; \
            std::cout << "    " #expect << " = " << expect << std::endl;                      \
        }                                                                                     \
    } while (0)


void testParseRequestAllInOne()
{
  HttpParse context;
  Buffer input;
  input.append("GET /index.html HTTP/1.1\r\n"
       "Host: www.ustc.edu.cn\r\n"
       "\r\n");

  CHECKEQUAL(context.parseRequest(&input, Timestamp::now()),true);
  CHECKEQUAL(context.success(),true);
  const HttpRequest& request = context.request();
  CHECKEQUAL(request.method(), HttpRequest::Get);
  CHECKEQUAL(request.path(), string("/index.html"));
  CHECKEQUAL(request.getVersion(), HttpRequest::Http11);
  CHECKEQUAL(request.getHeader("Host"), string("www.ustc.edu.cn"));
  CHECKEQUAL(request.getHeader("User-Agent"), string(""));
}

void testParseRequestEmptyHeaderValue()
{
HttpParse context;
  Buffer input;
  input.append("GET /index.html HTTP/1.1\r\n"
       "Host: www.chenshuo.com\r\n"
       "User-Agent:\r\n"
       "Accept-Encoding: \r\n"
       "\r\n");

  CHECKEQUAL(context.parseRequest(&input, Timestamp::now()));
  CHECKEQUAL(context.gotAll());
  const HttpRequest& request = context.request();
  CHECKEQUAL(request.method(), HttpRequest::kGet);
  CHECKEQUAL(request.path(), string("/index.html"));
  CHECKEQUAL(request.getVersion(), HttpRequest::kHttp11);
  CHECKEQUAL(request.getHeader("Host"), string("www.chenshuo.com"));
  CHECKEQUAL(request.getHeader("User-Agent"), string(""));
  CHECKEQUAL(request.getHeader("Accept-Encoding"), string(""));
}

int main()
{
    void testParseRequestAllInOne();
    void testParseRequestEmptyHeaderValue();
    std::cout << test_pass << "/" << test_count << " passed\n";
}
