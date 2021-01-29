#include "event/channel.h"

#include <unistd.h>
#include <stdlib.h>

#include <iostream>
#include <queue>

#include "event/poller.h"
#include "event/event_loop.h"

namespace event {

Channel::Channel() {
    fd_ = 0;
    events_ = 0;
    last_events_ = 0;  
}

Channel::Channel(int fd) {
    fd_ = fd;
    events_ = 0;
    last_events_ = 0; 
}

Channel::~Channel() {
}

//IO事件的回调函数 EventLoop中调用Loop开始事件循环 会调用Poll得到就绪事件 
//然后依次调用此函数处理就绪事件
void Channel::HandleEvents() {
    events_ = 0;
    //触发挂起事件 并且没触发可读事件
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
        events_ = 0;
        return;
    }
    //触发错误事件
    if (revents_ & EPOLLERR) {
        HandleError();
        events_ = 0;
        return;
    }
    
    //触发可读事件 | 高优先级可读 | 对端（客户端）关闭连接
    if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
        HandleRead();
    }
    //触发可写事件
    if (revents_ & EPOLLOUT) {
        HandleWrite();
    }
    //不管就绪了什么事件 最后都要处理连接事件
    HandleConnect();
}

//处理读事件的回调函数
void Channel::HandleRead() {
    if (read_handler_) {
        read_handler_();
    }
}

//处理写事件的回调函数
void Channel::HandleWrite() {
    if (write_handler_) {
        write_handler_();
    }
}

//处理连接事件的回调函数
void Channel::HandleConnect() {
    if (connect_handler_) {
        connect_handler_();
    }
}

//处理错误事件的回调函数
void Channel::HandleError() {
    if (error_handler_) {
        error_handler_();
    }
}

}  // namespace event