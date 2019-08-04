#include "InetAddress.h"
#include "Logging.h"
#include "SocketsOps.h"

#include <string.h>
#include <netinet/in.h>
#include <netdb.h> //gethostbyname_r

namespace
{
//INADDR_ANY是ANY，是绑定地址0.0.0.0上的监听, 能收到任意一块网卡的连接；
//INADDR_LOOPBACK, 也就是绑定地址LOOPBAC, 往往是127.0.0.1, 只能收到127.0.0.1上面的连接请求
//127.0.0.1也就是本地的IP地址
const in_addr_t kInaddrAny = INADDR_ANY;
const in_addr_t kInaddrLoopback = INADDR_LOOPBACK;
} // namespace

namespace hxmmxh;

//     /* Structure describing an Internet socket address.  */
//     struct sockaddr_in {
//         sa_family_t    sin_family; /* address family: AF_INET */
//         uint16_t       sin_port;   /* port in network byte order */
//         struct in_addr sin_addr;   /* internet address */
//     };

//     /* Internet address. */
//     typedef uint32_t in_addr_t;
//     struct in_addr {
//         in_addr_t       s_addr;     /* address in network byte order */
//     };

//     struct sockaddr_in6 {
//         sa_family_t     sin6_family;   /* address family: AF_INET6 */
//         uint16_t        sin6_port;     /* port in network byte order */
//         uint32_t        sin6_flowinfo; /* IPv6 flow information */
//         struct in6_addr sin6_addr;     /* IPv6 address */
//         uint32_t        sin6_scope_id; /* IPv6 scope-id */
//     };

static_assert(sizeof(InetAddress) == sizeof(struct sockaddr_in6),
              "InetAddress is same size as sockaddr_in6");
//一个结构成员相对于结构开头的字节偏移量
static_assert(offsetof(sockaddr_in, sin_family) == 0, "sin_family offset 0");
static_assert(offsetof(sockaddr_in6, sin6_family) == 0, "sin6_family offset 0");
static_assert(offsetof(sockaddr_in, sin_port) == 2, "sin_port offset 2");
static_assert(offsetof(sockaddr_in6, sin6_port) == 2, "sin6_port offset 2");

InetAddress::InetAddress(uint16_t port, bool loopbackOnly, bool ipv6)
{
    static_assert(offsetof(InetAddress, addr6_) == 0, "addr6_ offset 0");
    static_assert(offsetof(InetAddress, addr_) == 0, "addr_ offset 0");
    if (ipv6)
    {
        memset(&addr6_, 0, sizeof addr6_);
        addr6_.sin6_family = AF_INET6;
        in6_addr ip = loopbackOnly ? in6addr_loopback : in6addr_any;
        addr6_.sin6_addr = ip;
        addr6_.sin6_port = sockets::hostToNetwork16(port);
    }
    else
    {
        memset(&addr_, 0, sizeof addr6_);
        addr_.sin_family = AF_INET;
        in_addr_t ip = loopbackOnly ? kInaddrLoopback : kInaddrAny;
        addr_.sin_addr.s_addr = sockets::hostToNetwork32(ip);
        addr_.sin_port = sockets::hostToNetwork16(port);
    }
}

InetAddress::InetAddress(StringArg ip, uint16_t port, bool ipv6)
{
    if (ipv6)
    {
        memset(&addr6_, 0, sizeof addr6_);
        sockets::fromIpPort(ip.c_str(), port, &addr6_);
    }
    else
    {
        memset(&addr_, 0, sizeof addr6_);
        sockets::fromIpPort(ip.c_str(), port, &addr_);
    }
}

string InetAddress::toIpPort() const
{
    char buf[64] = "";
    sockets::toIpPort(buf, sizeof buf, getSockAddr());
    return buf;
}

string InetAddress::toIp() const
{
    char buf[64] = "";
    sockets::toIp(buf, sizeof buf, getSockAddr());
    return buf;
}

uint32_t InetAddress::ipNetEndian() const
{
    assert(family() == AF_INET);
    return addr_.sin_addr.s_addr;
}

uint16_t InetAddress::toPort() const
{
    return sockets::networkToHost16(portNetEndian());
}

void InetAddress::setScopeId(uint32_t scope_id)
{
    if (family() == AF_INET6)
    {
        addr6_.sin6_scope_id = scope_id;
    }
}

static __thread char t_resolveBuffer[64 * 1024];

bool InetAddress::resolve(StringArg hostname, InetAddress *out)
{
    assert(out != NULL);
    struct hostent hent;
    struct hostent *he = NULL;
    int herrno = 0;
    memset(&hent, 0, sizeof(hent));

    //UNP P239,270
    //只能返回IPV4地址
    // t_resolveBuffer是缓冲区，用于存放hostent结构，result返回的指针指向该缓冲区的内部
    int ret = gethostbyname_r(hostname.c_str(), &hent, t_resolveBuffer, sizeof t_resolveBuffer, &he, &herrno);
    if (ret == 0 && he != NULL)
    {
        assert(he->h_addrtype == AF_INET && he->h_length == sizeof(uint32_t));
        out->addr_.sin_addr = *reinterpret_cast<struct in_addr *>(he->h_addr);
        return true;
    }
    else
    {
        if (ret)
        {
            LOG_SYSERR << "InetAddress::resolve";
        }
        return false;
    }
}