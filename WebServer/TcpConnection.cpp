#include "TcpConnection.h"

#include "Logging.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Socket.h"
#include "SocketsOps.h"

#include <functional>
#include <errno.h>

using namespace hxmmxh;

void hxmmxh::defaultConnectionCallback(const TcpConnectionPtr &conn)
{
    LOG_TRACE << conn->localAddress().toIpPort() << " -> "
              << conn->peerAddress().toIpPort() << " is "
              << (conn->connected() ? "UP" : "DOWN");
}

//默认的信息处理函数：丢弃数据
void hxmmxh::defaultMessageCallback(const TcpConnectionPtr &,
                            Buffer *buf,
                            Timestamp)
{
    //把buf清空，并没有获取数据
    buf->retrieveAll();
}

TcpConnection::TcpConnection(EventLoop *loop,
                             const std::string &nameArg,
                             int sockfd,
                             const InetAddress &localAddr,
                             const InetAddress &peerAddr)
    : loop_(loop),
      name_(nameArg),
      state_(kConnecting), //初始状态，正在建立连接
      reading_(true),
      socket_(new Socket(sockfd)),         //封装这个套接字，传入的是已连接套接字
      channel_(new Channel(loop, sockfd)), //开始监控这个套接字
      localAddr_(localAddr),               //本地地址
      peerAddr_(peerAddr),                 //对端地址
      highWaterMark_(64 * 1024 * 1024)     //高水位阈值
{
    channel_->setReadCallback(
        std::bind(&TcpConnection::handleRead, this, _1));
    channel_->setWriteCallback(
        std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(
        std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(
        std::bind(&TcpConnection::handleError, this));
    LOG_DEBUG << "TcpConnection::ctor[" << name_ << "] at " << this
              << " fd=" << sockfd;
    //设置SO_KEEPALIVE选项，定时发送保存存活探测分节
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
    LOG_DEBUG << "TcpConnection::dtor[" << name_ << "] at " << this
              << " fd=" << channel_->fd()
              << " state=" << stateToString();
    //需要确保此时连接已经断开
    assert(state_ == kDisconnected);
}

//获取Tcp连接信息，保存在tcpi中
bool TcpConnection::getTcpInfo(struct tcp_info *tcpi) const
{
    return socket_->getTcpInfo(tcpi);
}

//已字符串的形式储存TCP连接的信息
string TcpConnection::getTcpInfoString() const
{
    char buf[1024];
    buf[0] = '\0';
    socket_->getTcpInfoString(buf, sizeof buf);
    return buf;
}

//发送数据
void TcpConnection::send(const void *data, int len)
{
    send(StringPiece(static_cast<const char *>(data), len));
}

//发送字符串
void TcpConnection::send(const StringPiece &message)
{
    if (state_ == kConnected)
    {
        if (loop_->isInLoopThread())
        {
            sendInLoop(message);
        }
        else
        {
            void (TcpConnection::*fp)(const StringPiece &message) = &TcpConnection::sendInLoop;
            loop_->runInLoop(
                std::bind(fp, this, message.as_string()));
        }
    }
}

//发送缓冲区中的数据
void TcpConnection::send(Buffer *buf)
{
    if (state_ == kConnected)
    {
        if (loop_->isInLoopThread())
        {
            sendInLoop(buf->peek(), buf->readableBytes());
            buf->retrieveAll();
        }
        else
        {
            void (TcpConnection::*fp)(const StringPiece &message) = &TcpConnection::sendInLoop;
            loop_->runInLoop(
                std::bind(fp, this, buf->retrieveAllAsString()));
        }
    }
}

void TcpConnection::sendInLoop(const StringPiece &message)
{
    sendInLoop(message.data(), message.size());
}

//实际处理数据发送的函数
void TcpConnection::sendInLoop(const void *data, size_t len)
{
    loop_->assertInLoopThread();
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;
    //连接已断开就放弃发送
    if (state_ == kDisconnected)
    {
        LOG_WARN << "disconnected, give up writing";
        return;
    }
    //如果发送缓冲区中已经有待发送的数据，就不能直接写入套接字了，会造成数据乱序，只能把数据先写入缓冲区
    //如果发送缓冲区是空，则可以直接开始写入
    //channel不在监听可写信号，只有缓冲区有数据等待送时，才会监听
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        //第一次往套接字中写数据
        nwrote = sockets::write(channel_->fd(), data, len);
        if (nwrote >= 0)
        {
            remaining = len - nwrote;
            //如果写完，回调writeCompleteCallback_
            if (remaining == 0 && writeCompleteCallback_)
            {
                loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
            }
        }
        else
        {
            nwrote = 0;
            if (errno != EWOULDBLOCK) //在非阻塞模式下调用了阻塞操作，在该操作没有完成就返回这个错误
            {
                LOG_SYSERR << "TcpConnection::sendInLoop";
                //EPIPE ：Broken pipe
                //ECONNRESET：套接字关闭了还往里面写数据
                if (errno == EPIPE || errno == ECONNRESET)
                {
                    faultError = true;
                }
            }
        }
    }

    assert(remaining <= len);
    //把剩余的数据写入缓冲区
    //如果出现了错误，数据就不用再缓冲了，直接丢弃
    if (!faultError && remaining > 0)
    {
        size_t oldLen = outputBuffer_.readableBytes();
        //触发高水位回调，类似ET,只会触发一次，所以还要检测oldLen < highWaterMark_
        if (oldLen + remaining >= highWaterMark_ && oldLen < highWaterMark_ && highWaterMarkCallback_)
        {
            loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
        }
        //把剩余的数据放入缓冲区，开始关注可写事件，在handlderwrite中发送剩下的数据
        outputBuffer_.append(static_cast<const char *>(data) + nwrote, remaining);
        if (!channel_->isWriting())
        {
            channel_->enableWriting();
        }
    }
}

//只关闭写端，依旧可以接收到数据
void TcpConnection::shutdown()
{
    if (state_ == kConnected)
    {
        setState(kDisconnecting);
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::shutdownInLoop()
{
    loop_->assertInLoopThread();
    //只有不在等待可写信号时才能关闭，防止丢失数据
    if (!channel_->isWriting())
    {
        //shutdown(sockfd, SHUT_WR)
        socket_->shutdownWrite();
    }
}

//强行关闭读写两端
void TcpConnection::forceClose()
{
    if (state_ == kConnected || state_ == kDisconnecting)
    {
        setState(kDisconnecting);
        loop_->queueInLoop(std::bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
    }
}

void TcpConnection::forceCloseWithDelay(double seconds)
{
    if (state_ == kConnected || state_ == kDisconnecting)
    {
        setState(kDisconnecting);
        loop_->runAfter(
            seconds,
            std::bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
    }
}

void TcpConnection::forceCloseInLoop()
{
    loop_->assertInLoopThread();
    if (state_ == kConnected || state_ == kDisconnecting)
    {
        handleClose();
    }
}

const char *TcpConnection::stateToString() const
{
    switch (state_)
    {
    case kDisconnected:
        return "kDisconnected";
    case kConnecting:
        return "kConnecting";
    case kConnected:
        return "kConnected";
    case kDisconnecting:
        return "kDisconnecting";
    default:
        return "unknown state";
    }
}

void TcpConnection::setTcpNoDelay(bool on)
{
    //::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY,&optval, static_cast<socklen_t>(sizeof optval));
    socket_->setTcpNoDelay(on);
}

void TcpConnection::startRead()
{
    loop_->runInLoop(std::bind(&TcpConnection::startReadInLoop, this));
}

void TcpConnection::startReadInLoop()
{
    loop_->assertInLoopThread();
    if (!reading_ || !channel_->isReading())
    {
        channel_->enableReading();
        reading_ = true;
    }
}

void TcpConnection::stopRead()
{
    loop_->runInLoop(std::bind(&TcpConnection::stopReadInLoop, this));
}

void TcpConnection::stopReadInLoop()
{
    loop_->assertInLoopThread();
    if (reading_ || channel_->isReading())
    {
        channel_->disableReading();
        reading_ = false;
    }
}

void TcpConnection::connectEstablished()
{
    loop_->assertInLoopThread();
    assert(state_ == kConnecting);
    setState(kConnected);
    //给Channel一个TcpConnection对象的指针，防止TcpConnection提前析构
    channel_->tie(shared_from_this());
    channel_->enableReading();
    //连接建立后的回调函数
    // shared_from_this()返回this，保证调用connectionCallback_时TcpConnection对象一定存在
    connectionCallback_(shared_from_this());
}

//关闭这个连接
void TcpConnection::connectDestroyed()
{
    loop_->assertInLoopThread();
    if (state_ == kConnected)
    {
        setState(kDisconnected);
        channel_->disableAll();
        connectionCallback_(shared_from_this());
    }
    channel_->remove();
}

//接收到可读信号时调用的函数
void TcpConnection::handleRead(Timestamp receiveTime)
{
    loop_->assertInLoopThread();
    int savedErrno = 0;
    //先读入缓冲区，一次就能读完
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    if (n > 0)
    {
        //收到数据，数据已存入缓冲区，调用消息处理函数
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }
    else if (n == 0)
    {
        //接收到可读信号，但读到的字节数为0，说明对方已经关闭，回调自身的close函数
        handleClose();
    }
    else
    {
        //处理出错
        errno = savedErrno;
        LOG_SYSERR << "TcpConnection::handleRead";
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    loop_->assertInLoopThread();
    if (channel_->isWriting())
    {
        ssize_t n = sockets::write(channel_->fd(),
                                   outputBuffer_.peek(),
                                   outputBuffer_.readableBytes());
        if (n > 0)
        {
            //从缓冲区中取出n字节，代表已经发送n字节数据
            outputBuffer_.retrieve(n);
            //数据发送完毕则停止观测可写事件，防止busy loop,没有的话，继续观测
            if (outputBuffer_.readableBytes() == 0)
            {
                channel_->disableWriting();
                //调用低水位处理函数
                if (writeCompleteCallback_)
                {
                    loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                }
                //如果是在断开连接状态，数据发送完了就断开连接
                if (state_ == kDisconnecting)
                {
                    shutdownInLoop();
                }
            }
        }
        else
        {
            LOG_SYSERR << "TcpConnection::handleWrite";
        }
    }
    else
    {
        LOG_TRACE << "Connection fd = " << channel_->fd()
                  << " is down, no more writing";
    }
}

void TcpConnection::handleClose()
{
    loop_->assertInLoopThread();
    LOG_TRACE << "fd = " << channel_->fd() << " state = " << stateToString();
    assert(state_ == kConnected || state_ == kDisconnecting);
    setState(kDisconnected);
    channel_->disableAll();

    TcpConnectionPtr guardThis(shared_from_this());
    connectionCallback_(guardThis);
    //服务端关闭套接字调用&TcpServer::removeConnection，最终调用&TcpConnection::connectDestroyed
    //客户端关闭套接字调用TcpConnection::connectDestroyed
    closeCallback_(guardThis);
}

void TcpConnection::handleError()
{
    int err = sockets::getSocketError(channel_->fd());
    LOG_ERROR << "TcpConnection::handleError [" << name_
              << "] - SO_ERROR = " << err << " " << strerror_tl(err);
}