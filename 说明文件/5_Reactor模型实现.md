
-----------------------------------
# Reactor模型介绍
程序使用Reactor模型，并使用多线程提高并发度。为避免线程频繁创建和销毁带来的开销，使用线程池，在程序的开始创建固定数量的线程。使用epoll或poll作为IO多路复用的实现方式

## Reactor和Proactor区别

- Reactor模式是同步I/O，处理I/O操作的依旧是产生I/O的程序；Proactor是异步I/O，产生I/O调用的用户进程不会等待I/O发生，具体I/O操作由操作系统完成。
- 异步I/O需要操作系统支持，Linux异步I/O为AIO，Windows为IOCP。
- Reactor
  - 应用程序注册读就需事件和相关联的事件处理器。
  - 事件分离器等待事件的发生。
  - 当发生读就需事件的时候，事件分离器调用第一步注册的事件处理器。
  - 事件处理器首先执行实际的读取操作，然后根据读取到的内容进行进一步的处理。
- Proactor
  - 应用程序初始化一个异步读取操作，然后注册相应的事件处理器，此时事件处理器不关注读取就绪事件，而是关注读取完成事件，这是区别于Reactor的关键。
  - 事件分离器等待读取操作完成事件。
  - 在事件分离器等待读取操作完成的时候，操作系统调用内核线程完成读取操作，并将读取的内容放入用户传递过来的缓存区中。这也是区别于Reactor的一点，在Proactor中，应用程序需要传递缓存区。
  - 事件分离器捕获到读取完成事件后，激活应用程序注册的事件处理器，事件处理器直接从缓存区读取数据，而不需要进行实际的读取操作。
- 从上面可以看出，Reactor和Proactor模式的主要区别就是真正的读取和写入操作是有谁来完成的，Reactor中需要应用程序自己读取或者写入数据，而Proactor模式中，应用程序不需要进行实际的读写过程，操作系统会读取缓存区或者写入缓存区到真正的IO设备。

## 本次项目使用的模型

- 事件循环+线程池


# 多线程模型介绍





## 多线程服务器的使用场合

### 必须用单线程的场合

- 程序可能会fork
- 限制程序的CPU占用率
- 优点：简单
- 缺点: 非抢占的

### 适用于多线程程序的场景

- 提高响应速度，让IO和计算相互重叠，降低延迟
- 多线程程序要满足的条件
  - 有多个CPU可以用
  - 线程之间有共享的数据，且共享数据可以修改
  - 提供给均质的服务，事件的响应有优先级差异，防止优先级反转
  - 能有效的划分责任和功能，让每个线程的逻辑比较简单，任务单一



- 处理并发连接

------------------------------------
# EventLoop

## 特殊性质
- one loop per thread，每个线程只能有一个EventLoop对象
- 创建了EventLoop对象的线程是IO线程，其主要功能是运行事件循环EventLoop::loop()
- 成员分为两类
  - 一类可以跨线程调用
  - 一类只能在创建了EventLoop的线程中调用，使用前要用assertInLoopThread()断言
- 意味着每个线程只能有一个EventLoop对象，EventLoop即是时间循环，每次从poller里拿活跃事件，并给到Channel里分发处理。EventLoop中的loop函数会在最底层(Thread)中被真正调用，开始无限的循环，直到某一轮的检查到退出状态后从底层一层一层的退出。

## 使用流程

- 初始化
  - 分配eventfd, 并注册wakeupChannel。
    - 因为这个Channel的作用是用来唤醒这个线程，所以在初始化时就要注册
  - 创建定时器序列
  - 创建一个Poller
- 注销
  - 停止监控wakeupChannel，并关闭对应的eventfd



## EventLoop中未命名的命名空间使用
- 未命名的命名空间中定义的变量拥有静态生命周期，在第一次使用前创建，并且直到程序结束才销毁，类似static关键字
- 一个未命名的命名空间可以在某个给定的文件内不连续，但是不能跨越多个文件
- 如果一个头文件定义了未命名的命名空间，则该命名空间中定义的名字将在每个包含了该头文件的文件中对应不同的实体
- 未命名的命名空间中定义的名字的作用域与该命名空间所在的所用域相同，名字可以直接使用

## eventfd
- eventfd 是 Linux 的一个系统调用，创建一个文件描述符用于事件通知
- 本程序中eventfd的使用是为了唤醒阻塞在poll调用中的IO线程。传统的方法是用pipe，IO线程始终监视着此管道的readable事件，在需要唤醒的时候，其他线程往管道里写一个字节，这样IO线程就从IO复用的poll调用中返回。现在使用eventf可以更高效的唤醒
- 创建对象
  - 对象包含了由内核维护的无符号64位整数计数器 count 。使用参数 initval 初始化此计数器。
  - flags的可能取值如下：
    - EFD_CLOEXEC，文件被设置成 O_CLOEXEC，创建子进程 (fork) 时不继承父进程的文件描述符。
    - EFD_NONBLOCK，文件被设置成 O_NONBLOCK，执行 read / write 操作时，不会阻塞。
    - EFD_SEMAPHORE， 提供类似信号量语义的 read 操作，简单说就是计数值 count 递减 1。
```cpp
#include <sys/eventfd.h>
int eventfd(unsigned int initval, int flags);
```
- read()
  - read函数会从eventfd对应的64位计数器中读取一个8字节的整型变量，返回的值是按小端字节序的
  - 如果eventfd设置了EFD_SEMAPHORE，那么每次read就会返回1，并且让eventfd对应的计数器减一
  - 如果eventfd没有设置EFD_SEMAPHORE，那么每次read就会直接返回计数器中的数值，read之后计数器就会置0
  - 当计数器为0时，如果继续read，那么read就会阻塞（如果eventfd没有设置EFD_NONBLOCK）或者返回EAGAIN错误（如果eventfd设置了EFD_NONBLOCK）
