
//对一些回调函数的typedef
//2019-7-31,4PM完成

#ifndef HXMMXH_CALLBACKS_H
#define HXMMXH_CALLBACKS_H

#include "Timestamp.h"
#include <functional>
#include <memory>

namespace hxmmxh
{

//用于bind函数
using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;


class Buffer;
class TcpConnection;
typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
typedef std::function<void()> TimerCallback;
typedef std::function<void(const TcpConnectionPtr &)> ConnectionCallback;
typedef std::function<void(const TcpConnectionPtr &)> CloseCallback;
typedef std::function<void(const TcpConnectionPtr &)> WriteCompleteCallback;
typedef std::function<void(const TcpConnectionPtr &, size_t)> HighWaterMarkCallback;

typedef std::function<void(const TcpConnectionPtr &,
                           Buffer *,
                           Timestamp)>
    MessageCallback;

void defaultConnectionCallback(const TcpConnectionPtr &conn);
void defaultMessageCallback(const TcpConnectionPtr &conn,
                            Buffer *buffer,
                            Timestamp receiveTime);

} // namespace hxmmxh

#endif