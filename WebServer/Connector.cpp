#include "Connector.h"
#include "Channel.h"
#include "EventLoop.h"
#include "SocketsOps.h"
#include "Logging.h"

#include <cerrno>

using namespace hxmmxh;

//类的常量静态成员可以在类内初始化
//但也要在类外定义一下，类外定义不能再指定一个初始值
const int Connector::kMaxRetryDelayMs;

Connector::Connector(EventLoop *loop, const InetAddress &serverAddr)
    : loop_(loop),
      serverAddr_(serverAddr),
      connect_(false),
      state_(kDisconnected),
      retryDelayMs_(kInitRetryDelayMs)
{
  LOG_DEBUG << "ctor[" << this << "]";
}

Connector::~Connector()
{
  LOG_DEBUG << "dtor[" << this << "]";
  assert(!channel_);
}

void Connector::start()
{
  connect_ = true;
  loop_->runInLoop(std::bind(&Connector::startInLoop, this));
}

void Connector::startInLoop()
{
  loop_->assertInLoopThread();
  assert(state_ == kDisconnected);
  if (connect_)
  {
    connect();
  }
  else
  {
    LOG_DEBUG << "do not connect";
  }
}

//主动停止
void Connector::stop()
{
  connect_ = false;
  loop_->queueInLoop(std::bind(&Connector::stopInLoop, this));
}

void Connector::stopInLoop()
{
  loop_->assertInLoopThread();
  if (state_ == kConnecting)
  {
    setState(kDisconnected);
    //只是为了移除channel，和下一步close sockfd
    int sockfd = removeAndResetChannel();
    //在stop中connect_ 被设为false;retry仅仅close sockfd，不会重新尝试连接
    retry(sockfd);
  }
}

void Connector::connect()
{
  int sockfd = sockets::createNonblockingOrDie(serverAddr_.family());
  int ret = sockets::connect(sockfd, serverAddr_.getSockAddr());
  // 成功返回0，出错返回-1
  int savedErrno = (ret == 0) ? 0 : errno;
  switch (savedErrno)
  {
    //非阻塞套接字的正常返回
    //对于非阻塞套接字调用connect不会立即完成，通常返回错误-1，错误码是EINPROGRESS，
    //我们应该调用select或者poll等待套接字可写，然后使用getsockopt获取错误值，如果等于0就是连接成功。
  case 0:
  case EINPROGRESS: //表示连接建立已经启动，但是尚未完成。套接字为非阻塞套接字，且连接请求没有立即完成
  case EINTR:       //调用被中断
  case EISCONN:     //已经连接到该套接字
                    //开始监听套接字可写
    connecting(sockfd);
    break;
    //可修正的错误，尝试重连
  case EAGAIN:        //没有足够空闲的本地端口，需要关闭socket再延期重试
  case EADDRINUSE:    //本地地址处于使用状态
  case EADDRNOTAVAIL: //ip地址不存在
  case ECONNREFUSED:  //远程地址并没有处于监听状态
  case ENETUNREACH:   //网络不可到达
    retry(sockfd);
    break;
    //无法挽回的错误，关闭套接字
  case EACCES:       //用户试图在套接字广播标志没有设置的情况下连接广播地址或由于防火墙策略导致连接失败。
  case EPERM:        //用户试图在套接字广播标志没有设置的情况下连接广播地址或由于防火墙策略导致连接失败。
  case EAFNOSUPPORT: //参数serv_add中的地址非合法地址
  case EALREADY:     //套接字为非阻塞套接字，并且原来的连接请求还未完成
  case EBADF:        //非法的文件描述符
  case EFAULT:       //指向套接字结构体的地址非法
  case ENOTSOCK:     //文件描述符不与套接字相关
    LOG_SYSERR << "connect error in Connector::startInLoop " << savedErrno;
    sockets::close(sockfd);
    break;

  default:
    LOG_SYSERR << "Unexpected error in Connector::startInLoop " << savedErrno;
    sockets::close(sockfd);
    // connectErrorCallback_();
    break;
  }
}