- write():
  - 在没有设置EFD_SEMAPHORE的情况下，write函数会将发送buf中的数据写入到eventfd对应的计数器中，最大只能写入0xffffffffffffffff，否则返回EINVAL错误
  - 在设置了EFD_SEMAPHORE的情况下，write函数相当于是向计数器中进行“添加”，比如说计数器中的值原本是2，如果write了一个3，那么计数器中的值就变成了5。其实是执行 add 操作，累加 count 值。如果counter的值达到0xfffffffffffffffe时，就会阻塞。直到counter的值被read。
  - 阻塞和非阻塞情况同上面read一样。

## 对SIGPIPE信号的处理
- 对于一个对端关闭了的socket进行两次写操作，第一次会收到RST,第二次会产生一个SIGPIPE信号，该信号默认退出进程。
- TCP是全双工的信道, 可以看作两条单工信道, TCP连接两端的两个端点各负责一条. 当对端调用close时, 虽然本意是关闭整个两条信道, 但本端只是收到FIN包. 按照TCP协议的语义, 表示对端只是关闭了其所负责的那一条单工信道, 仍然可以继续接收数据. 也就是说, 因为TCP协议的限制, 一个端点无法获知对端已经完全关闭.
- 假设server和client 已经建立了连接，client 调用了close, 发送FIN 段给server（其实不一定会发送FIN段，close不能保证，只有当某个sockfd的引用计数为0，close 才会发送FIN段，否则只是将引用计数减1而已。也就是说只有当所有进程（可能fork多个子进程都打开了这个套接字）都关闭了这个套接字，close 才会发送FIN 段），此时client不能再通过socket发送和接收数据，此时server调用read，如果接收到FIN 段会返回0。但server此时还是可以write 给client的，write调用只负责把数据交给TCP发送缓冲区就可以成功返回了，所以不会出错，
- 而client收到数据后应答一个RST段，表示已经不能接收数据，连接重置，server收到RST段后无法立刻通知应用层，只把这个状态保存在TCP协议层。如果server再次调用write发数据给client，由于TCP协议层已经处于RST状态了，因此不会将数据发出，而是发一个SIGPIPE信号给应用层，SIGPIPE信号的缺省处理动作是终止程序。
- 有时候代码中需要连续多次调用write，可能还来不及调用read得知对方已关闭了连接就被SIGPIPE信号终止掉了，这就需要在初始化时调用sigaction处理SIGPIPE信号，对于这个信号的处理我们通常忽略即可，signal(SIGPIPE, SIG_IGN); 如果SIGPIPE信号没有导致进程异常退出（捕捉信号/忽略信号），write返回-1并且errno为EPIPE（Broken pipe）。（非阻塞地write）
- close 关闭了自身数据传输的两个方向。
- close把描述符的引用计数减一，仅在该计数变为零时才关闭套接字，而使用shutdown可以不管引用计数就激发TCP的正常连接终止序列
- shutdown 可以选择关闭某个方向或者同时关闭两个方向，shutdown how = 0 or how = 1 or how = 2 (SHUT_RD or SHUT_WR or SHUT_RDWR)，后两者可以保证对等方接收到一个EOF字符（即发送了一个FIN段），而不管其他进程是否已经打开了这个套接字。所以说，如果是调用shutdown how = 1 ，则意味着往一个已经发送出FIN的套接字中写是允许的，接收到FIN段仅代表对方不再发送数据，但对方还是可以读取数据的，可以让对方可以继续读取缓冲区剩余的数据。
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



----------------------------------
# Channel
  
- 自始至终只属于一个EventLoop，也即只属于一个IO线程
- 负责一个文件描述符的IO事件分发，但不拥有这个文件描述符，不会在析构时关闭这个文件描述符
- 主要功能是把不同的IO事件分发为不同的回调
- Channel在remove自身时，一定要调用disableAll（注销所有的监测事件）
- 它自始至终都属于一个EventLoop，负责一个文件描述符的IO事件，在Channel类中保存这IO事件的类型以及对应的回调函数，当IO事件发生时，最终会调用到Channel类中的回调函数
  
## 注册事件的函数调用
以可读事件为例
1. Channel::setReadCallback(): 注册回调函数
2. Channel::enableReading(): 开始监测可读事件，调用3
3. Channel::update():把自身加入owner EventLoop中，调用4
4. EventLoop::updateChannel(Channel *)：把Channel加入到Poller的监测列表中，调用5
5. Poller::updateChannel(Channel *)


----------------------------------------
## Poller
- 实现IO复用的类
- 同时支持poll和epoll[IO复用函数介绍](IO复用.md)
- 每个Poller都是一个EventLoop的成员
  - EventLoop对象初始化时会创建一个Poller
  - 只供其owner EventLoop在IO线程调用
- 管理Channel,但不拥有Channel,Channel在析构之前必须自己解除在Poller中的注册
  - `map<int, Channel *> channels_`成员保存所有注册的Channel


---------------------------------------------
## 多线程EventLoop
一般而言，多线程服务器中的线程可分为以下几类：
    * IO线程(负责网络IO)
    * 计算线程(负责复杂计算)
    * 第三方库所用线程
本程序中的Log线程属于第三种，其它线程属于IO线程，因为Web静态服务器计算量较小，所以没有分配计算线程，减少跨线程分配的开销，让IO线程兼顾计算任务。除Log线程外，每个线程一个事件循环，遵循One loop per thread。
## EventLoopThread

## EventLoopThreadPool
### 负载均衡算法