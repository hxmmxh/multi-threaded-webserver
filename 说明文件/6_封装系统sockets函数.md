

- SocketsOps，修改了套接字操作
  - 在sockets命名空间内重写了常用的套接字函数
    - 新建套接字，bind,listen,connect,accept,shutdown, close
    - read,write
    - struct sockaddr_in，struct sockaddr_in6，sockaddr三种套接字地址结构转换
    - 从地址结构中取出IP地址和端口号，或者反过来
- 创建一个InetAddress来封装地址
  - 同时支持IPv4和IPv6地址
  - 能通过端口号，ip地址构造
  - 实现网络字节序和主机字节序的转换
- 创建了一个Socket类来封装原生的套接字
  - 构造函数的参数是一个原生的套接字描述符
  - 析构时会自动关闭这个描述符
  - 提供接口禁用Nagle算法，保活，端口重用












[accpet错误代码](https://blog.csdn.net/21aspnet/article/details/8196671)











[struct hostent](https://blog.csdn.net/sctq8888/article/details/7432092)
```cpp
	struct hostent
	{
		char *h_name;         //正式主机名
		char **h_aliases;     //主机别名
		int h_addrtype;       //主机IP地址类型
		int h_length;		  //主机IP地址字节长度
		char **h_addr_list;	  //主机的IP地址列表
	};
	
	#define h_addr h_addr_list[0]   //保存的是IP地址
```


[tcp_info](https://www.xuebuyuan.com/1810955.html)

```cpp
struct tcp_info {
    __u8 tcpi_state; /* TCP状态 */
__u8 tcpi_ca_state;                            /* TCP拥塞状态 */
__u8 tcpi_retransmits;                         /* 超时重传的次数 */
__u8 tcpi_probes;                              /* 持续定时器或保活定时器发送且未确认的段数*/
__u8 tcpi_backoff;                             /* 退避指数 */
__u8 tcpi_options;                             /* 时间戳选项、SACK选项、窗口扩大选项、ECN选项是否启用*/
__u8 tcpi_snd_wscale : 4, tcpi_rcv_wscale : 4; /* 发送、接收的窗口扩大因子*/

__u32 tcpi_rto;     /* 超时时间，单位为微秒*/
__u32 tcpi_ato;     /* 延时确认的估值，单位为微秒*/
__u32 tcpi_snd_mss; /* 本端的MSS */
__u32 tcpi_rcv_mss; /* 对端的MSS */

__u32 tcpi_unacked; /* 未确认的数据段数，或者current listen backlog */
__u32 tcpi_sacked;  /* SACKed的数据段数，或者listen backlog set in listen() */
__u32 tcpi_lost;    /* 丢失且未恢复的数据段数 */
__u32 tcpi_retrans; /* 重传且未确认的数据段数 */
__u32 tcpi_fackets; /* FACKed的数据段数 */

/* Times. 单位为毫秒 */
__u32 tcpi_last_data_sent; /* 最近一次发送数据包在多久之前 */
__u32 tcpi_last_ack_sent;  /* 不能用。Not remembered, sorry. */
__u32 tcpi_last_data_recv; /* 最近一次接收数据包在多久之前 */
__u32 tcpi_last_ack_recv;  /* 最近一次接收ACK包在多久之前 */

/* Metrics. */
__u32 tcpi_pmtu;         /* 最后一次更新的路径MTU */
__u32 tcpi_rcv_ssthresh; /* current window clamp，rcv_wnd的阈值 */
__u32 tcpi_rtt;          /* 平滑的RTT，单位为微秒 */
__u32 tcpi_rttvar;       /* 四分之一mdev，单位为微秒v */
__u32 tcpi_snd_ssthresh; /* 慢启动阈值 */
__u32 tcpi_snd_cwnd;     /* 拥塞窗口 */
__u32 tcpi_advmss;       /* 本端能接受的MSS上限，在建立连接时用来通告对端 */
__u32 tcpi_reordering;   /* 没有丢包时，可以重新排序的数据段数 */

__u32 tcpi_rcv_rtt;   /* 作为接收端，测出的RTT值，单位为微秒*/
__u32 tcpi_rcv_space; /* 当前接收缓存的大小 */

__u32 tcpi_total_retrans; /* 本连接的总重传个数 */
};
```

#### 
[Nagle算法](https://www.cnblogs.com/lshs/p/6038641.html)
#

