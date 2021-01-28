#include "channel.h"

#include <unistd.h>
#include <stdlib.h>

#include <iostream>
#include <queue>

#include "epoll.h"
#include "event_loop.h"
#include "utility/socket_utils.h"

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
    // event_loop_->poller_->epoll_del(fd, events_);
    // close(fd_);
}

void Channel::HandleEvents() {
    events_ = 0;
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
        events_ = 0;
        return;
    }
    
    if (revents_ & EPOLLERR) {
        if (error_handler_)
            error_handler_();
        events_ = 0;
        return;
    }
    
    if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
        HandleRead();
    }
    
    if (revents_ & EPOLLOUT) {
        HandleWrite();
    }

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

