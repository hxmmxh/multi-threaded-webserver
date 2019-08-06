#ifndef HXMMXH_TCPCONNECTION_H
#define HXMMXH_TCPCONNECTION_H

#include "Callbacks.h"
#include "Buffer/Buffer.h"
#include "Sockets/InetAddress.h"
#include "../Log/StringPiece.h"

#include <string>
#include <memory>

struct tcp_info;
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

    void send(const void *message, int len);
    void send(const StringPiece &message);
    void send(Buffer *message);

    void shutdown();

    void forceClose();                        //主动关闭
    void forceCloseWithDelay(double seconds); //延迟后主动关闭
    void setTcpNoDelay(bool on);

    void startRead();
    void stopRead();
    bool isReading() const { return reading_; };

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
    void connectEstablished();
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
    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    void sendInLoop(const StringPiece &message);
    void sendInLoop(const void *message, size_t len);

    void shutdownInLoop();
    // void shutdownAndForceCloseInLoop(double seconds);
    void forceCloseInLoop();
    void setState(StateE s) { state_ = s; }
    const char *stateToString() const;
    void startReadInLoop();
    void stopReadInLoop();

    EventLoop *loop_;
    std::string name_;
    StateE state_;
    bool reading_;

    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;
    const InetAddress localAddr_;
    const InetAddress peerAddr_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    HighWaterMarkCallback highWaterMarkCallback_;
    CloseCallback closeCallback_;
    size_t highWaterMark_;
    Buffer inputBuffer_;
    Buffer outputBuffer_;
};

#endif