#ifndef EVENT_POLLER_H_
#define EVENT_POLLER_H_ 

#include <sys/epoll.h>

#include <memory>
#include <unordered_map>
#include <vector>

#include "timer/timer_heap.h"
#include "utility/noncopyable.h"

//类的前置声明
namespace http {
class HttpConnection;
}  // namespace http

namespace event {
//类的前置声明
class Channel;

//IO多路复用类
class Poller : utility::NonCopyAble {
 public:
    Poller();
    ~Poller();
    
    //分发
    std::vector<std::shared_ptr<Channel>> Poll();

    //注册 修改 删除epoll内核事件表
    void EpollAdd(std::shared_ptr<Channel> channel, int timeout);
    void EpollMod(std::shared_ptr<Channel> channel, int timeout);
    void EpollDel(std::shared_ptr<Channel> channel);

    //添加定时器
    void AddTimer(std::shared_ptr<Channel> channel, int timeout);
    //处理超时
    void HandleExpire();

    int epoll_fd() {
        return epoll_fd_;
    }

 private:
    static constexpr int MAX_FD_NUM = 100000;      //最大fd数量
    static constexpr int MAX_EVENTS_NUM = 10000;  //最大事件数量
    static constexpr int EPOLL_TIMEOUT = 10000;   //epoll wait的超时时间

 private:
    int epoll_fd_;
    std::vector<epoll_event> event_array_;
    std::vector<std::shared_ptr<Channel>> channel_array_;
    std::vector<std::shared_ptr<http::HttpConnection>> http_array_;
    timer::TimerHeap timer_heap_;
};

}  // namespace event

#endif  // EVENT_POLLER_H_