//和start的区别是只能所属的loop线程中调用
//并把State和重试间隔恢复到初始状态
void Connector::restart()
{
  loop_->assertInLoopThread();
  setState(kDisconnected);
  retryDelayMs_ = kInitRetryDelayMs;
  connect_ = true;
  //前面已保证在本线程，所以可以直接调用startInLoop
  startInLoop();
}

//在Channel中监听套接字变得可写，可写则表明连接建立完毕
void Connector::connecting(int sockfd)
{
  setState(kConnecting);
  assert(!channel_);
  //unqiue_ptr::reset，释放原对象，重新绑定一个对象
  channel_.reset(new Channel(loop_, sockfd));
  channel_->setWriteCallback(
      std::bind(&Connector::handleWrite, this));
  channel_->setErrorCallback(
      std::bind(&Connector::handleError, this));
  channel_->enableWriting();
}

//释放Channel，并返回它监听的fd
int Connector::removeAndResetChannel()
{
  channel_->disableAll();
  channel_->remove();
  int sockfd = channel_->fd();
  // Can't reset channel_ here, because we are inside Channel::handleEvent
  loop_->queueInLoop(std::bind(&Connector::resetChannel, this)); // FIXME: unsafe
  return sockfd;
}

void Connector::resetChannel()
{
  //释放所指向的对象
  channel_.reset();
}

void Connector::handleWrite()
{
  LOG_TRACE << "Connector::handleWrite " << state_;

  if (state_ == kConnecting)
  {
    int sockfd = removeAndResetChannel();
    int err = sockets::getSocketError(sockfd);
    if (err)
    {
      LOG_WARN << "Connector::handleWrite - SO_ERROR = "
               << err << " " << strerror_tl(err);
      retry(sockfd);
    }
    //发生了自连接，断开重试
    else if (sockets::isSelfConnect(sockfd))
    {
      LOG_WARN << "Connector::handleWrite - Self connect";
      retry(sockfd);
    }
    //正常建立了连接
    else
    {
      setState(kConnected);
      if (connect_)
      {
        newConnectionCallback_(sockfd);
      }
      else
      {
        sockets::close(sockfd);
      }
    }
  }
  else
  {
    // what happened?
    assert(state_ == kDisconnected);
  }
}

void Connector::handleError()
{
  LOG_ERROR << "Connector::handleError state=" << state_;
  //如果是在连接过程中出现了错误
  if (state_ == kConnecting)
  {
    int sockfd = removeAndResetChannel();
    int err = sockets::getSocketError(sockfd);
    LOG_TRACE << "SO_ERROR = " << err << " " << strerror_tl(err);
    retry(sockfd);
  }
}

//每次调用retry的时候，channel_都是一个空指针，不用管理旧channel_的析构
//channel_在connecting中被构造，在handleWrite和handleError中被移除
//retry会在connect中被调用，这时channel_未被建立
//或是在在stopInloop,handleWrite和handleError中被调用，此时channel_已经被析构
void Connector::retry(int sockfd)
{
  //重新尝试需要先关闭旧的文件描述符
  sockets::close(sockfd);
  setState(kDisconnected);
  if (connect_)
  {
    LOG_INFO << "Connector::retry - Retry connecting to " << serverAddr_.toIpPort()
             << " in " << retryDelayMs_ << " milliseconds. ";
    //runAfter的第一个参数单位是秒
    // shared_from_this()返回该对象的一个智能指针，防止它在重试的等待过程中被析构
    loop_->runAfter(retryDelayMs_ / 1000.0,
                    std::bind(&Connector::startInLoop, shared_from_this()));
    //逐步增加重试的间隔
    retryDelayMs_ = std::min(retryDelayMs_ * 2, kMaxRetryDelayMs);
  }
  else
  {
    LOG_DEBUG << "do not connect";
  }
}
