#include "channel.h"

#include <unistd.h>
#include <stdlib.h>

#include <iostream>
#include <queue>

#include "poller.h"
#include "event_loop.h"
#include "utility/socket_utils.h"

namespace event {

Channel::Channel(EventLoop* event_loop)
    : event_loop_(event_loop), 
      fd_(0),
      events_(0), 
      last_events_(0) { 
}

Channel::Channel(EventLoop* event_loop, int fd)
    : event_loop_(event_loop), 
      fd_(fd), 
      events_(0), 
      last_events_(0) {
}

Channel::~Channel() {
    // poller_->epoll_del(fd, events_);
    // close(fd_);
}

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
    //处理连接事件
    HandleConnect();
}

void Channel::HandleConnect() {
    if (connect_handler_) {
        connect_handler_();
    }
}

void Channel::HandleRead() {
    if (read_handler_) {
        read_handler_();
    }
}

void Channel::HandleWrite() {
    if (write_handler_) {
        write_handler_();
    }
}

void Channel::HandleError() {
    if (error_handler_) {
        error_handler_();
    }
}

}  // namespace event