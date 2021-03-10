#ifndef HXMMXH_TCPCONNECTION_H
#define HXMMXH_TCPCONNECTION_H

#include "Callbacks.h"
#include "Buffer.h"
#include "InetAddress.h"
#include "Socket.h"
#include "StringPiece.h"

#include <string>
#include <memory>
struct tcp_info; //定义在<netinet/tcp.h>
namespace hxmmxh
{

class Channel;
class EventLoop;
class Socket;

class TcpConnection : public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop *loop,
                  const std::string &name,
                  int sockfd,
                  const InetAddress &localAddr,
                  const InetAddress &peerAddr);
    ~TcpConnection();

    EventLoop *getLoop() const { return loop_; }
    const std::string &name() const { return name_; }
    const InetAddress &localAddress() { return localAddr_; }
    const InetAddress &peerAddress() { return peerAddr_; }
    bool connected() const { return state_ == kConnected; }
    bool disconnected() const { return state_ == kDisconnected; }

    bool getTcpInfo(struct tcp_info *) const;
    std::string getTcpInfoString() const;
    void setTcpNoDelay(bool on);

    //发送数据相关函数
    void send(const void *message, int len);
    void send(const StringPiece &message);
    void send(Buffer *message);

    //关闭连接相关函数
    void shutdown();                          //关闭写端，还可以接收数据
    void forceClose();                        //主动关闭
    void forceCloseWithDelay(double seconds); //延迟后主动关闭

    //接收数据相关函数
    void startRead();                            //开始接收数据
    void stopRead();                             //停止接收数据
    bool isReading() const { return reading_; }; //是否在接收数据

    void setConnectionCallback(const ConnectionCallback &cb)
    {
        connectionCallback_ = cb;
    }

    void setMessageCallback(const MessageCallback &cb)
    {
        messageCallback_ = cb;
    }

    void setWriteCompleteCallback(const WriteCompleteCallback &cb)
    {
        writeCompleteCallback_ = cb;
    }

    void setHighWaterMarkCallback(const HighWaterMarkCallback &cb, size_t highWaterMark)
    {
        highWaterMarkCallback_ = cb;
        highWaterMark_ = highWaterMark;
    }

    void setCloseCallback(const CloseCallback &cb)
    {
        closeCallback_ = cb;
    }

    Buffer *inputBuffer()
    {
        return &inputBuffer_;
    }

    Buffer *outputBuffer()
    {
        return &outputBuffer_;
    }

    //用于Tcpserver，接收到一个新连接时调用
    void connectEstablished();
    //用于Tcpserver，移除一个连接时调用
    void connectDestroyed();

private:
    enum StateE
    {
        kDisconnected,
        kConnecting,
        kConnected,
        kDisconnecting
    };
    void setState(StateE s) { state_ = s; }
    const char *stateToString() const;

    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    void sendInLoop(const StringPiece &message);
    void sendInLoop(const void *message, size_t len);
    void shutdownInLoop();
    void forceCloseInLoop();

    void startReadInLoop();
    void stopReadInLoop();

    EventLoop *loop_;
    std::string name_;
    StateE state_;
    bool reading_; //是否在接收数据

    std::unique_ptr<Socket> socket_;   //拥有的已连接套接字
    std::unique_ptr<Channel> channel_; //监听该套接字的Channel
    const InetAddress localAddr_;      //本地地址
    const InetAddress peerAddr_;       //对端地址

    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    HighWaterMarkCallback highWaterMarkCallback_;
    CloseCallback closeCallback_;

    size_t highWaterMark_;
    Buffer inputBuffer_;  //输入缓冲区
    Buffer outputBuffer_; //输出缓冲区
};
} // namespace hxmmxh
#endif