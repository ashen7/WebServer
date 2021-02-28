# 基于C++11主从Reactor模式的Web高性能服务器

## 简介
* 本项目使用C++11标准编写了一个遵循One Loop Per Thread思想的Web高性能服务器。
* IO多路复用使用Epoll ET边沿触发工作模式，Socket使用非阻塞IO，网络使用主从Reactor模式+线程池。
* 主线程也就是主Reactor(MainEventLoop)只负责accept客户端的请求，接收请求后将新连接以Round Robin轮循的方式平均的分发给线程池里的每个线程，
  每个线程里都有一个子Reactor(SubEventLoop)，子Reactor(SubEventLoop)负责与客户端进行交互，这里的线程是IO线程，没有创建计算线程，所以IO线程里兼顾计算。
* 使用状态机解析HTTP请求，支持HTTP GET、POST、HEAD请求方法，支持HTTP长连接与短连接。
* 使用小根堆做定时器，惰性删除超时的任务，即客户端没有向服务器发送请求的时间已经超过了我们给定的超时时间，当再次访问它的时候才会去关闭这个连接。
* 实现了双缓冲区的异步日志系统，记录服务器运行状态。
* 使用智能指针等RAII机制，减少内存泄漏的可能。
* 使用eventfd实现了线程的异步唤醒，每个EventLoop将eventfd注册到epoll内核表中，当新事件被accept后分发到某个SubEventLoop，
但此时没有就绪事件Loop阻塞在epoll_wait中，主线程向eventfd中写入数据，此时Loop的epoll_wait因有可读事件就绪而被唤醒，去添加新连接。

## 环境 
* OS: Ubuntu 18.04
* Complier: g++ 7.5.0
* CMake: 3.15.4
* Makefile: 4.1 

## 构建
* 使用CMake来build
    mkdir build 
    cd build
    cmake .. && make -j8 && make install
    cd ..

* 使用Makefile来build
    make -j8 && make install

* 直接运行脚本
    ./build.sh

## 运行
	./web_server [-p port] [-t thread_numbers] [-f log_file_name] [-o open_log] [-s log_to_stderr] [-c color_log_to_stderr] [-l min_log_level]

