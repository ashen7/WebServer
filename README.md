# A C++ High Performance Web Server

## Introduction  
* 本项目是用C++11语法编写的Web服务器，支持HTTP GET、POST、Head请求方法，支持HTTP长连接与短连接，实现了4缓冲区异步日志，记录服务器运行状态。
* 小根堆定时器，异步日志系统，互斥锁条件变量封装，HTTP连接，Reactor(Channel, Poller, EventLoop), WebServer(主接口)等链接成了libevent库，可供main调用

## Envoirment  
* OS: Ubuntu 18.04
* Complier: g++ 7.5.0

## Build
* 使用cmake来build
    mkdir build 
    cd build
    cmake .. && make -j8 && make install
    cd ..

* 使用Makefile来build
    make -j8 && make install

* 也可以直接运行脚本
    ./build.sh

## Usage
	./web_server [-p port] [-t thread_numbers] [-l log_file_name]

## Concurrency model
并发模型为Reactor+非阻塞IO+线程池，新连接Round Robin分配

## Technical points
* 使用Epoll边沿触发的IO多路复用技术，非阻塞IO，使用Reactor模式
* 使用多线程充分利用多核CPU，并使用线程池避免线程频繁创建销毁的开销
* 使用基于小根堆的定时器关闭超时请求
* 主线程只负责accept请求，并以Round Robin的方式分发给其它IO线程(该IO线程也兼计算线程)，锁的争用只会出现在主线程和某一特定线程中
* 使用eventfd实现了线程的异步唤醒
* 使用四个缓冲区实现了简单的异步日志系统
* 为减少内存泄漏的可能，使用智能指针等RAII机制
* 使用状态机解析了HTTP请求,支持管线化
* 支持优雅关闭连接
 
## Stress Testing
* 对开源压测工具WebBench进行了代码的修改，写了一份c++11的版本
* 修复了WebBench connect()失败时sockfd泄漏的bug，以及连续read二次阻塞的bug。
* 支持HTTP1.1 Connection: keep-alive。
* WebBench是本项目的submodule, 用git submodule init && git submodule update来添加
