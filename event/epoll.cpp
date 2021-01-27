#include "epoll.h"

#include <string.h>
#include <assert.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#include <iostream>
#include <queue>

#include "log/logging.h"
#include "utility/socket_utils.h"

Epoll::Epoll() 
    : epoll_fd_(epoll_create1(EPOLL_CLOEXEC)), 
      events_(MAX_EVENTS_NUM) {
    assert(epoll_fd_ > 0);
}

Epoll::~Epoll() {
}

// epoll wait返回就绪事件数
std::vector<std::shared_ptr<Channel>> Epoll::Poll() {
    while (true) {
        int events_num = epoll_wait(epoll_fd_, &*events_.begin(),
                                   events_.size(), EPOLL_TIMEOUT);
        if (events_num < 0) {
            perror("epoll wait error");
        }
        
        auto channel_array = GetEventsRequest(events_num);
        if (channel_array.size() > 0) {
            return channel_array;
        }
    }
}

// 分发处理函数
std::vector<std::shared_ptr<Channel>> Epoll::GetEventsRequest(int events_num) {
    std::vector<std::shared_ptr<Channel>> channel_array;

    for (int i = 0; i < events_num; ++i) {
        // 获取就绪事件的描述符
        int fd = events_[i].data.fd;
        std::shared_ptr<Channel> channel = channel_[fd];

        if (channel) {
            channel->set_revents(events_[i].events);
            channel->set_events(0);
            channel_array.push_back(channel);
        } else {
            LOG << "Channel is invalid";
        }
    }

    return channel_array;
}

// 注册新描述符
void Epoll::EpollAdd(std::shared_ptr<Channel> channel, int timeout) {
    int fd = channel->get_fd();
    if (timeout > 0) {
        AddTimer(channel, timeout);
        http_[fd] = channel->get_holder();
    }
    struct epoll_event event;
    event.data.fd = fd;
    event.events = channel->get_events();

    channel->EqualAndUpdateLastEvents();

    channel_[fd] = channel;
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &event) < 0) {
        perror("EpollAdd error");
        channel_[fd].reset();
    }
}

// 修改描述符状态
void Epoll::EpollMod(std::shared_ptr<Channel> channel, int timeout) {
    if (timeout > 0) {
        AddTimer(channel, timeout);
    }

    int fd = channel->get_fd();
    if (!channel->EqualAndUpdateLastEvents()) {
        struct epoll_event event;
        event.data.fd = fd;
        event.events = channel->get_events();
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &event) < 0) {
            perror("EpollMod error");
            channel_[fd].reset();
        }
    }
}

// 从epoll中删除描述符
void Epoll::EpollDel(std::shared_ptr<Channel> channel) {
    int fd = channel->get_fd();
    struct epoll_event event;
    event.data.fd = fd;
    event.events = channel->get_last_events();
    // event.events = 0;
    // channel->EqualAndUpdateLastEvents()
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, &event) < 0) {
        perror("EpollDel error");
    }
    channel_[fd].reset();
    http_[fd].reset();
}

void Epoll::AddTimer(std::shared_ptr<Channel> channel, int timeout) {
    shared_ptr<HttpData> t = channel->get_holder();
    if (t) {
        timer_queue_.AddTimer(t, timeout);
    } else {
        LOG << "timer add fail";
    }
}

void Epoll::HandleExpire() {
    timer_queue_.HandleExpireEvent();
}