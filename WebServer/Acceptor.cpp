#include "Acceptor.h"

#include "Logging.h"
#include "Reactor/EventLoop.h"
#include "Sockets/InetAddress.h"
#include "Sockets/SocketsOps.h"

#include <cerrno>
#include <fcntl.h>
#include <unistd.h>

using namespace hxmmxh;

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport)
    : loop_(loop),
      acceptSocket_(sockets::createNonblockingOrDie(listenAddr.family())),
      acceptChannel_(loop, acceptSocket_.fd()),
      listenning_(false),
      idleFd_(::open("/dev/null", O_RDONLY | O_CLOEXEC))
{
    assert(idleFd_ >= 0);
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(reuseport);
    //调用bind,绑定地址
    acceptSocket_.bindAddress(listenAddr);
    //对应的Channel注册回调函数
    acceptChannel_.setReadCallback(
        std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor()
{
    acceptChannel_.disableAll();
    acceptChannel_.remove();
    ::close(idleFd_);
}

void Acceptor::listen()
{
    loop_->assertInLoopThread();
    listenning_ = true;
    //套接字开始listen
    acceptSocket_.listen();
    //对应的Channel开始监听可读事件
    acceptChannel_.enableReading();
}

//当有客户与服务器建立连接时，监听描述符变得可读，于是调用accept
void Acceptor::handleRead()
{
    loop_->assertInLoopThread();
    InetAddress peerAddr;
    //返回已连接套接字描述符,peerAddr中保存对端地址
    int connfd = acceptSocket_.accept(&peerAddr);
    if (connfd >= 0)
    {
        if (newConnectionCallback_)
        {
            newConnectionCallback_(connfd, peerAddr);
        }
        else
        {
            sockets::close(connfd);
        }
    }
    else
    {
        LOG_SYSERR << "in Acceptor::handleRead";
        //处理文件描述符耗尽
        if (errno == EMFILE)
        {
            ::close(idleFd_);
            idleFd_ = ::accept(acceptSocket_.fd(), NULL, NULL);
            ::close(idleFd_);
            idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
        }
    }
}
