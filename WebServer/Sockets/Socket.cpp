
#include "Socket.h"
#include "InetAddress.h"
#include "SocketsOps.h"
#include "../../Log/Logging.h"

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h> // snprintf

using namespace hxmmxh;

Socket::~Socket()
{
  sockets::close(sockfd_);
}

bool Socket::getTcpInfo(struct tcp_info *tcpi) const
{
  socklen_t len = sizeof(*tcpi);
  memset(tcpi, 0, len);
  return ::getsockopt(sockfd_, SOL_TCP, TCP_INFO, tcpi, &len) == 0;
}

bool Socket::getTcpInfoString(char *buf, int len) const
{
  struct tcp_info tcpi;
  bool ok = getTcpInfo(&tcpi);
  if (ok)
  {
    snprintf(buf, len, "unrecovered=%u "
                       "rto=%u ato=%u snd_mss=%u rcv_mss=%u "
                       "lost=%u retrans=%u rtt=%u rttvar=%u "
                       "sshthresh=%u cwnd=%u total_retrans=%u",
             tcpi.tcpi_retransmits,    // 超时重传的次数
             tcpi.tcpi_rto,            // 超时时间，单位为微秒
             tcpi.tcpi_ato,            // 延时确认的估值，单位为微秒
             tcpi.tcpi_snd_mss,        //本端的MSS(maximum segment size)
             tcpi.tcpi_rcv_mss,        //对端的MSS(最大分节大小)
             tcpi.tcpi_lost,           // 丢失且未恢复的数据段数
             tcpi.tcpi_retrans,        // 重传且未确认的数据段数
             tcpi.tcpi_rtt,            // 平滑的RTT(往返时间)，单位为微秒
             tcpi.tcpi_rttvar,         // 四分之一mdev，单位为微秒
             tcpi.tcpi_snd_ssthresh,   //慢启动阈值
             tcpi.tcpi_snd_cwnd,       //拥塞窗口
             tcpi.tcpi_total_retrans); // 本连接的总重传个数
  }
  return ok;
}

void Socket::bindAddress(const InetAddress& addr)
{
  sockets::bindOrDie(sockfd_, addr.getSockAddr());
}

void Socket::listen()
{
  sockets::listenOrDie(sockfd_);
}

int Socket::accept(InetAddress* peeraddr)
{
  struct sockaddr_in6 addr;
  memset(&addr,0, sizeof addr);
  int connfd = sockets::accept(sockfd_, &addr);
  //记录对端的地址
  if (connfd >= 0)
  {
    peeraddr->setSockAddrInet6(addr);
  }
  return connfd;
}

void Socket::shutdownWrite()
{
  sockets::shutdownWrite(sockfd_);
}

//禁用Nagle算法
//在未确认数据发送的时候让发送器把数据送到缓存里。任何数据随后继续直到得到明显的数据确认或者直到攒到了一定数量的数据了再发包
void Socket::setTcpNoDelay(bool on)
{
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY,
               &optval, static_cast<socklen_t>(sizeof optval));
}

//允许重用本次地址
//UNP P165
void Socket::setReuseAddr(bool on)
{
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR,
               &optval, static_cast<socklen_t>(sizeof optval));
}

void Socket::setReusePort(bool on)
{
  int optval = on ? 1 : 0;
  int ret = ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT,
                         &optval, static_cast<socklen_t>(sizeof optval));
  if (ret < 0 && on)
  {
    LOG_SYSERR << "SO_REUSEPORT failed.";
  }
}

//UNP P157 保持存活探测分节
void Socket::setKeepAlive(bool on)
{
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE,
               &optval, static_cast<socklen_t>(sizeof optval));
}

