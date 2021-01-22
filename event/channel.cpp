#include "channel.h"

#include <unistd.h>
#include <stdlib.h>

#include <iostream>
#include <queue>

#include "epoll.h"
#include "event_loop.h"
#include "utility/utils.h"

Channel::Channel(EventLoop* loop)
    : loop_(loop), events_(0), lastEvents_(0), fd_(0) {}

Channel::Channel(EventLoop* loop, int fd)
    : loop_(loop), fd_(fd), events_(0), lastEvents_(0) {}

Channel::~Channel() {
    // loop_->poller_->epoll_del(fd, events_);
    // close(fd_);
}

void Channel::HandleEvents() {
    events_ = 0;
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
        events_ = 0;
        return;
    }
    if (revents_ & EPOLLERR) {
        if (errorHandler_)
            errorHandler_();
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

void Channel::HandleConn() {
    if (connect_handler_) {
        connect_handler_();
    }
}