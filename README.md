# 基于C++11主从Reactor模型+线程池的Web高性能服务器

## 简介
* 本项目使用C++11标准编写了一个遵循One Loop Per Thread思想的Web高性能服务器，IO多路复用使用Epoll ET边沿触发工作模式，Socket使用非阻塞IO，网络使用主从Reactor模式+线程池。
主线程也就是主Reactor(MainEventLoop)只负责accept客户端的请求，接收请求后将新连接以Round Robin轮循的方式平均的分发给线程池里的每个线程，
每个线程里都有一个子Reactor(SubEventLoop)，子Reactor(SubEventLoop)负责与客户端进行交互，这里的线程是IO线程，没有创建计算线程，所以IO线程里兼顾计算。
* 使用状态机解析HTTP请求，支持HTTP GET、POST、HEAD请求方法，支持HTTP长连接与短连接。
* 使用小根堆做定时器，惰性删除超时的任务，即客户端没有向服务器发送请求的时间已经超过了我们给定的超时时间，当再次访问它的时候才会去关闭这个连接。
* 实现了双缓冲区的异步日志系统，记录服务器运行状态。
* 使用eventfd实现了线程的异步唤醒，
* 使用智能指针等RAII机制，减少内存泄漏的可能。

## 环境 
* OS: Ubuntu 18.04
* Complier: g++ 7.5.0
* CMake: 3.15.4

## 构建
* 使用cmake来build
    mkdir build 
    cd build
    cmake .. && make -j8 && make install
    cd ..
* 使用Makefile来build
    make -j8 && make install
* 也可以直接运行脚本
    ./build.sh

## 运行
	./web_server [-p port] [-t thread_numbers] [-f log_file_name] -o open_log -s log_to_stderr -c color_log_to_stderr -l min_log_level

## 压力测试
* 本项目对开源压测工具WebBench进行了代码的修改，将其作为了子模块，用git submodule init && git submodule update来添加。修改后的源码[WebBench](https://github.com/ashen7/WebBench.git)
* 修复了WebBench connect()失败时sockfd泄漏的bug，以及接收响应报文时读完了依然read导致阻塞的bug(因为是BIO，读完了再读就会阻塞了)。
* 添加支持HTTP1.1长连接 Connection: keep-alive。

## 压测结果
HTTP长连接 QPS
![长连接](https://github.com/ashen7/WebServer/blob/master/resource/WebServer%E9%95%BF%E8%BF%9E%E6%8E%A5QPS.png)
HTTP短连接 QPS
![短连接](https://github.com/ashen7/WebServer/blob/master/resource/WebServer%E7%9F%AD%E8%BF%9E%E6%8E%A5QPS.png)
WebServer运行时状态
![WebServer运行状态](https://github.com/ashen7/WebServer/blob/master/resource/WebServer%E8%BF%90%E8%A1%8C%E6%97%B6%E7%8A%B6%E6%80%81.png)

