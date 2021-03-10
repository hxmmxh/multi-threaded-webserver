#include "SocketsOps.h"

#include "Logging.h"
#include "InetAddress.h"

#include <errno.h>
#include <stdio.h>  // snprintf
#include <string.h> //memset,memcmp
#include <sys/socket.h>
#include <sys/uio.h>  // readv
#include <unistd.h>

using namespace hxmmxh;

//创建IPV4或IPV6的TCP套接字
int sockets::createNonblockingOrDie(sa_family_t family)
{
    int sockfd = ::socket(family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd < 0)
    {
        LOG_SYSFATAL << "sockets::createNonblockingOrDie";
    }
    return sockfd;
}

//成功返回0<,出错返回-1
int sockets::connect(int sockfd, const struct sockaddr *addr)
{
    return ::connect(sockfd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
}

void sockets::bindOrDie(int sockfd, const struct sockaddr *addr)
{
    int ret = ::bind(sockfd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
    if (ret < 0)
    {
        LOG_SYSFATAL << "sockets::bindOrDie";
    }
}

void sockets::listenOrDie(int sockfd)
{
    int ret = ::listen(sockfd, SOMAXCONN);
    if (ret < 0)
    {
        LOG_SYSFATAL << "sockets::listenOrDie";
    }
}

int sockets::accept(int sockfd, struct sockaddr_in6 *addr)
{
    socklen_t addrlen = static_cast<socklen_t>(sizeof *addr);
    //和accept的不同之处是可以在新开的文件描述符中设置SOCK_NONBLOCK | SOCK_CLOEXEC
    int connfd = ::accept4(sockfd, sockaddr_cast(addr),
                           &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (connfd < 0)
    {
        int savedErrno = errno;
        LOG_SYSERR << "Socket::accept";
        switch (savedErrno)
        {
        case EAGAIN:       //套接口被标记为非阻塞并且没有连接等待接受
        case ECONNABORTED: //一个连接已经中止了。
        case EINTR:        //在一个有效的连接到达之前，本系统调用被信号中断
        case EPROTO:       // 协议错误。
        case EPERM:        //防火墙规则禁止连接
        case EMFILE:       //达到单个进程打开的文件描述上限。
            // 可恢复错误
            errno = savedErrno;
            break;
        case EBADF:      //描述符无效
        case EFAULT:     //参数 addr 不在可写的用户地址空间里。
        case EINVAL:     //套接口不在监听连接，或 addrlen 无效
        case ENFILE:     //达到系统允许打开文件个数的全局上限。
        case ENOBUFS:    //没有足够的自由内存。这通常是指套接口内存分配被限制，而不是指系统内存不足。
        case ENOMEM:     //同ENOBUFS
        case ENOTSOCK:   //描述符是一个文件，不是一个套接字
        case EOPNOTSUPP: //引用的套接口不是 SOCK_STREAM 类型的。
            // 不可恢复错误
            LOG_FATAL << "unexpected error of ::accept " << savedErrno;
            break;
        default:
            LOG_FATAL << "unknown error of ::accept " << savedErrno;
            break;
        }
    }
    return connfd;
}

ssize_t sockets::read(int sockfd, void *buf, size_t count)
{
    return ::read(sockfd, buf, count);
}
//在一次函数调用中读、写多个非连续缓冲区
//详细见Buffer设计与应用
ssize_t sockets::readv(int sockfd, const struct iovec *iov, int iovcnt)
{
    return ::readv(sockfd, iov, iovcnt);
}

ssize_t sockets::write(int sockfd, const void *buf, size_t count)
{
    return ::write(sockfd, buf, count);
}

void sockets::close(int sockfd)
{
    if (::close(sockfd) < 0)
    {
        LOG_SYSERR << "sockets::close";
    }
}

void sockets::shutdownWrite(int sockfd)
{
    if (::shutdown(sockfd, SHUT_WR) < 0)
    {
        LOG_SYSERR << "sockets::shutdownWrite";
    }
}

//套接字地址结构转换函数
const struct sockaddr *sockets::sockaddr_cast(const struct sockaddr_in6 *addr)
{
    return (const struct sockaddr *)(addr);
}

//第54行中用到非const版本
struct sockaddr *sockets::sockaddr_cast(struct sockaddr_in6 *addr)
{
    return (struct sockaddr *)(addr);
}

const struct sockaddr *sockets::sockaddr_cast(const struct sockaddr_in *addr)
{
    return (const struct sockaddr *)(addr);
}

const struct sockaddr_in *sockets::sockaddr_in_cast(const struct sockaddr *addr)
{
    return (const struct sockaddr_in *)(addr);
}

const struct sockaddr_in6 *sockets::sockaddr_in6_cast(const struct sockaddr *addr)
{
    return (const struct sockaddr_in6 *)(addr);
}

//取出addr中保存的Ip地址和端口号，储存在buf中
void sockets::toIpPort(char* buf, size_t size,
                       const struct sockaddr* addr)
{
  toIp(buf,size, addr);
  size_t end = ::strlen(buf);
  //IPV4和IPV6套接字地址结构中存储端口号的位置是一样的
  //见UNP P61
  const struct sockaddr_in* addr4 = sockaddr_in_cast(addr);
  uint16_t port = networkToHost16(addr4->sin_port);
  assert(size > end);
  snprintf(buf+end, size-end, ":%u", port);
}


//取出addr中保存的Ip地址，储存在buf中
void sockets::toIp(char* buf, size_t size,
                   const struct sockaddr* addr)
{
    //IPV4
  if (addr->sa_family == AF_INET)
  {
    assert(size >= INET_ADDRSTRLEN);
    const struct sockaddr_in* addr4 = sockaddr_in_cast(addr);
    //值->字符串
    ::inet_ntop(AF_INET, &addr4->sin_addr, buf, static_cast<socklen_t>(size));
  }
  //IPV6
  else if (addr->sa_family == AF_INET6)
  {
    assert(size >= INET6_ADDRSTRLEN);
    const struct sockaddr_in6* addr6 = sockaddr_in6_cast(addr);
    ::inet_ntop(AF_INET6, &addr6->sin6_addr, buf, static_cast<socklen_t>(size));
  }
}

//把addr中的地址设置为ip,port，IPV4
void sockets::fromIpPort(const char* ip, uint16_t port,
                         struct sockaddr_in* addr)
{
  addr->sin_family = AF_INET;
  addr->sin_port = hostToNetwork16(port);
  //字符串->值
  if (::inet_pton(AF_INET, ip, &addr->sin_addr) <= 0)
  {
    LOG_SYSERR << "sockets::fromIpPort";
  }
}
//IPv6
void sockets::fromIpPort(const char* ip, uint16_t port,
                         struct sockaddr_in6* addr)
{
  addr->sin6_family = AF_INET6;
  addr->sin6_port = hostToNetwork16(port);
  if (::inet_pton(AF_INET6, ip, &addr->sin6_addr) <= 0)
  {
    LOG_SYSERR << "sockets::fromIpPort";
  }
}

//之所以返回 sockaddr_in6,而不是sockaddr_in
//在使用返回的地址时可以先检查sin_family，如果是AF_INET,可以用reinterpret_cast转换成sockaddr_in
struct sockaddr_in6 sockets::getLocalAddr(int sockfd)
{
  struct sockaddr_in6 localaddr;
  memset(&localaddr,0, sizeof (localaddr));
  socklen_t addrlen = static_cast<socklen_t>(sizeof localaddr);
  if (::getsockname(sockfd, sockaddr_cast(&localaddr), &addrlen) < 0)
  {
    LOG_SYSERR << "sockets::getLocalAddr";
  }
  return localaddr;
}

struct sockaddr_in6 sockets::getPeerAddr(int sockfd)
{
  struct sockaddr_in6 peeraddr;
  memset(&peeraddr,0,sizeof(peeraddr));
  socklen_t addrlen = static_cast<socklen_t>(sizeof peeraddr);
  if (::getpeername(sockfd, sockaddr_cast(&peeraddr), &addrlen) < 0)
  {
    LOG_SYSERR << "sockets::getPeerAddr";
  }
  return peeraddr;
}

bool sockets::isSelfConnect(int sockfd)
{
  struct sockaddr_in6 localaddr = getLocalAddr(sockfd);
  struct sockaddr_in6 peeraddr = getPeerAddr(sockfd);
  if (localaddr.sin6_family == AF_INET)
  {
    const struct sockaddr_in* laddr4 = reinterpret_cast<struct sockaddr_in*>(&localaddr);
    const struct sockaddr_in* raddr4 = reinterpret_cast<struct sockaddr_in*>(&peeraddr);
    return laddr4->sin_port == raddr4->sin_port
        && laddr4->sin_addr.s_addr == raddr4->sin_addr.s_addr;
  }
  else if (localaddr.sin6_family == AF_INET6)
  {
    return localaddr.sin6_port == peeraddr.sin6_port
        && memcmp(&localaddr.sin6_addr, &peeraddr.sin6_addr, sizeof localaddr.sin6_addr) == 0;
  }
  else
  {
    return false;
  }
}

int sockets::getSocketError(int sockfd)
{
  int optval;
  socklen_t optlen = static_cast<socklen_t>(sizeof(optval));
  if (::getsockopt(sockfd,SOL_SOCKET,SO_ERROR,&optval,&optlen)<0)
  {
    return errno;
  }
  else
  {
    return optval;
  }
}
