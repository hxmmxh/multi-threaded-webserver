介绍输入输出缓冲区的设计与实现
_____
## 目录
## buffer存在的必要性
### 
1. input buffer
2. output buffer

### 使用者
1. TcpConnection  
    * 从socket中读取数据，写入input buffer
    * 从output buffer中读取数据并写入socket
2. 用户
    * 从input buffer中读取数据
    * 把数据写入output buffer
## 设计

### 设计要点
1. 对外表现为一块连续的内存
2. size可以自动增长，能适应不同大小的消息
3. 内部以vector\<char>来保存消息
4. 行为类似queue，从末尾写入数据，从头部读出数据
### 数据结构

### 主要操作

### 临时栈上空间运用
见Buffer.cpp中readFd函数    
利用临时栈上空间，避免每个连接的初始Buffer过大造成内存浪费，也避免反复调用read()的系统开销  
在栈上准备一个65536字节的extrabuf(内置buffer的初始大小为1024字节)，然后利用
readv()来读取数据，iovec有两块，第一块指向内置Buffer中的writable字节，另一块指向栈上的extrabuf。  
如果读入的数据不多，则全部读入到Buffer中，如果长度超过了Buffer的writable字节数，则读到栈上的extrabuf,然后再把extrabuf里的数据append()到Buffer中



## 关键函数
### io向量和readv函数
```cpp
//从文件描述符fd所对应的的文件中读去count个数据段到多个buffers中，该buffer用iovec描述
ssize_t readv(int fd,const struct iovec *iov, int count);
//把count个数据buffer(使用iovec描述)写入到文件描述符fd所对应的的文件中
ssize_t writev(int fd,const struct iovec *iov, int count);
struct iovec{
      void *iov_base; /* pointer to the start of buffer */
      size_t iov_len; /* size of buffer in bytes */
};
```
* 将按照iov[0]、iov[1]、…、iov[count-1]的顺序依次读写，并且他们在文件中的地址是连续的
* 成功，返回读写的字节数，这个字节数是所有iovec结构中iov_len的总和；
* 失败返回-1，并设置好errno
* [参考资料](https://fuliang.iteye.com/blog/652297)

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


