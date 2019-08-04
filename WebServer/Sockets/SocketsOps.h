#ifndef HXMMXH_SOCKETSOPS_H
#define HXMMXH_SOCKETSOPS_H

#include <arpa/inet.h>

namespace hxmmxh
{
namespace sockets
{

int createNonblockingOrDie(sa_family_t family);

int connect(int sockfd, const struct sockaddr *addr);
void bindOrDie(int sockfd, const struct sockaddr *addr);
void listenOrDie(int sockfd);
int accept(int sockfd, struct sockaddr_in6 *addr);
ssize_t read(int sockfd, void *buf, size_t count);
ssize_t readv(int sockfd, const struct iovec *iov, int iovcnt);
ssize_t write(int sockfd, const void *buf, size_t count);
void close(int sockfd);
void shutdownWrite(int sockfd);


void toIpPort(char *buf, size_t size,
              const struct sockaddr *addr);
void toIp(char *buf, size_t size,
          const struct sockaddr *addr);

void fromIpPort(const char *ip, uint16_t port,
                struct sockaddr_in *addr);
void fromIpPort(const char *ip, uint16_t port,
                struct sockaddr_in6 *addr);

int getSocketError(int sockfd);

//IPV4套接字地址结构转通用套接字地址结构
const struct sockaddr *sockaddr_cast(const struct sockaddr_in *addr);
//IPV6套接字地址结构转通用套接字地址结构
const struct sockaddr *sockaddr_cast(const struct sockaddr_in6 *addr);
//通用套接字地址结构转IPV4套接字地址结构
const struct sockaddr_in *sockaddr_in_cast(const struct sockaddr *addr);
//通用套接字地址结构转IPV6套接字地址结构
const struct sockaddr_in6 *sockaddr_in6_cast(const struct sockaddr *addr);

struct sockaddr_in6 getLocalAddr(int sockfd);
struct sockaddr_in6 getPeerAddr(int sockfd);
bool isSelfConnect(int sockfd);
} // namespace sockets
} // namespace hxmmxh

#endif