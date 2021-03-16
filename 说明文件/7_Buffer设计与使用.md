
# 非阻塞编程中的应用层buffer

- 一个输入操作通常包括两个阶段
  - 等待数据准备好
  - 从内核向进程复制数据
- 对于一个套接字上的输入操作
  - 第一步通常涉及等待数据从网络中到达。当所等待数据到达时，它被复制到内核中的某个缓冲区。
  - 第二步就是把数据从内核缓冲区复制到应用进程缓冲区。
- 写数据时应用层缓冲是必要的
  - 如果程序要写入的数据大于了操作系统能写的数据
  - 剩下的部分可以存入应用层缓冲区中，并注册POLLOUT事件
  - 一旦套接字变得可写就立即发送数据，写完后停止关注POLLOUT
  - 如果应用层缓冲区中已有数据时，程序再次写入，应该append到原有数据之后
  - 每次关闭TCP连接时应该发送完缓冲区中的数据
- 读数据也是必要的
  - TCP是一个无边界的字节流协议
  - 接收方需要处理“收到的数据尚不构成一条完整的信息”“一次收到两条信息的数据”的状况
  - 处理套接字可读时，最好一次性全部读完，不然会反复触发可读事件
  - 先把收到的数据都放到应用程序缓冲区中，等待构成一条完整的信息或者分解成多条信息

## 使用者
- input和output是针对客户代码而言的
- TcpConnection  
  - 从socket中读取数据，写入input buffer
  - 从output buffer中读取数据并写入socket
- 用户
  - 从input buffer中读取数据
  - 把数据写入output buffer

# 设计

## 设计要点
- 对外表现为一块连续的内存
- size可以自动增长，能适应不同大小的消息
- 内部以vector\<char>来保存消息
- 行为类似queue，从末尾写入数据，从头部读出数据

## 临时栈上空间运用
- 使用这项技术的原因
  - 如果希望减少系统调用，一次读/写的数据越多越划算，这需要一个大的缓冲区
  - 但是缓冲区会占用内存
- 利用临时栈上空间，避免每个连接的初始Buffer过大造成内存浪费，也避免反复调用read()的系统开销  
- 在栈上准备一个65536字节的extrabuf(内置buffer的初始大小为1024字节)，然后利用readv()来读取数据，iovec有两块，第一块指向内置Buffer中的writable字节，另一块指向栈上的extrabuf。  
- 如果读入的数据不多，则全部读入到Buffer中，如果长度超过了Buffer的writable字节数，则读到栈上的extrabuf,然后再把extrabuf里的数据append()到Buffer中
- 见Buffer.cpp中readFd函数
  - 两个缓冲区，第一个是buffer,第二个是栈上的extrabuf
  - 先读写第一个，再读写第二个   
- 读数据的时候会用到，保证一次就能读完所有的数据
- 如果写数据时缓冲区太小，直接就扩充缓冲区了

## 前方添加

- 提供prepend空间，让程序能以很低的代价在数据前面添加几个字节
- 例如程序用固定的4字节表示消息的长度，但一开始不知道具体的长度，可以一直append直到数据全部完成，然后再在前面添加长度信息

## 数据结构
- 三个数据成员
  - 一个vector<char>来保存数据
  - 两个index，一个记录可写的序号，一个记录可读的序号
- vector的内容被划分成三块

## 主要操作

- 常见的就是往缓冲区追加数据，取数据
- append时需要确保里面空间足够，调用makeSpace
  - 如果空间确实不够，调用vector的resize
  - 现在的空间是够的，只是prepend所占空间太大，把数据往前移，给writable腾出空间
- 用户可以主动调用shrink收缩缓冲区大小

## 关键函数
### io向量和readv函数
```cpp
//从文件描述符fd所对应的的文件中读数据，写到count个buffers中，该buffer用iovec描述
ssize_t readv(int fd,const struct iovec *iov, int count);
//把count个buffer(使用iovec描述)中的数据写入到文件描述符fd所对应的的文件中
ssize_t writev(int fd,const struct iovec *iov, int count);
struct iovec{
      void *iov_base; /* pointer to the start of buffer */
      size_t iov_len; /* size of buffer in bytes */
};
```
- 将按照iov[0]、iov[1]、…、iov[count-1]的顺序依次读写，并且他们在文件中的地址是连续的
- 成功，返回读写的字节数，这个字节数是所有iovec结构中iov_len的总和；
- 失败返回-1，并设置好errno
- [参考资料](https://fuliang.iteye.com/blog/652297)

### 字节序转换函数
比较通用的函数
```cpp
#include<endian.h>
//be表示大端，le表示小端，h表示主机
uint16_t htobe16(uint16_t host_16bits);
uint16_t htole16(uint16_t host_16bits);
uint16_t be16toh(uint16_t big_endian_16bits);
uint16_t le16toh(uint16_t little_endian_16bits);
 
uint32_t htobe32(uint32_t host_32bits);
uint32_t htole32(uint32_t host_32bits);
uint32_t be32toh(uint32_t big_endian_32bits);
uint32_t le32toh(uint32_t little_endian_32bits);
 
uint64_t htobe64(uint64_t host_64bits);
uint64_t htole64(uint64_t host_64bits);
uint64_t be64toh(uint64_t big_endian_64bits);
uint64_t le64toh(uint64_t little_endian_64bits);
```
运用在套接字编程中一般是下面的
```cpp
#include<netinet/in.h>
// hton* 主机字节转网络字节序
uint64_t htonll(uint64_t hostlonglong);
uint32_t htonl(uint32_t hostlong);
uint16_t htons(uint16_t hostshort);
// ntoh* 网络字节序转主机字节序
uint64_t ntohll(uint64_t hostlonglong);
uint32_t ntohl(uint32_t netlong);
uint16_t ntohs(uint16_t netshort);
```
[参考资料](https://wanshi.iteye.com/blog/2214693)