## 压力测试
* 本项目对开源压测工具WebBench进行了代码的修改，将其作为了子模块，用git submodule init && git submodule update来添加。修改后的源码:[WebBench](https://github.com/ashen7/WebBench.git)
* 修复了WebBench connect()失败时sockfd泄漏的bug，以及接收响应报文时读完了依然read导致阻塞的bug(因为是BIO，读完了再读就会阻塞了)。
* 添加支持HTTP1.1长连接 Connection: keep-alive。

## 压测结果
压力测试开启1000个进程，访问服务器60s，过程是客户端发出请求，然后服务器读取并解析，返回响应报文，客户端读取。
长连接因为不必频繁的创建新套接字去请求，然后读取数据，比短连接QPS高很多。
HTTP长连接 QPS: 26万
![长连接](https://github.com/ashen7/WebServer/blob/master/resource/WebServer%E9%95%BF%E8%BF%9E%E6%8E%A5QPS.png)
HTTP短连接 QPS：2万8
![短连接](https://github.com/ashen7/WebServer/blob/master/resource/WebServer%E7%9F%AD%E8%BF%9E%E6%8E%A5QPS.png)
压力测试HTTP短连接时 WebServer的状态
![WebServer运行状态](https://github.com/ashen7/WebServer/blob/master/resource/WebServer%E8%BF%90%E8%A1%8C%E6%97%B6%E7%8A%B6%E6%80%81.png)

## 代码结构
* include/server和src/server: WebServer主接口
    web_server.h/web_server.cpp: server::WebServer类，单例模式(线程安全的懒汉式)，由主线程调用

    Initialize(绑定服务器，设置监听套接字为NIO，给监听套接字的Channel绑定回调函数, 创建EventLoop线程池)

    Start(线程池创建线程，每个线程就是一个SubEventLoop，线程函数就是Loop函数(epoll_wait等待处理就绪事件)，
          给监听套接字绑定可读回调事件(while true的accept连接，对新套接字关闭TCP Nagle算法(小数据不用等着一起发，而是直接发)，
          将此新连接以轮转的方式分发给线程池中其中一个SubEventLoop，由于此时SubEventLoop正在epoll_wait中阻塞，
          这里主线程通过QueueInLoop(学习muduo做法，内部通过eventfd将其异步唤醒)，然后SubEventLoop将此新连接绑定回调函数(处理http连接))

* include/event和src/event: 主从Reactor模式+线程池
    channel.h/channel.cpp: event::Channel类 
        给不同的文件描述符绑定不同的可读/可写/更新/错误回调函数(主要是监听套接字和连接套接字)

    poller.h/poller.cpp: event::Poller类
        封装了epoll的操作(后续会支持poll和select，在所有连接都活跃的情况下，使用epoll并没有poll或select性能好)

    event_loop.h/event_loop.cpp: event::EventLoop类
        每个EventLoop都有一个Poller(epoll内核事件表)
        Loop(epoll_wait等待就绪事件的到来，有就绪事件后处理就绪事件, 处理PendingFunctions(学习muduo), 处理超时事件)
        RunInLoop(如果不是跨线程调用就直接执行，否则放入等待执行函数区)
        QueueInLoop(将任务放入等待执行函数区，然后用eventfd异步唤醒SubEventLoop的epoll_wait, Loop里就会处理这个等待执行函数区)
        PerformPendingFunctions(执行等待执行函数区的函数，这里的func是将新连接注册到这个SubEventLoop的epoll内核事件表的)

    event_loop_thread.h/event_loop_thread.cpp, event_loop_thread_pool.h/event_loop_thread_pool.cpp: event::EventLoopThreadPool类
        event_loop的线程池，每个线程里一个SubEventLoop，SubEventLoop调用Loop函数

* include/http_connection.h和src/http_connection.cpp: HTTP连接的处理
    http_connection.h/http_connection.cpp: http::HttpConnection类
        读取客户端发来的请求数据存到read_buffer_(read)
        解析请求行，解析请求头，解析请求体(POST方法), 构建响应报文，将响应报文写入write_buffer_(将请求的文件内容通过mmap映射到一块共享内存中，加快访问速度)
        将write_buffer_中的响应报文数据发送给客户端(write)

    http_type.h/http_type.cpp: http::HttpType类
        定义一些http的enum

* include/timer和src/timer: 用小根堆管理定时器
    timer.h/timer.cpp: timer::Timer类
        通过gettimeofday函数得到当前时间以毫秒计算，到期时间=当前秒%10000然后*1000(换算成毫秒)，加上当前微秒/1000(也是换算成毫秒), 加上我们设定的超时时间
        每次处理完请求后会重新计时(这里是将到期时间用当前时间再重算一遍)

    timer_heap.h/timer_heap.cpp: timer::TimerHeap类
        使用C++的优先级队列，小根堆管理定时器，小根堆的堆头是会最先到期的定时器，所以每次只有pop头，O(1)的删除，O(logN)的插入
        每次Loop最后会去检测有无连接超时，超时则删除，这里是惰性删除(检查之前可能已经超时了但没有主动去删)

* include/log和src/log: 双缓冲区异步日志系统
    log_stream.h/log_stream.cpp: Buffer类, log::LogStream类
        设计了一个固定大小的缓冲区Buffer类(模板类，模板参数是buffer size)
        LogStream则是重载输出流运算符<<, <<后面接的数据都写入Buffer中，重载了长短int, float, double, char, const char*, string等数据类型

    async_logging.h/async_logging.cpp: log::AsyncLogging类
        创建一个线程将两个Buffer中数据写入日志(pthread_once只会调用一次的函数，这里用线程的原因是写入内存速度很快，但是涉及到磁盘IO，
        速度就很慢了，所以其他线程只负责把日志写入Buffer，这个日志线程会把Buffer中的数据写入日志，用两个缓冲区是因为一个Buffer可能很快就写满了，此时就得用空闲的与它交换指针)

    logging.h/logging.cpp: log::Logging类
        构造函数通过LogStream对象将当前时间写入Buffer，析构时通过LogStream对象将日志内容写入Buffer
        这里模仿Google的glog，实现了对日志等级的划分，以及是否打开日志，将日志内容打印到屏幕，颜色输出日志等

* include/thread和src/thread: 线程池
    thread.h/thread.cpp: thread::Thread类
        用到了TLS线程局部存储，每个线程一个线程id

    thread_pool.h/thread_pool.cpp: thread::ThreadPool类
        实现阻塞队列 + 线程池

* include/locker: 封装互斥锁RAII机制
    mutex_lock.h: locker::MutexLock类, locker::LockGuard类, locker::ConditionVariable类
        对互斥锁和条件变量进行了封装，并提供了RAII机制的访问(互斥锁析构时先要加锁，再释放锁)

* include/utility: 封装Socket操作
    socket_utils.h: 
        套接字设置非阻塞，套接字关闭TCP Nagle算法，优雅关闭套接字，封装Read，Write等

    noncopyable.h:
        将拷贝构造函数和赋值运算符重载给设置为private或者C++11语法使用delete
        子类继承这个类，这个子类就不能使用拷贝构造函数或赋值运算符了
