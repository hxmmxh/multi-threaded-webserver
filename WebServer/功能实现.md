
本项目为C++11编写的Web服务器
------------
# 目录

1. [日志系统](#)
2. Reactor模型
	* [Reactor模型介绍](#Reactor模型介绍)
    * [Eventloop](#EventLoop)
    * [Channel](#Channel)
    * [Poller](#Poller)
    * [多线程EventLoop](#多线程EventLoop)
3. 套接字连接的建立
    * [Acceptor](#Acceptor)
    * [Connector](#Connector)
5. 已连接套接字
    * [TcpConnection](#TcpConnection)
6. 服务端
    * [TcpServer](#TcpServer)
7. 客户端
    * [Tcpclient](#Tcpclient)

# 日志系统





## Reactor模型介绍


## EventLoop


### 特殊性质
* one loop per thread，每个线程只能有一个EventLoop对象
* 创建了EventLoop对象的线程是IO线程，其主要功能是运行事件循环EventLoop::loop()
* 成员分为两类，一类可以跨线程调用，一类只能在创建了EventLoop的线程中调用

#### EventLoop中未命名的命名空间使用
* 未命名的命名空间中定义的变量拥有静态生命周期，在第一次使用前创建，并且直到程序结束才销毁，类似static关键字
* 一个未命名的命名空间可以在某个给定的文件内不连续，但是不能跨越多个文件
* 如果一个头文件定义了未命名的命名空间，则该命名空间中定义的名字将在每个包含了该头文件的文件中对应不同的实体
* 未命名的命名空间中定义的名字的作用域与该命名空间所在的所用域相同，名字可以直接使用

#### eventfd
* eventfd 是 Linux 的一个系统调用，创建一个文件描述符用于事件通知
本程序中eventfd的使用是为了唤醒阻塞在poll调用中的IO线程。传统的方法是用pipe，IO线程始终监视着此管道的readable事件，在需要唤醒的时候，其他线程往管道里写一个字节，这样IO线程就从IO复用的poll调用中返回。现在使用eventf可以更高效的唤醒
* 创建对象
```cpp
#include <sys/eventfd.h>
int eventfd(unsigned int initval, int flags);
```
对象包含了由内核维护的无符号64位整数计数器 count 。使用参数 initval 初始化此计数器。
flags的可能取值如下：
>EFD_CLOEXEC (since Linux 2.6.27)
>文件被设置成 O_CLOEXEC，创建子进程 (fork) 时不继承父进程的文件描述符。
>EFD_NONBLOCK (since Linux 2.6.27)
>文件被设置成 O_NONBLOCK，执行 read / write 操作时，不会阻塞。
>EFD_SEMAPHORE (since Linux 2.6.30)
>提供类似信号量语义的 read 操作，简单说就是计数值 count 递减 1。

* 读写操作
    * read(): 读取 count 值后置 0。如果设置 EFD_SEMAPHORE，读到的值为 1，同时 count 值递减 1。。如果counter的值为0，非阻塞模式，会直接返回失败，并把errno的值置为EINVAL。如果为阻塞模式，一直会阻塞到counter为非0位置。
    * write(): 其实是执行 add 操作，累加 count 值。如果counter的值达到0xfffffffffffffffe时，就会阻塞。直到counter的值被read。阻塞和非阻塞情况同上面read一样。

#### 对SIGPIPE信号的处理
对于一个对端关闭了的socket进行两次写操作，第一次会收到RST,第二次会产生一个SIGPIPE信号，该信号默认退出进程。
TCP是全双工的信道, 可以看作两条单工信道, TCP连接两端的两个端点各负责一条. 当对端调用close时, 虽然本意是关闭整个两条信道, 但本端只是收到FIN包. 按照TCP协议的语义, 表示对端只是关闭了其所负责的那一条单工信道, 仍然可以继续接收数据. 也就是说, 因为TCP协议的限制, 一个端点无法获知对端已经完全关闭.

假设server和client 已经建立了连接，client 调用了close, 发送FIN 段给server（其实不一定会发送FIN段，close不能保证，只有当某个sockfd的引用计数为0，close 才会发送FIN段，否则只是将引用计数减1而已。也就是说只有当所有进程（可能fork多个子进程都打开了这个套接字）都关闭了这个套接字，close 才会发送FIN 段），此时client不能再通过socket发送和接收数据，此时server调用read，如果接收到FIN 段会返回0。但server此时还是可以write 给client的，write调用只负责把数据交给TCP发送缓冲区就可以成功返回了，所以不会出错，

而client收到数据后应答一个RST段，表示已经不能接收数据，连接重置，server收到RST段后无法立刻通知应用层，只把这个状态保存在TCP协议层。如果server再次调用write发数据给client，由于TCP协议层已经处于RST状态了，因此不会将数据发出，而是发一个SIGPIPE信号给应用层，SIGPIPE信号的缺省处理动作是终止程序。

有时候代码中需要连续多次调用write，可能还来不及调用read得知对方已关闭了连接就被SIGPIPE信号终止掉了，这就需要在初始化时调用sigaction处理SIGPIPE信号，对于这个信号的处理我们通常忽略即可，signal(SIGPIPE, SIG_IGN); 如果SIGPIPE信号没有导致进程异常退出（捕捉信号/忽略信号），write返回-1并且errno为EPIPE（Broken pipe）。（非阻塞地write）
close 关闭了自身数据传输的两个方向。
close把描述符的引用计数减一，仅在该计数变为零时才关闭套接字，而使用shutdown可以不管引用计数就激发TCP的正常连接终止序列
shutdown 可以选择关闭某个方向或者同时关闭两个方向，shutdown how = 0 or how = 1 or how = 2 (SHUT_RD or SHUT_WR or SHUT_RDWR)，后两者可以保证对等方接收到一个EOF字符（即发送了一个FIN段），而不管其他进程是否已经打开了这个套接字。所以说，如果是调用shutdown how = 1 ，则意味着往一个已经发送出FIN的套接字中写是允许的，接收到FIN段仅代表对方不再发送数据，但对方还是可以读取数据的，可以让对方可以继续读取缓冲区剩余的数据。
[参考文献](https://blog.csdn.net/u010982765/article/details/79038062)



#### 编译器处理警告
pragma GCC diagnostic [error|warning|ignored] "-W<警告选项>"
诊断-忽略:(关闭警告)
　　#pragma  GCC diagnostic ignored  "-Wunused"
　　#pragma  GCC diagnostic ignored  "-Wunused-parameter"

诊断-警告:(开启警告)
　　#pragma GCC diagnostic warning "-Wunused" 

　　#pragma GCC diagnostic warning "-Wunused-parameter"

诊断-错误:(开启警告-升级为错误)
　　#pragma GCC diagnostic error "-Wunused" 
　　#pragma GCC diagnostic error "-Wunused-parameter"

[常用警告选项](https://www.cnblogs.com/Dennis-mi/articles/7150321.html)

## EventLoopThreadPool
### 负载均衡算法


## Channel
* 自始至终只属于一个EventLoop，也即只属于一个IO线程
* 负责一个文件描述符的IO时间分发，但不拥有这个文件描述符，不会在析构时关闭这个文件描述符
* 主要功能是把不同的IO事件分发为不同的回调
* Channel在remove自身时，一定要调用disableAll
### 注册事件的函数调用
以可读事件为例
1. Channel::setReadCallback(): 注册回调函数
2. Channel::enableReading(): 开始监测可读事件，调用3
3. Channel::update():把自身加入owner EventLoop中，调用4
4. EventLoop::updateChannel(Channel *)：把Channel加入到Poller的监测列表中，调用5
5. Poller::updateChannel(Channel *)



## Poller
* 实现IO复用的类
* 同时支持poll和epoll
* 是EventLoop的间接成员，只供其owner EventLoop在IO线程调用
* 管理Channel,但不拥有Channel,Channel在析构之前必须自己解除在Poller中的注册



## Acceptor
* 供TcpServer使用，生命期由TcpServer控制
* 功能是用于accept新的Tcp连接，并通过回调通知使用者

###accept函数介绍
```cpp
#include <sys/socket.h>
int accept(int sockfd, struct sockaddr *cliaddr, socklen_t *addrlen);
```
* 返回：若成功则为非负描述符，若出错则为-1
* 第一个参数为监听套接字，返回值为已连接套接字。
* 参数cliaddr和addrlen用来返回已连接的对端进程（客户）的协议地址。
    * 调用前，我们将由*addrlen所引用的整数值置为由cliaddr所指的套接字地址结构的长度
    * 返回时，该整数值即为由内核存放在该套接字地址结构内的确切字节数
* 如果对返回的客户地址不感兴趣，那么可以把cliaddr和addrlen均置为空指针。
* 在监听套接字变得可读时才调用accept

### bind函数介绍
```cpp
#include <sys/socket.h> 
int bind(int sockfd, const struct sockaddr *myaddr, socklen_t addrlen);
```
* 返回： 若成功则为0，若出错则为-1
* 第二个参数是一个指向特定的地址结构的指针第三个参数是该地址结构的长度。
* 调用bind函数可以指定一个端口号，或指定一个IP地址，也可以两者都指定，还可以不指定.
* 服务器在启动时捆绑众所周知端口，客户一般让内核选择临时端口
* Tcp客户通常不把IP地址捆绑到它的套接字上，当连接套接字时，内核将根据所用外出网络接口来选择源IP地址
* 如果TCP服务器没有把IP地址捆绑到它的套接字上，内核就把客户发送的SYN的目的地址作为服务器的源IP地址

|IP地址|端口|结果|
|----|----|----|
|通配地址|0|内核选择IP地址和端口|
|通配地址|非0|内核选择IP地址，进程指定端口|
|本地IP地址|0|进程指定IP地址，内核选择端口|
|本地IP地址|非0|进程指定IP地址和端口|

### listen函数介绍
```cpp
#include <sys/socket.h>
int listen(int sockfd, int backlog);
```
* 返回： 若成功则为0，若出错则为-1
* 第二个参数规定了内核应该为相应套接字队列的最大连接个数。
* 监听套接字的两个队列：未完成连接队列/已完成连接队列
    * 未完成连接队列：每个这样的SYN分节对应其中一项，已由某个客户发出并到达服务器，而服务器正在等待完成相应的TCP三路握手过程。这些套接字处于SYN_RCVD状态。
    * 已完成连接队列：每个已完成TCP三路握手过程的客户对应其中一项。这些套接字处于ESTABLISHED状态。
    * 在未完成连接队列中连接，若在RTT时间内还未完成三次握手，则超时并从该队列中删除
    * 当进程调用accept时，已完成连接队列中的队头项将返回给进程，或者如果该队列为空，那么进程将被投入睡眠，直到TCP在该队列中放入一项才唤醒它。
    * 当一个客户SYN到达时，若这些队列满了，TCP应该忽略这些该字节，而不应该发送RST。因为：这种情况时暂时的，客户TCP将重发SYN，期望不久就能在这些队列中有可用的空间。要是服务器TCP立即响应一个RST，客户的connect调用就会立即返回一个错误，强制应用进程处理这种情况，而不是让TCP的正常重传机制来处理。另外，客户无法区别响应SYN的RST究竟意味着“该端口没有服务器在监听”，还是意味着“该端口有服务器在监听，不过它的队列满了”

### 监听套接字和已连接套接字
* 监听套接字：由socket创建，随后用作bind和listen的第一个参数的描述符
* 已连接套接字：accept的返回值
* 一个服务器通常仅仅创建一个监听套接字，它在该服务器的生命期内一直存在
* 内核为每个由服务器进程接受的客户连接创建一个已连接套接字，当服务器完成对某个给定客户的服务时，相应的已连接套接字就被关闭


### 获取新连接的方式
* 每次accept一个socket
* 每次循环accept，直到没有新连接到达
* 每次尝试accept N个新连接，N的值一般是10
* 后面两种方法适合短连接读物，这里采用的是第一种方法

### 短连接和长连接
[参考资料](https://blog.csdn.net/csdnlijingran/article/details/88343285)
#### Tcp层
* 短链接：指通信双方有数据交互时，就建立一个TCP连接，数据发送完成后，则断开此TCP连接  
	`连接→数据传输→关闭连接→连接→数据传输→关闭连接~~~`
* 长连接：指在一个TCP连接上可以连续发送多个数据包，在TCP连接保持期间，如果没有数据包发送，需要双方发检测包以维持此连接，一般需要自己做在线维持  
`TCP连接→数据包传输→保持连接(心跳)→数据传输→保持连接(心跳)→……→关闭连接`
* 在长连接中一般是没有条件能够判断读写什么时候结束，所以必须要加长度报文头。读函数先是读取报文头的长度，再根据这个长度去读相应长度的报文。
* SO_KEEPALIVE套接字选项: 如果两小时内套接字的任一方向上都没有数据交换，Tcp就自动给对端发送一个保持存活探测分节(keep-alive probe)。会导致以下三种情况发生  
	1. 对端以期望的ACK相应。两小时后继续发送下一个探测分节
	2. 对端以RST响应，表示对端已崩溃并且已重新启动，该套接字会把待处理错误置为ECONNRESET，并关闭套接字
	3. 对端没有响应，将另外发送8个探测分节，两两相隔75秒，试图得到响应。如果没有得到响应则放弃


#### http层
* 在HTTP/1.0中默认使用短连接。也就是说，客户端和服务器每进行一次HTTP操作，就建立一次连接，任务结束就中断连接
* 从HTTP/1.1起，默认使用长连接，用以保持连接特性。当一个网页打开完成后，客户端和服务器之间用于传输HTTP数据的TCP连接不会关闭，客户端再次访问这个服务器时，会继续使用这一条已经建立的连接。
#### 优缺点对比
* 长连接
	* 优点：可以省去较多的TCP建立和关闭的操作，减少浪费，节约时间。
	* 缺点：服务器端有很多TCP连接时，会降低服务器的性能。管理所有的TCP连接，检测是否是无用的连接（长时间没有读写事件），并进行关闭等操作。server端需要采取一些策略，如关闭一些长时间没有读写事件发生的连接，这样可 以避免一些恶意连接导致server端服务受损；如果条件再允许就可以以客户端机器为颗粒度，限制每个客户端的最大长连接数
	* 适用场景： 操作频繁（读写），点对点的通讯，而且连接数不能太多情况
* 短连接
	* 优点：对于服务器来说管理较为简单，存在的连接都是有用的连接，不需要额外的控制手段。
	* 缺点：但如果客户请求频繁，将在TCP的建立和关闭操作上浪费较多时间和带宽。
	* 适用场景：短连接用于并发量大，且每个用户无需频繁向服务器发数据包。 而像WEB网站的http服务一般都用短链接。因为长连接对于服务端来说会耗费一定的资源像，WEB网站这么频繁的成千上万甚至上亿客户端的连接用短连接会更省一些资源。
### EMFILE,文件描述符耗尽处理方法
* 当accept返回EMFILE时，意味着本进程的文件描述符已经达到上限，无法为新连接创建套接字文件描述符。但是，既然没有文件描述符来表示这个连接，也就无法close它，这个新连接会处于一直等待处理的状态。每次epoll_wait都会立刻返回。会使程序陷入busy loop。
* 有7种解决做法，本程序选用的是第6种
    1. 调高进程的文件描述符数目。治标不治本
    2. 死等
    3. 退出程序
    4. 关闭监听套接字
    5. 用ET取代LT,不会陷入无限循环的状态
    6. 提前准备一个空闲的文件描述符，遇到EMFILE时，先关闭这个空闲文件，获得一个文件描述符的名额，再accept拿到新连接的描述符，随后立刻close它，最后再重新打开一个空闲文件。
    7. 自己设置一个稍微低点的文件描述符数量限制，超过这个限制就主动关闭新连接。


## Connector
用于客户端主动发起连接，调用connect，当socket变得可写时表明连接建立完毕
### connect函数介绍
```cpp
#include<sys/socket.h>
int connect(int sockfd, const struct sockaddr *servaddr, socklen_t addrlen);
```
* 返回：若成功则为0，若出错则为-1 
* 第二个参数，第三个参数分别是一个指向要连接的套接字地址结构的指针和该结构的大小。套接字地址结构必须包含有服务器的IP地址和端口号。
* 客户在调用函数connect前不必非得调用bind函数，因为需要的话，内核会确定源IP地址，并选择一个临时端口作为源端口。
* 如果是TCP套接字，调用connect函数将激发TCP的三路握手过程，而且仅在成功或错误时才退出，其中出错返回可能有以下几种情况：
    * 若TCP客户没有收到SYN分节的响应，则返回ETIMEDOUT错误。
    * 若对客户的SYN响应是RST（表示复位），则表明该服务器主机在我们指定的端口上没有进程在等待与之连接（例如服务器进程也许没有运行），返回ECONNREFUSED错误。产生RST的三个条件是：目的地为某端口的SYN到达，然而该端口上没有正在监听的服务器；TCP想取消一个已有连接；TCP接收到一个根本不存在的连接上的分节。
    * 客户发出的SYN在中间的某个路由器上引发目的地不可达。客户主机内核保存该消息，并增大间隔时间重发SYN。若在某个规定的时间后仍未收到响应，则把保存的消息（ICMP错误）作为EHOSTTUNREACH或ENETUNREACH错误返回给进程。
* 若connect失败则套接字不再可用，必须关闭。需要close当前的套接字描述符并重新调用socket。
* 非阻塞套接字调用connect，如果返回0，表示连接已经完成，如果返回-1，那么期望收到的错误是EINPROGRESS，表示连接建立已经启动，但是尚未完成。
* 对于非阻塞套接字调用connect不会立即完成，通常返回错误-1，错误码是EINPROGRESS，我们应该调用select或者poll等待套接字可写，然后使用getsockopt获取错误值，如果等于0就是连接成功。


## TcpConnection
用于表示已经建立的套接字连接
* 表示一次Tcp连接，是不可再生的，一旦连接断开，这个对象就失去了作用
* 没有发起连接的功能，构造参数是已经建立好连接的套接字文件描述符，建立连接的功能在[connector](#Connector)和[accptor](#Acceptor)
* 使用Channel来获取socket上的IO事件，然后自己处理wriable事件，而把readable事件通过MessageCallback传达给客户
* 拥有Tcp已连接套接字，析构函数会关闭fd

### 高水位回调和低水位回调
* 发送缓冲区被清空则调用WriteCompleteCallback
* 输入缓冲区的长度超过指定的大小，就调用HighWaterMarkCallback



## TcpServer
* 为连接创建对应的TcpConnection对象
* 拥有监听套接字listening socket

### 初始化工作


### 建立新连接
* 等待新连接到来，新连接到达时，回调newConnection()
* 调用getNextLoop()来取得EventLoop,并生成新连接的名字
* 创建TcpConnection对象，加入ConncectionMap,设置好回调函数
* 调用EventLoop::runInLoop运行TcpConnection::connectionEstablish()

### 创建线程池
  /// - 0 means all I/O in loop's thread, no thread will created.
  ///   this is the default value.
  /// - 1 means all I/O in another thread.
  /// - N means a thread pool with N threads, new connections
  ///   are assigned on a round-robin basis.



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












## TcpClient
Tcp客户端的实现
* 每个TcpClient只管理一个TcpConnection