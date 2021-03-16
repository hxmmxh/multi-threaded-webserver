
# 概述

- Webench是一款轻量级的网站测压工具，最多可以对网站模拟3w左右的并发请求
- 可以控制时间、是否使用缓存、是否等待服务器回复等等,能测试相同的服务在不同的硬件的性能和不同服务在相同硬件下的性能。  
- Webbench用C语言编写，运行于linux平台，下载源码后直接编译即可使用，非常迅速快捷。


# 编译与安装
```bash
wget http://www.ha97.com/code/webbench-1.5.tar.gz
tar zxvf webbench-1.5.tar.gz
cd webbench-1.5
make
make install
```

# 使用
- 进行简单测试，不需要安装，直接从网上下载webbench压缩包[下载链接](http://home.tiscali.cz/~cz210552/webbench.html)，解压后，进入文件夹，运行make，就会得到webbench可执行文件。
- 使用时 `./webbench [可选参数] URI `，例如`webbench -c 10 -t 20 http://www.baidu.com/`
- 可选参数包括：
  - 常用的：-t表示运行测试的时间，如果不指定默认是30秒，-c表示客户端数量，也就是并发数。
```
  -f|--force               不需要等待服务端的回复
  -r|--reload              Send reload request - Pragma: no-cache.
  -t|--time <sec>          Run benchmark for <sec> seconds. Default 30.
  -p|--proxy <server:port> Use proxy server for request.
  -c|--clients <n>         Run <n> HTTP clients at once. Default one.
  -9|--http09              Use HTTP/0.9 style requests.
  -1|--http10              Use HTTP/1.0 protocol.
  -2|--http11              Use HTTP/1.1 protocol.
  --get                    Use GET request method.
  --head                   Use HEAD request method.
  --options                Use OPTIONS request method.
  --trace                  Use TRACE request method.
  -?|-h|--help             This information.
  -V|--version             Display program version.
```
- 输出的结果中包含
  - Pages/min：指的输出页数/分 
  - bytes/sec：是指比特/秒
  - susceed和failed表示请求的成功数目和失败数目

# 实现原理
- 父进程fork若干个子进程，每个子进程在用户要求时间或默认的时间内对目标web循环发出实际访问请求
- 父子进程通过管道进行通信，子进程通过管道写端向父进程传递在若干次请求访问完毕后记录到的总信息
- 父进程通过管道读端读取子进程发来的相关信息
- 子进程在时间到后结束，父进程在所有子进程退出后统计并给用户显示最后的测试结果，然后退出

# 源码剖析

## Webbench返回码
- 0，sucess
- 1，benchmark failed (server is not on-line)
- 2，bad param
- 3，internal error, fork failed


## main函数流程
1. 判断参数个数是否正确
2. 调用getopt_long函数解析命令行参数，[getopt_long](#getopt_long)
   1. 得到主机名
3. 测试对端套接字能否成功创建
4. 创建管道
5. 创建子进程


# 重点函数

## getopt_long

- 命令行参数可以分为两类，一类是短选项，一类是长选项，短选项在参数前加一杠"-"，长选项在参数前连续加两杠"--"
- getopt函数只能处理短选项
- getopt_long和getopt_long_only函数两者都能处理
  - getopt_long只将--name当作长参数，但getopt_long_only会将--name和-name两种选项都当作长参数来匹配。
  - 在getopt_long在遇到-name时，会拆解成-n -a -m -e到optstring中进行匹配，而getopt_long_only只在-name不能在longopts中匹配时才将其拆解成-n -a -m -e这样的参数到optstring中进行匹配。
- 使用时需要多次调用getopt函数，直到其返回-1

```cpp
#include <unistd.h>  
extern char *optarg;  
extern int optind, opterr, optopt;  
#include <getopt.h>
int getopt(int argc, char * const argv[],const char *optstring);  
int getopt_long(int argc, char * const argv[], const char *optstring, const struct option *longopts, int *longindex);  
int getopt_long_only(int argc, char * const argv[], const char *optstring, const struct option *longopts, int *longindex);

struct option 
{  
     const char *name;  
     int         has_arg;  
     int        *flag;  
     int         val;  
};
```
- 参数以及返回值介绍（以上三个函数都适用
  - argc和argv和main函数的两个参数一致
  - optstring: 表示短选项字符串
    - 形式如“a:b::cd:“，分别表示程序支持的命令行短选项有-a、-b、-c、-d，冒号含义如下
    - 只有一个字符，不带冒号:只表示选项， 如-c 
    - 一个字符，后接一个冒号:表示选项后面带一个参数，如-a 100
    - 一个字符，后接两个冒号:表示选项后面带一个可选参数，即参数可有可无，如果带参数，则选项与参数直接不能有空格,形式应该如-b200
  - longopts：表示长选项结构体
    - name:表示选项的名称,比如daemon,dir,out等
    - has_arg:表示选项后面是否携带参数。该参数有三个不同值，如下：
      - no_argument(或者是0)时。 参数后面不跟参数值，eg: --version,--help
      - required_argument(或者是1)时。必须带参数，参数输入格式为：--参数 值 或者 --参数=值。eg:--dir=/home
      - optional_argument(或者是2)时。参数是可选的。参数输入格式只能为：--参数=值
    - flag:这个参数有两个意思，空或者非空。
      - 如果参数为空NULL，那么当选中某个长选项的时候，getopt_long将返回val值。           
      - 如果参数不为空，那么当选中某个长选项的时候，getopt_long将返回0，并且将flag指针参数指向val值。
    - val：表示指定函数找到该选项时的返回值，或者当flag非空时指定flag指向的数据的值val。
  - longindex：longindex非空，它指向的变量将记录当前找到参数符合longopts里的第几个元素的描述，即是longopts的下标值
  - 全局变量
    - optarg：表示当前选项对应的参数值。
    - optind：表示的是下一个将被处理到的参数在argv中的下标值。
    - opterr：如果opterr = 0，在getopt、getopt_long、getopt_long_only遇到错误将不会输出错误信息到标准输出流。opterr在非0时，向屏幕输出错误。
    - optopt：表示没有被未标识的选项。
  - 返回值
    - 如果短选项找到，那么将返回短选项对应的字符。
    - 如果长选项找到，如果flag为NULL，返回val。如果flag不为空，返回0
    - 如果遇到一个选项没有在短字符、长字符里面。或者在长字符里面存在二义性的，返回“？”
    - 如果解析完所有字符没有找到（一般是输入命令参数格式错误，eg： 连斜杠都没有加的选项），返回“-1”
    - 如果选项需要参数，忘了添加参数。返回值取决于optstring，如果其第一个字符是“：”，则返回“：”，否则返回“？”。
  - 注意事项 
    - longopts的最后一个元素必须是全0填充，否则会报段错误
    - 短选项中每个选项都是唯一的。而长选项如果简写，也需要保持唯一性。


## sigaction
- 检查或修改与指定信号相关联的处理动作

```c
#include<signal.h>
int sigaction(int signo, const srtuct sigaction * act, strcut sigaction * oact);
//成功返回0，出错返回-1
struct sigaction{
    void (*sa_handler)(int);
    void (*sa_sigaction)(int, siginfo_t*, void *);
    sigset_t sa_mask;
    int sa_flags;
};
// sa_handler和sa_sigaction可能使用同一存储区，只能一次使用这两个字段中的一个
```

- 函数sigaction中参数signo表示要检测或修改其动作的信号编号。若act指针非空，则要修改其动作，若oact指针非空，则oact返回该信号的上一个动作
- 如果sigaciton结构体中的sa_handler字段包含一个信号捕捉函数的地址(不是常量SIG_IGN或SIG_DEL)，则sa_mask说明了一个信号集。在调用该信号捕捉函数之前，这一信号集要加到进程的信号屏蔽字中，当信号捕捉函数返回时，再将进程的信号屏蔽字恢复为原线值。这样的目的是
  - 在调用信号处理程序时阻塞某些信号
  - 在信号处理程序被调用时，新的信号屏蔽字会包括正在递送的信号，保证了在处理一个给定的信号时，如果这种信号再次发生，会被阻塞到对前一个信号的处理结束为止
- 一旦对给定的信号设置了一个动作，那么在调用sigaction显示地改变它之前，该设置一直有效
- sigaction中的sa_flags字段指定对信号进程处理的各个选项，包括有
  - SA_INTERRUPT
  - SA_NOCLDSTOP
  - SA_NOCLDWAIT
  - SA_NODEFER
  - SA_ONSTACK
  - SA_RESETHAND,处理完当前信号后，将信号处理函数设置为SIG_DFL行为
  - SA_RESTART,
  - SA_SIGINFO, 信号处理程序具有附加信息
* 最后sa_sigaction是一个替代的信号处理程序，sa_flags字段包含SA_SIGINFO时,使用该信号处理程序。

```c
struct siginfo{
    int si_signo;   //信号编号
    int si_errno;   //
    int si_code;    //
    pid_t si_pid;   //进程ID
    uid_t si_uid;   //进程实际用户ID
    void *si_addr;  //发生错误的地址
    int si_status;  //退出状态
    union sigval si_value;
};
union sigval{
    int sival_int;
    void *sival_ptr;
};
