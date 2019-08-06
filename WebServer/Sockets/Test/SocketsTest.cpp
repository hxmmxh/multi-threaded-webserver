#include "../InetAddress.h"
#include "../../../Log/Logging.h"

#include <iostream>
#include <string>

using namespace hxmmxh;
using namespace std;

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

void testInetAddress()
{
    InetAddress addr0(1234);
    CHECKEQUAL(addr0.toIp(), string("0.0.0.0"));
    CHECKEQUAL(addr0.toIpPort(), string("0.0.0.0:1234"));
    CHECKEQUAL(addr0.toPort(), 1234);

    InetAddress addr1(4321, true);
    CHECKEQUAL(addr1.toIp(), string("127.0.0.1"));
    CHECKEQUAL(addr1.toIpPort(), string("127.0.0.1:4321"));
    CHECKEQUAL(addr1.toPort(), 4321);

    InetAddress addr2("1.2.3.4", 8888);
    CHECKEQUAL(addr2.toIp(), string("1.2.3.4"));
    CHECKEQUAL(addr2.toIpPort(), string("1.2.3.4:8888"));
    CHECKEQUAL(addr2.toPort(), 8888);

    InetAddress addr3("255.254.253.252", 65535);
    CHECKEQUAL(addr3.toIp(), string("255.254.253.252"));
    CHECKEQUAL(addr3.toIpPort(), string("255.254.253.252:65535"));
    CHECKEQUAL(addr3.toPort(), 65535);
}

void testInetAddressResolve()
{
    InetAddress addr(80);
    if (InetAddress::resolve("google.com", &addr))
    {
        std::cout<< "google.com resolved to " << addr.toIpPort();
    }
    else
    {
        std::cout << "Unable to resolve google.com\n";
    }
}

int main()
{
    testInetAddress();
    testInetAddressResolve();
    std::cout << test_pass << "/" << test_count << " passed\n";
}