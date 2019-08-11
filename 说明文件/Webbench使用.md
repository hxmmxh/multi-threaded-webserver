webbench介绍和使用方法  

------------------------

# 概述

* Webench是一款轻量级的网站测压工具，最多可以对网站模拟3w左右的并发请求，可以控制时间、是否使用缓存、是否等待服务器回复等等,能测试相同的服务在不同的硬件的性能和不同服务在相同硬件下的性能。  
* Webbench用C语言编写，运行于linux平台，下载源码后直接编译即可使用，非常迅速快捷。
--------------------------

# 使用
* 进行简单测试，不需要安装，直接从网上下载webbench压缩包[下载链接](http://home.tiscali.cz/~cz210552/webbench.html)，解压后，进入文件夹，运行make，就会得到webbench可执行文件。
* 使用时 `./webbench [可选参数] URI `，例如`webbench -c 10 -t 20 http://www.baidu.com/`
* 可选参数包括：
常用的：-t表示运行测试的时间，如果不指定默认是30秒，-c表示客户端数量，也就是并发数。
```
  -f|--force               Don't wait for reply from server.
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
* 输出的结果中包含
    * Pages/min：指的输出页数/分 
    * bytes/sec：是指比特/秒
    * susceed和failed表示请求的成功数目和失败数目


# 实现原理
Webbench实现的核心原理是：父进程fork若干个子进程，每个子进程在用户要求时间或默认的时间内对目标web循环发出实际访问请求，父子进程通过管道进行通信，子进程通过管道写端向父进程传递在若干次请求访问完毕后记录到的总信息，父进程通过管道读端读取子进程发来的相关信息，子进程在时间到后结束，父进程在所有子进程退出后统计并给用户显示最后的测试结果，然后退出