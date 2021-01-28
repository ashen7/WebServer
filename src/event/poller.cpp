#include "event/poller.h"

#include <string.h>
#include <assert.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#include <iostream>
#include <queue>

#include "utility/socket_utils.h"
// #include "log/logging.h"

namespace event {

Poller::Poller() {
    //创建epoll内核事件表
    epoll_fd_ = epoll_create1(EPOLL_CLOEXEC); 
    assert(epoll_fd_ > 0);

    event_array_.resize(MAX_EVENTS_NUM);
    channel_array_.resize(MAX_FD_NUM);
    http_array_.resize(MAX_FD_NUM);
}

Poller::~Poller() {
}

// io多路复用 epoll wait返回就绪事件数
std::vector<std::shared_ptr<Channel>> Poller::Poll() {
    while (true) {
        int events_num = epoll_wait(epoll_fd_, &(*event_array_.begin()), MAX_EVENTS_NUM, EPOLL_TIMEOUT);
        if (events_num < 0) {
            perror("epoll wait error");
        }
        
        std::vector<std::shared_ptr<Channel>> channel_array;
        for (int i = 0; i < events_num; ++i) {
            // 获取就绪事件的描述符
            int fd = event_array_[i].data.fd;
            int events = event_array_[i].events;
            auto channel = channel_array_[fd];
            if (channel) {
                //给fd设置就绪的事件
                channel->set_revents(events);
                //给fd设置监听的事件
                channel->set_events(0);
                channel_array.push_back(channel);
            } else {
                // LOG << "Channel is invalid";
            }
        }

        if (channel_array.size() > 0) {
            return channel_array;
        }
    }
}

// 注册新描述符
void Poller::EpollAdd(std::shared_ptr<Channel> channel, int timeout) {
    int fd = channel->fd();
    int events = channel->events();
    if (timeout > 0) {
        AddTimer(channel, timeout);
        http_array_[fd] = channel->holder();
    }

    //注册要监听的事件
    struct epoll_event event;
    event.data.fd = fd;
    event.events = events;
    //更新上一次事件
    channel->update_last_events();
    //将channel添加到channel array中
    channel_array_[fd] = channel;
    
    //注册fd以及监听的事件到epoll内核事件表
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &event) < 0) {
        perror("epoll add error");
        channel_array_[fd].reset();
    }
}

// 修改描述符状态
void Poller::EpollMod(std::shared_ptr<Channel> channel, int timeout) {
    if (timeout > 0) {
        AddTimer(channel, timeout);
    }

    int fd = channel->fd();
    if (!channel->update_last_events()) {
        struct epoll_event event;
        event.data.fd = fd;
        event.events = channel->events();
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &event) < 0) {
            perror("EpollMod error");
            channel_array_[fd].reset();
        }
    }
}

// 从epoll中删除描述符
void Poller::EpollDel(std::shared_ptr<Channel> channel) {
    int fd = channel->fd();
    struct epoll_event event;
    event.data.fd = fd;
    event.events = channel->last_events();
    // event.events = 0;
    // channel->EqualAndUpdateLastEvents()
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, &event) < 0) {
        perror("EpollDel error");
    }
    channel_array_[fd].reset();
    http_array_[fd].reset();
}

void Poller::AddTimer(std::shared_ptr<Channel> channel, int timeout) {
    std::shared_ptr<http::Http> t = channel->holder();
    if (t) {
        timer_heap_.AddTimer(t, timeout);
    } else {
        // LOG << "timer add fail";
    }
}

void Poller::HandleExpire() {
    timer_heap_.HandleExpireEvent();
}

}  // namespace event
