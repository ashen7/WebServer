#ifndef EPOLL_H_
#define EPOLL_H_ 

#include <sys/epoll.h>

#include <memory>
#include <unordered_map>
#include <vector>

#include "channel.h"
#include "http/http.h"
#include "timer/timer.h"

class Epoll {
 public:
    Epoll();
    ~Epoll();
    
    //分发
    std::vector<std::shared_ptr<Channel>> Poll();
    std::vector<std::shared_ptr<Channel>> GetEventsRequest(int events_num);

    //注册 修改 删除epoll内核事件表
    void EpollAdd(std::shared_ptr<Channel> channel, int timeout);
    void EpollMod(std::shared_ptr<Channel> channel, int timeout);
    void EpollDel(std::shared_ptr<Channel> channel);

    //添加定时器
    void AddTimer(std::shared_ptr<Channel> channel, int timeout);
    //处理超时
    void HandleExpire();

    int get_epoll_fd() {
        return epoll_fd_;
    }

 private:
    static constexpr int MAX_FD_NUM = 100000;     //最大fd数量
    static constexpr int MAX_EVENTS_NUM = 10000;  //最大事件数量
    static constexpr int EPOLL_TIMEOUT = 10000;   //epoll wait的超时时间

    int epoll_fd_;
    std::vector<epoll_event> events_;
    std::shared_ptr<Channel> channel_[MAX_FD_NUM];
    std::shared_ptr<Http> http_[MAX_FD_NUM];
    TimerQueue timer_queue_;
};

#endif