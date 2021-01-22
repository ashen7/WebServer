#include "channel.h"

#include <unistd.h>
#include <cstdlib>
#include <iostream>

#include <queue>

#include "epoll.h"
#include "event_loop.h"
#include "util.h"

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
        handleRead();
    }
    if (revents_ & EPOLLOUT) {
        handleWrite();
    }
    handleConn();
}

void Channel::HandleRead() {
    if (readHandler_) {
        readHandler_();
    }
}

void Channel::HandleWrite() {
    if (writeHandler_) {
        writeHandler_();
    }
}

void Channel::HandleConn() {
    if (connHandler_) {
        connHandler_();
    }
}