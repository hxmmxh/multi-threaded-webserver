#ifndef HXMMXH_CHAT_CODEC_H
#define HXMMXH_CHAT_CODEC_H

#include "Logging.h"
#include "Buffer.h"
#include "InetAddress.h"
#include "Callbacks.h"
#include "TcpConnection.h"

namespace hxmmxh
{

class LengthHeaderCodec
{
 public:
  typedef std::function<void (const TcpConnectionPtr&,
                                const std::string& message,
                                Timestamp)> StringMessageCallback;

  explicit LengthHeaderCodec(const StringMessageCallback& cb)
    : messageCallback_(cb)
  {
  }

  void onMessage(const TcpConnectionPtr& conn,
                 Buffer* buf,
                 Timestamp receiveTime)
  {
    // 当缓冲区中的数据大于4字节时，才会去解析  
    while (buf->readableBytes() >= kHeaderLen) // kHeaderLen == 4
    {
      // 取出消息的长度
      const void* data = buf->peek();
      int32_t be32 = * static_cast<const int32_t*>(data); 
      const int32_t len = sockets::networkToHost32(be32);
      if (len > 65536 || len < 0)
      {
        LOG_ERROR << "Invalid length " << len;
        conn->shutdown();
        break;
      }
      // 数据的可读字节数大于消息的长度，就需要分包
      else if (buf->readableBytes() >= len + kHeaderLen)
      {
        buf->retrieve(kHeaderLen);
        std::string message(buf->peek(), len);
        // 得到了一条完整的信息，交给messageCallback_处理
        messageCallback_(conn, message, receiveTime);
        buf->retrieve(len);
        // 循环处理剩下的数据
      }
      // 数据的可读字节小于消息长度，不是完整的一个消息，则继续等待
      else
      {
        break;
      }
    }
  }

  void send(TcpConnection* conn,
            const StringPiece& message)
  {
    Buffer buf;
    buf.append(message.data(), message.size());
    int32_t len = static_cast<int32_t>(message.size());
    int32_t be32 = sockets::hostToNetwork32(len);
    buf.prepend(&be32, sizeof be32);
    conn->send(&buf);
  }

 private:
  StringMessageCallback messageCallback_;
  const static size_t kHeaderLen = sizeof(int32_t);
};

}


#endif  // MUDUO_EXAMPLES_ASIO_CHAT_CODEC_H
