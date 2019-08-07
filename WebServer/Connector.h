#ifndef HXMMXH_CONNECTOR_H
#define HXMMXH_CONNECTOR_H

#include "Sockets/InetAddress.h"

#include <functional>
#include <memory>

namespace hxmmxh
{
class Channel;
class EventLoop;

class Connector : public std::enable_shared_from_this<Connector>
{
public:
    typedef std::function<void(int sockfd)> NewConnectionCallback;

    Connector(EventLoop *loop, const InetAddress &serverAddr);
    ~Connector();

    void setNewConnectionCallback(const NewConnectionCallback &cb)
    {
        newConnectionCallback_ = cb;
    }

    void start();   // 可以在任意线程调用，开始connect
    void restart(); // 只能所属的loop线程中调用
    void stop();    // 可以在任意线程调用，停止connect

    const InetAddress &serverAddress() const { return serverAddr_; }

private:
    enum States
    {
        kDisconnected, //没有连接
        kConnecting,   //正在连接
        kConnected     //已连接
    };
    //重试的间隔
    static const int kMaxRetryDelayMs = 30 * 1000;
    static const int kInitRetryDelayMs = 500;

    void setState(States s) { state_ = s; }
    void startInLoop(); //实际执行start()的函数，调用connect()
    void stopInLoop();  //实际执行stop()的函数
    void connect();     //执行连接工作
    void connecting(int sockfd);
    void handleWrite();     //可写时的操作，表明连接建立
    void handleError();     //错误处理操作
    void retry(int sockfd); //重试，需要新的socket和新的Channel
    //管理Channel的移除和重置
    int removeAndResetChannel();
    void resetChannel();

    EventLoop *loop_;
    InetAddress serverAddr_; //要连接的地址
    bool connect_;           //是否要进行连接
    States state_;
    //重试需要新的Channel对象，为了方便管理其生命期，采用智能指针
    std::unique_ptr<Channel> channel_;
    NewConnectionCallback newConnectionCallback_;
    int retryDelayMs_;
};

} // namespace hxmmxh

#endif