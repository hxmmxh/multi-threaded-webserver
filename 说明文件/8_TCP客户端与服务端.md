

- [Acceptor](#acceptor)
  - [工作流程](#工作流程)
  - [EMFILE,文件描述符耗尽处理方法](#emfile文件描述符耗尽处理方法)
- [TcpServer](#tcpserver)
  - [主要成员](#主要成员)
  - [创建一个可用TcpServer对象的流程](#创建一个可用tcpserver对象的流程)
- [TcpConnection](#tcpconnection)
  - [主要成员](#主要成员-1)
  - [工作流程](#工作流程-1)
  - [读数据](#读数据)
  - [写数据](#写数据)
  - [关闭连接](#关闭连接)
  - [高水位回调和低水位回调](#高水位回调和低水位回调)
  - [四种回调函数](#四种回调函数)
    - [正常状态下主动关闭连接](#正常状态下主动关闭连接)
    - [强制关闭连接](#强制关闭连接)
    - [边缘触发的读写问题](#边缘触发的读写问题)
- [Connector](#connector)
  - [工作流程](#工作流程-2)
- [TcpClient](#tcpclient)



# Acceptor

- 功能是用于accept新的Tcp连接，并通过回调通知使用者
- 属于一个TcpServer，由TcpServer负责创建和销毁

## 工作流程

- TcpServer对象初始化时会新建一个Accoptor对象
  - 需要提供套接字地址，可选项，一个EventLoop对象
  - 构造函数中完成以下事情：
  - 创建一个Socket套接字，设置好地址端口和可选项
  - 创建一个Channel负责这个套接字
  - Channel注册套接字可读的回调函数为handleRead
  - 提前准备一个空闲的文件描述符
- TcpServer开始监听时会调用Accetor的Listen函数
  - 套接字开始Socket::listen
  - Channel开始监听可读事件
- 当有客户与服务器建立连接时，监听描述符变得可读，调用handleRead
  - 调用Socket::accept返回已连接套接字描述符
  - 调用newConnectionCallback_函数开始处理这个套接字
    - newConnectionCallback_函数会在TcpServer对象初始化时被设置为TcpServer::newConnection
  - 如果遇到了Emfile错误，如下处理

## EMFILE,文件描述符耗尽处理方法
- 当accept返回EMFILE时，意味着本进程的文件描述符已经达到上限，无法为新连接创建套接字文件描述符。
- 但是既然没有文件描述符来表示这个连接，也就无法close它，这个新连接会处于一直等待处理的状态。每次epoll_wait都会立刻返回。会使程序陷入busy loop。
- 有7种解决做法，本程序选用的是第6种
    1. 调高进程的文件描述符数目。治标不治本
    2. 死等
    3. 退出程序
    4. 关闭监听套接字
    5. 用ET取代LT,不会陷入无限循环的状态
    6. 提前准备一个空闲的文件描述符，遇到EMFILE时，先关闭这个空闲文件，获得一个文件描述符的名额，再accept拿到新连接的描述符，随后立刻close它，最后再重新打开一个空闲文件
    7. 自己设置一个稍微低点的文件描述符数量限制，超过这个限制就主动关闭新连接。

# TcpServer

- 用于建立新连接
- 为连接创建对应的TcpConnection对象
- 拥有监听套接字listening socket

## 主要成员
- `unique_ptr<Acceptor> acceptor_`
  - 用是unique_ptr为了方便自动析构
  - 并且一个Acceptor只属于一个TcpServer
- 

## 创建一个可用TcpServer对象的流程
- 新建一个EventLoop
- 新建TcpServer对象
  - 提供Server名称，套接字地址，可选项，还需要提供一个EventLoop对象`TcpServer(EventLoop *loop,const InetAddress &listenAddr,const std::string &nameArg,Option option = kNoReusePort);`
  - 构造函数中完成以下操作：
  - 创建一个Acceptor用于接收新连接,并设置接收到新连接的回调函数为newConnection
  - 创建一个EventLoopThreadPool线程池对象,为的是把新连接分配倒线程池中
- 调用setThreadNum设置线程池的大小，不设置默认是单线程
- 调用start开始监听套接字
  - 修改start_为1,表明开始监听
  - 启动线程池
  - 在所处的EventLoop中注册Acceptor::listen任务，开始监听
  - 监听套接字表的可读时会调用newConnection
    - 从线程池中取出一个线程，记录该线程中的EventLoop
    - 创建一个TcpConnection对象
      - 对象的名字为`服务端的名字+ip地址+TCP连接序号`
      - 在ConncectionMap中记录这个对象
      - 设置这个对象的回调函数
      - 调用EventLoop::runInLoop运行TcpConnection::connectionEstablish()
- 可以调用setMessageCallback，设置收到数据时服务端的操作
- EventLoop调用loop()开始事件循环
- 移除一个connection的流程
  - 在ConncectionMap中删除这个对象
  - 获取这个connection所在的事件循环
  - 在那个EventLoop中注册connecetionDestory任务



# TcpConnection

- 用于表示已经建立的套接字连接
  - 表示一次Tcp连接，是不可再生的，一旦连接断开，这个对象就失去了作用
  - 没有发起连接的功能，构造参数是已经建立好连接的套接字文件描述符，建立连接的功能在[connector](#Connector)和[accptor](#Acceptor)
- 使用Channel来获取socket上的IO事件，然后自己处理wriable事件，而把readable事件通过MessageCallback传达给客户
- 拥有Tcp已连接套接字，析构函数会关闭fd


## 主要成员

- 已连接套接字和响应的监听Channel
- 本地地址和对端地址
- 输入输出缓冲区，高水位阈值
- 一系列回调函数
  - 每当有新连接建立时，会调用connectionCallback_
  - 每当有新数据可读时，会调用messageCallback_
  - 每当数据写完时，会调用writeCompleteCallback_


## 工作流程

- 在accpet到一个新连接时，由TcpServer创建一个TcpConnection对象
  - 设置state_为 正在建立连接
  - 创建一个Socket类封装这个套接字（传入的是套接字描述符）
  - 创建一个Channel用于监听这个套接字
  - 注册这个Channel的四个回调函数（可读，可写，出错，已关闭）
  - 设置套接字的SO_KEEPALIVE选项，定时发送保存存活探测分节
  - 设置高水位阈值
- TcpServer随后会运行connectionEstablish函数
  - 设置state_为已建立连接
  - Channel开始监听可读事件
  - 给Channel一个TcpConnection对象的指针，防止TcpConnection提前析构
  - 调用connectionCallback_
- 需要服务端程序设置一些函数调用
  - setConnectionCallback
  - setMessageCallback
  - setWriteCompleteCallback
  - setHighWaterMarkCallback
  - setCloseCallback

## 读数据

- TcpConnection对象建立是就会开始监听可读事件
- 可读事件就绪时会调用handleRead
- 把数据写入到inputBuffer_，随后调用消息处理函数messageCallback_
- 读数据是被动的

## 写数据

- 写数据接口,send
  - 判断状态为已连接
  - 如果是在所属的IO线程中，直接调用sendInLoop，否则用runInLoop注册sendInLoop函数
- 实际完成写数据操作的函数,sendInLoop
  - 判断连接状态，如果连接已断开就放弃发送
  - 如果发送缓冲区中已经有待发送的数据，就不能直接写入套接字了，会造成数据乱序，只能把数据先写入缓冲区
  - 如果发送缓冲区是空并且Channel不在监听可写事件，则可以直接开始写入
    - 记录写入的字节数，和剩余的字节数
    - 如果数据写完了调用writeCompleteCallback_
    - 没写完就把剩下的数据写入outputBuffer_缓冲区
  - 写入数据到缓冲区操作
    - 如果数据大小超过了高水位值，调用highWaterMarkCallback_
    - 开始监听可写操作
    - 在handlderwrite中发送剩下的数据

## 关闭连接


## 高水位回调和低水位回调
- 发送缓冲区被清空则调用WriteCompleteCallback
- 输入缓冲区的长度超过指定的大小，就调用HighWaterMarkCallback


## 四种回调函数
- handleRead
  - 调用readfd把数据一次性读入inputBuffer缓冲区中
  - 如果读入的字节数大于0，调用消息处理函数messageCallback_
  - 如果读到的字节数为0，说明对方已经关闭，调用handleClose关闭连接
  - 如果读到的字节数小于0，说明出错，调用handleError处理错误
- handleWrite
  - 首先判断Channel是否在监听可写事件，在的话才开始写入
  - 写outputBuffer_缓冲区中的数据到套接字文件描述符
  - 从缓冲区中取走已写的字节数
  - 如果缓冲区已空，即数据发送完毕，则停止观测可写事件，防止busy loop,并调用writeCompleteCallback_，没有的话，继续观测
  - 如果是在断开连接状态，数据发送完了就要断开连接
- handleClose
  - 把状态设置为已断开连接
  - 把Channel监听的事件清空
  - 调用connectionCallback_
  - 调用CloseCallback
- handleError
  - 简单的输出错误信息









### 正常状态下主动关闭连接




* close()和shutdown()的区别
    * close把描述符的引用计数减一，仅在该计数变为0时才关闭套接字，但shutdown可以不管引用计数就激发tcp的正常连接终止序列
    * close同时终止读方向和写方向的数据传送，而shutdown具有可选的行为
* TcpConnection的shutdown()提供了shutdown(fd,SHUT_WR)。可以解决“当你打算关闭网络连接时，如何得知对方是否发送了一些数据而你还没有收到”的问题，不会漏收”还在路上“的数据
* 相当于把主动关闭连接这件事件分成两步来做，先关闭本地写端，等对象关闭后，再关闭本地读端。(read()返回0，就代表对方已经关闭)

### 强制关闭连接
* 正常来说TcpConnection只调用shutdown关闭套接字的写端，而等对方关闭后，再关闭读端，但如果对方故意不关闭连接，那么连接就一直处于半开状态，消耗系统资源，所以也提供强制关闭函数forceClose()
* 






### 边缘触发的读写问题
[](https://blog.csdn.net/weiwangchao_/article/details/43685671)
在epoll的ET模式下，正确的读写方式为:
读：只要可读，就一直读，直到返回0(#add 读空)，或者 errno = EAGAIN
写:只要可写，就一直写，直到数据发送完（#add 写满），或者 errno = EAGAIN




# Connector

- 用于TcpClient客户端调用，主动建立连接
- 调用connect，当套接字变得可写时表明连接建立完毕

- 套接字是一次性的，一旦出错就无法恢复，只能关闭重来
- 而Connector是可以反复使用的，每次尝试连接都要使用新的套接字文件描述符和新的Channel对象。要注意析构原来的
  - 用unqiue_ptr管理Channel，每次使用新Channel时，调用reset

## 工作流程

- 构造函数需要传入一个EventLoop和一个地址，一般在TcpClient的初始化过程中构造
- 随后会被TcpClient调用start，开始建立连接，实际工作是在connect函数中进行的
  - 先创建套接字
  - 然后调用socket::connect,成功返回0，出错返回-1
    - 由于是非阻塞的套接字，有些错误码仅代表提前返回（一般是EINPROGRESS),调用connecting
      - 新建一个Channel，注册可读出错回调函数，然后监听可写事件
    - 对于可修正的错误，尝试重连，调用retry
    - 对于无法挽回的错误，关闭套接字并退出
- connect的套接字可写时
  - 释放对应的Channel
  - 使用getsockopt获取错误值，如果等于0就是连接成功,如果出错调用retry,发生了自连接也调用retry
  - 如果正常建立了连接，调用newConnectionCallback_
- retry
  - 需要先关闭之前的套接字文件描述符
  - 在事件循环的定时器中注册startInLoop函数
  - 每次重试的时间加长

# TcpClient

- 每个TcpClient管理一个TcpConnection
- 具备TcpConnection断开后重新连接的功能
  - Connector也有反复尝试连接的功能，因此客户端和服务端的启动顺序无关紧要
- 连接断开后初次重试的延迟应该有随机性，因为如果连接同时断开后又同时再次发起连接，会给服务端带来短期的大负载