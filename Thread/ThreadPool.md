



### 严重bug

如果在ThreadPool对象调用start后迅速调用其析构函数可能会丢失任务
程序分析如下：
1. ThreadPool对象调用start(n)后，创建n个新线程，每个线程运行runInThread
2. runInThread函数内会判断running_，只有当其为真时，才会通过task()取出任务队列中的任务运行
3. 如果runInThread运行过慢，在判断running_之前，主程序调用了stop()，会把running_设为flase
4. 这就导致在任务队列中任务无法被取出
5. 解决方法，在stop函数中加上一条usleep(10),等待10微妙再改变running_