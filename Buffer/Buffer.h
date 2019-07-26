#ifndef BUFFER_H
#define BUFFER_H

#include <algorithm>
#include <string>
#include <cstring>
#include <vector>
#include <cassert>
#include <netinet/in.h> //htons,htonl,hton

/// A buffer class modeled after org.jboss.netty.buffer.ChannelBuffer
///
/// @code
/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size
/// @endcode
class Buffer
{
public:
    static const size_t kCheapPrepend = 8;   //prependable的初始大小
    static const size_t kInitialSize = 1024; //wirtable的初始大小

    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize),
          readerIndex_(kCheapPrepend),
          writerIndex_(kCheapPrepend)
    {
        assert(readableBytes() == 0);
        assert(writableBytes() == initialSize);
        assert(prependableBytes() == kCheapPrepend);
    }

    //使用默认的拷贝构造，析构，赋值函数

    //交换两个Buffer
    void swap(Buffer &rhs)
    {
        buffer_.swap(rhs.buffer_);
        std::swap(readerIndex_, rhs.readerIndex_);
        std::swap(writerIndex_, rhs.writerIndex_);
    }
    //可读字节数
    size_t readableBytes() const
    {
        return writerIndex_ - readerIndex_;
    }
    //可写字节数
    size_t writableBytes() const
    {
        return buffer_.size() - writerIndex_;
    }
    //缓存前面的空间
    size_t prependableBytes() const
    {
        return readerIndex_;
    }
    //可读的第一个字节
    const char *peek() const
    {
        return begin() + readerIndex_;
    }
    //取数指令，取多少字节
    void retrieve(size_t len)
    {
        assert(len <= readableBytes());
        if (len < readableBytes())
        {
            readerIndex_ += len;
        }
        else //如果超过最大可取数则全取
        {
            retrieveAll();
        }
    }
    //取数直到end指针位置
    void retrieveUntil(const char *end)
    {
        assert(peek() <= end);
        assert(end <= beginWrite());
        retrieve(end - peek());
    }
    //取不同字节大小的int
    void retrieveInt64()
    {
        retrieve(sizeof(int64_t));
    }

    void retrieveInt32()
    {
        retrieve(sizeof(int32_t));
    }

    void retrieveInt16()
    {
        retrieve(sizeof(int16_t));
    }

    void retrieveInt8()
    {
        retrieve(sizeof(int8_t));
    }
    //把buffer_全取完
    void retrieveAll()
    {
        readerIndex_ = kCheapPrepend;
        writerIndex_ = kCheapPrepend;
    }
    //把buffer_内的数据全取出，表示成string格式
    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes());
    }
    //取len个字节转换成string
    std::string retrieveAsString(size_t len)
    {
        assert(len <= readableBytes());
        std::string result(peek(), len);
        retrieve(len);
        return result;
    }
    //在buffer_追加数据
    void append(const std::string &str)
    {
        //str.data()返回str第一个char的指针
        append(str.data(), str.length());
    }
    void append(const char *data, size_t len)
    {
        ensureWritableBytes(len);
        std::copy(data, data + len, beginWrite());
        hasWritten(len);
    }
    void append(const void *data, size_t len)
    {
        append(static_cast<const char *>(data), len);
    }
    //确保可写空间足够大，大于len
    void ensureWritableBytes(size_t len)
    {
        //如果空间不够大，调用makespace扩展空间
        if (writableBytes() < len)
        {
            makeSpace(len);
        }
        assert(writableBytes() >= len);
    }
    //可写的第一个位置
    char *beginWrite()
    {
        return begin() + writerIndex_;
    }
    const char *beginWrite() const
    {
        return begin() + writerIndex_;
    }
    //写入操作完成后更改index
    void hasWritten(size_t len)
    {
        writerIndex_ += len;
    }
    void appendInt64(int64_t x)
    {
        int64_t be64 = htobe64(x);
        append(&be64, sizeof be64);
    }
    //写入不同大小的int，用网络字节序，也就是大端法
    void appendInt32(int32_t x)
    {
        int32_t be32 = htobe32(x);
        append(&be32, sizeof be32);
    }

    void appendInt16(int16_t x)
    {
        int16_t be16 = htobe16(x);
        append(&be16, sizeof be16);
    }

    void appendInt8(int8_t x)
    {
        append(&x, sizeof x);
    }
    int64_t readInt64()
    {
        int64_t result = peekInt64();
        retrieveInt64();
        return result;
    }
    //读出不同类型的int
    int32_t readInt32()
    {
        int32_t result = peekInt32();
        retrieveInt32();
        return result;
    }

    int16_t readInt16()
    {
        int16_t result = peekInt16();
        retrieveInt16();
        return result;
    }

    int8_t readInt8()
    {
        int8_t result = peekInt8();
        retrieveInt8();
        return result;
    }
    //把大端存储的字节转换成int
    int64_t peekInt64() const
    {
        assert(readableBytes() >= sizeof(int64_t));
        int64_t be64 = 0;
        ::memcpy(&be64, peek(), sizeof(be64));
        return be64toh(be64);
    }

    int32_t peekInt32() const
    {
        assert(readableBytes() >= sizeof(int32_t));
        int32_t be32 = 0;
        ::memcpy(&be32, peek(), sizeof(be32));
        return be32toh(be32);
    }

    int16_t peekInt16() const
    {
        assert(readableBytes() >= sizeof(int16_t));
        int16_t be16 = 0;
        ::memcpy(&be16, peek(), sizeof(be16));
        return be16toh(be16);
    }
    int8_t peekInt8() const
    {
        assert(readableBytes() >= sizeof(int8_t));
        int8_t x = *peek();
        return x;
    }
    //在prependable空间写入数据
    void prependInt64(int64_t x)
    {
        int64_t be64 = htobe64(x);
        prepend(&be64, sizeof be64);
    }
    void prependInt32(int32_t x)
    {
        int32_t be32 = htobe32(x);
        prepend(&be32, sizeof be32);
    }

    void prependInt16(int16_t x)
    {
        int16_t be16 = htobe16(x);
        prepend(&be16, sizeof be16);
    }
    void prependInt8(int8_t x)
    {
        prepend(&x, sizeof x);
    }
    void prepend(const void * /*restrict*/ data, size_t len)
    {
        assert(len <= prependableBytes());
        readerIndex_ -= len;
        const char *d = static_cast<const char *>(data);
        std::copy(d, d + len, begin() + readerIndex_);
    }

    void prepend(const void *data, size_t len)
    {
        assert(len <= prependableBytes());
        readerIndex_ -= len;
        const char *d = static_cast<const char *>(data);
        std::copy(d, d + len, begin() + readerIndex_);
    }
    //prependable区域收缩到初始大小
    //收缩后的writableBytes为kInitialSize - readableBytes()和reserve之间的较大者
    void shrink(size_t reserve)
    {
        Buffer other;
        other.ensureWritableBytes(readableBytes() + reserve);
        other.append(begin() + readerIndex_, readableBytes());
        swap(other);
    }

    //返回容量大小
    size_t internalCapacity() const
    {
        return buffer_.capacity();
    }

    //ssize_t readFd(int fd, int *savedErrno);

private:
    //返回buffer_的第一个元素的指针
    char *begin()
    {
        return &*buffer_.begin();
    }
    const char *begin() const
    {
        return &*buffer_.begin();
    }
    //使Buffer至少有len大小的空间
    void makeSpace(size_t len)
    {
        //空间确实不够，分配新空间
        if (writableBytes() + prependableBytes() < len + kCheapPrepend)
        {
            buffer_.resize(writerIndex_ + len);
        }
        //现在的空间使够的，知识prepend所占空间太大，把数据往前移，给writable腾出空间
        else
        {
            assert(kCheapPrepend < readerIndex_);
            size_t readable = readableBytes();
            //把readerIndex_和writerIndex_之间的数据移到kCheapPrepend开始的位置
            std::copy(begin() + readerIndex_,
                      begin() + writerIndex_,
                      begin() + kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
            assert(readable == readableBytes());
        }
    }

private:
    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;
};

#endif