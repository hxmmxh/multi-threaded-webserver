




## 三种函数对比
[三种函数优缺点对比](https://blog.csdn.net/shenya1314/article/details/73691088)
[优缺点对比](https://blog.csdn.net/apacat/article/details/51375950)
### select和poll对比
1. 功能
select 和 poll 的功能基本相同，不过在一些实现细节上有所不同。
* select 会修改描述符，而 poll 不会；
* select 的描述符类型使用数组实现，FD_SETSIZE 大小默认为 1024，因此默认只能监听 1024 个描述符。如果要监听更多描述符的话，需要修改 FD_SETSIZE 之后重新编译；而 poll 的描述符类型使用链表实现，没有描述符数量的限制；
* poll 提供了更多的事件类型，并且对描述符的重复利用上比 select 高。
* 如果一个线程对某个描述符调用了 select 或者 poll，另一个线程关闭了该描述符，会导致调用结果不确定。
2. 速度
select 和 poll 速度都比较慢。
select 和 poll 每次调用都需要将全部描述符从应用进程缓冲区复制到内核缓冲区。
select 和 poll 的返回结果中没有声明哪些描述符已经准备好，所以如果返回值大于 0 时，应用进程都需要使用轮询的方式来找到 I/O 完成的描述符。
3. 可移植性
几乎所有的系统都支持 select，但是只有比较新的系统支持 poll。
### 和epoll对比
* select和poll的缺点
    * 内核 / 用户空间内存拷贝问题，select需要复制大量的句柄数据结构，产生巨大的开销；
    * select返回的是含有整个句柄的数组，应用程序需要遍历整个数组才能发现哪些句柄发生了事件；
    * select的触发方式是水平触发，应用程序如果没有完成对一个已经就绪的文件描述符进行IO操作，那么之后每次select调用还是会将这些文件描述符通知进程。
* epoll优势
    * 更加灵活而且没有描述符数量限制
    * 实现高并发
    * epoll 只需要将描述符从进程缓冲区向内核缓冲区拷贝一次，并且进程不需要通过轮询来获得事件完成的描述符

### 应用场景
* select 可移植性更好，几乎被所有主流平台所支持。
* 只需要运行在 Linux 平台上，有大量的描述符需要同时轮询，并且这些连接最好是长连接。使用epoll
* 需要同时监控小于 1000 个描述符，就没有必要使用 epoll，因为这个应用场景下并不能体现 epoll 的优势。
* 需要监控的描述符状态变化多，而且都是非常短暂的，也没有必要使用 epoll。因为 epoll 中的所有描述符都存储在内核中，造成每次需要对描述符的状态改变都需要通过 epoll_ctl() 进行系统调用，频繁系统调用降低效率。并且epoll 的描述符存储在内核，不容易调试。

-------------------------------------
## select

------------------------------------
## poll


------------------------------------
## epoll

[epoll本质介绍](https://zhuanlan.zhihu.com/p/63179839)
[epoll函数使用](https://blog.csdn.net/ljx0305/article/details/4065058)
[epoll问题](https://blog.csdn.net/PROGRAM_anywhere/article/details/71408708)
[epoll工作原理](https://blog.csdn.net/HDUTigerkin/article/details/7517390)

* 创建epoll文件描述符
```cpp
//创建一个epoll的文件描述符，size用来告诉内核这个监听的数目一共有多大。这个参数不同于select()中的第一个参数，给出最大监听的fd+1的值
//在使用完epoll后，必须调用close()关闭，否则可能导致fd被耗尽。
//成功返回一个非0 的未使用过的最小的文件描述符
//失败返回-1 errno被设置
int epoll_create(int size);
int epoll_create1(int flags);
/*
flags：
0:如果这个参数是0，这个函数等价于poll_create（0）
EPOLL_CLOEXEC：这是这个参数唯一的有效值，如果这个参数设置为这个。那么当进程替换映像的时候会关闭这个文件描述符，这样新的映像中就无法对这个文件描述符操作，适用于多进程编程+映像替换的环境里
*/
 ```
* 注册事件
```c
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
//第一个参数是epoll_create()的返回值
//第二个参数表示动作，用三个宏来表示：
//EPOLL_CTL_ADD：注册新的fd到epfd中；
//EPOLL_CTL_MOD：修改已经注册的fd的监听事件；
//EPOLL_CTL_DEL：从epfd中删除一个fd；
//第三个参数是需要监听的fd，
//第四个参数是告诉内核需要监听什么事
typedef union epoll_data {
    void *ptr;
    int fd;
    __uint32_t u32;
    __uint64_t u64;
} epoll_data_t;

struct epoll_event {
    __uint32_t events; /* Epoll events */
    epoll_data_t data; /* User data variable */
};
```
* events可以是以下几个宏的集合：(类似poll里的事件)
* 对端正常关闭（程序里close()，shell下kill或ctr+c），触发EPOLLIN和EPOLLRDHUP，但是不触发EPOLLERR和EPOLLHUP。
* [参考文献](https://blog.csdn.net/q576709166/article/details/8649911)
>EPOLLIN ：表示对应的文件描述符可以读（包括对端SOCKET正常关闭）；  
>EPOLLOUT：表示对应的文件描述符可以写；  
>EPOLLPRI：表示对应的文件描述符有紧急的数据可读（这里应该表示有带外数据到来）；  
>EPOLLERR：表示对应的文件描述符发生错误；  
>EPOLLHUP：表示对应的文件描述符被挂断；  
>EPOLLET： 将EPOLL设为边缘触发(Edge Triggered)模式，这是相对于水平触发(Level Triggered)来说的。  
>EPOLLONESHOT：只监听一次事件，当监听完这次事件之后，如果还需要继续监听这个socket的话，需要再次把这个socket加入到EPOLL队列里  
>EPOLLRDHUP事件，表示对端的文件描述符正常关闭,在低于2.6.17版本的内核中，这个事件一般是EPOLLIN，即0x1表示连接可读。如果这个事件上层尝试在对端已经close()的连接上读取请求，只能读到EOF，会认为发生异常，报告一个错误。即EPOLLRGHUP等于先收到EPOLLIN,但是read返回0。  

>EPOLLIN   The associated file is available for read(2) operations.  
>EPOLLOUT  The associated file is available for write(2) operations.  
>EPOLLRDHUP  Stream  socket peer closed connection, or shut down writing halfof connection.  (This flag is especially useful for writing sim-ple code to detect peer shutdown when using Edge Triggered moni-toring.)  
>EPOLLPRI  There is urgent data available for read(2) operations.  
>EPOLLERR  Error condition happened  on  the  associated  file  descriptor.epoll_wait(2)  will always wait for this event; it is not neces-sary to set it in events.  
>EPOLLHUP  Hang up happened on the associated   file   descriptor.epoll_wait(2)  will always wait for this event; it is not neces-sary to set it in events.  
>EPOLLET Sets  the  Edge  Triggered  behavior  for  the  associated  file descriptor.   The default behavior for epoll is Level Triggered.See epoll(7) for more detailed information about Edge and  LevelTriggered event distribution architectures.
> EPOLLONESHOT (since Linux 2.6.2) 
              Sets  the  one-shot behavior for the associated file descriptor.
              This means that after an event is pulled out with  epoll_wait(2)
              the  associated  file  descriptor  is internally disabled and no
              other events will be reported by the epoll interface.  The  user
              must  call  epoll_ctl() with EPOLL_CTL_MOD to re-enable the file
              descriptor with a new event mask. 
>

* 等待事件
```cpp
//第一个参数是epoll_create()的返回值
//参数events用来从内核得到事件的集合
//maxevents告之内核这个events有多大，这个 maxevents的值不能大于创建epoll_create()时的size
//参数timeout是超时时间（毫秒，0会立即返回，-1将不确定，也有说法说是永久阻塞）。
//返回需要处理的事件数目
int epoll_wait(int epfd, struct epoll_event * events, int maxevents, int timeout);
```


* ET和LT模式
[参考文献](https://blog.csdn.net/yusiguyuan/article/details/15027821)
ET模式仅当状态发生变化的时候才获得通知,这里所谓的状态的变化并不包括缓冲区中还有未处理的数据,也就是说,如果要采用ET模式,需要一直read/write直到出错为止,很多人反映为什么采用ET模式只接收了一部分数据就再也得不到通知了,大多因为这样;而LT模式是只要有数据没有处理就会一直通知下去的.
Edge Triggered(ET)       //高速工作方式，错误率比较大，只支持no_block socket (非阻塞socket)
LevelTriggered(LT)       //缺省工作方式，即默认的工作方式,支持blocksocket和no_blocksocket，错误率比较小。

最好以下面的方式调用ET模式的epoll接口，在后面会介绍避免可能的缺陷。(LT方式可以解决这种缺陷)  i   基于非阻塞文件句柄  ii  只有当read(2)或者write(2)返回EAGAIN时(认为读完)才需要挂起，等待。但这并不是说每次read()时都需要循环读，直到读到产生一个EAGAIN才认为此次事件处理完成，当read()返回的读到的数据长度小于请求的数据长度时(即小于sizeof(buf))，就可以确定此时缓冲中已没有数据了，也就可以认为此事读事件已处理完成。

#### 机制实现
当某一进程调用epoll_create方法时，Linux内核会创建一个eventpoll结构体，这个结构体中有两个成员与epoll的使用方式密切相关。eventpoll结构体如下所示：

```cpp
struct eventpoll{
    /*红黑树的根节点，这颗树中存储着所有添加到epoll中的需要监控的事件*/
    struct rb_root  rbr;
    /*双链表中则存放着将要通过epoll_wait返回给用户的满足条件的事件*/
    struct list_head rdlist;
};
```
* 每一个epoll对象都有一个独立的eventpoll结构体，用于存放通过epoll_ctl方法向epoll对象中添加进来的事件。这些事件都会挂载在红黑树中，如此，重复添加的事件就可以通过红黑树而高效的识别出来(红黑树的插入时间效率是lgn，其中n为树的高度)。
* 而所有添加到epoll中的事件都会与设备(网卡)驱动程序建立回调关系，也就是说，当相应的事件发生时会调用这个回调方法。这个回调方法在内核中叫ep_poll_callback,它会将发生的事件添加到rdlist双链表中。
* 当调用epoll_wait检查是否有事件发生时，只需要检查eventpoll对象中的rdlist双链表中是否有epitem元素即可。如果rdlist不为空，则把发生的事件复制到用户态，同时将事件数量返回给用户





