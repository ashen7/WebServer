#ifndef EVENT_CHANNEL_H_
#define EVENT_CHANNEL_H_

#include <sys/epoll.h>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include "timer/timer.h"

class EventLoop;
class http::Http;

namespace event {
// Channel封装了一系列fd对应的操作 使用EventCallBack回调函数的手法
// 包括处理读 处理写 处理错误 处理连接4个回调函数
// fd一般是tcp连接connfd(套接字fd), 或者timerfd(定时器fd)，文件fd
class Channel {
 public:
    typedef std::function<void()> EventCallBack;

    explicit Channel(EventLoop* event_loop);
    Channel(EventLoop* event_loop, int fd);
    ~Channel();

    //IO事件的回调函数 由Poller通过EventLoop调用
    void HandleEvents();      
    void HandleConnect();
    void HandleRead();
    void HandleWrite();
    void HandleError(int fd, int err_num, std::string short_msg);

    int fd() const {
        return fd_;
    }
    
    void set_fd(int fd) {
        fd_ = fd;
    }

    std::shared_ptr<http::Http> holder() {
        std::shared_ptr<http::Http> holder(holder_.lock());
        return holder;
    }

    void set_holder(std::shared_ptr<http::Http> holder) {
        holder_ = holder;
    }

    void set_connect_handler(EventCallBack&& connect_handler) {
        connect_handler_ = connect_handler;
    }

    void set_read_handler(EventCallBack&& read_handler) {
        read_handler_ = read_handler;
    }

    void set_write_handler(EventCallBack&& write_handler) {
        write_handler_ = write_handler;
    }

    void set_error_handler(EventCallBack&& error_handler) {
        error_handler_ = error_handler;
    }

    void set_revents(int revents) {
        revents_ = revents;
    }

    int& events() {
        return events_;
    }

    void set_events(int events) {
        events_ = events;
    }

    int last_events() const {
        return last_events_;
    }

    bool update_last_events() {
        bool is_change = (last_events_ == events_);
        last_events_ = events_;
        return is_change;
    }

 private:
    EventLoop* event_loop_;
    int fd_;            //Channel的fd
    int events_;        //Channel正在监听的事件
    int revents_;       //返回的就绪事件
    int last_events_;   //上一个事件

    // 方便找到上层持有该Channel的对象
    std::weak_ptr<http::Http> holder_;

    EventCallBack read_handler_;
    EventCallBack write_handler_;
    EventCallBack connect_handler_;
    EventCallBack error_handler_;
};

}  // namespace event

#endif  // EVENT_CHANNEL_H_