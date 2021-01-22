#ifndef EPOLL_H_
#define EPOLL_H_ 

#include <sys/epoll.h>

#include <memory>
#include <unordered_map>
#include <vector>

#include "channel.h"
#include "http/htpp.h"
#include "timer/timer.h"

class Epoll {
 public:
    Epoll();
    ~Epoll();

    int get_epollfd() {
        return epollfd_;
    }

    void AddEpoll(std::shared_ptr<Channel> request, int timeout);
    void ModEpoll(std::shared_ptr<Channel> request, int timeout);
    void DelEpoll(std::shared_ptr<Channel> request);
    
    std::vector<std::shared_ptr<Channel>> Poll();
    std::vector<std::shared_ptr<Channel>> GetEventsRequest(int events_num);
    void AddTimer(std::shared_ptr<Channel> request_data, int timeout);

    void HandleExpired();

 private:
    static constexpr int MAXFDS = 100000;

    int epollfd_;
    std::vector<epoll_event> events_;
    std::shared_ptr<Channel> fd2chan_[MAXFDS];
    std::shared_ptr<HttpData> fd2http_[MAXFDS];
    TimerManager timerManager_;
};

#endif