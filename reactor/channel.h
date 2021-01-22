#include <sys/epoll.h>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include "timer.h"

class EventLoop;
class HttpData;

class Channel {
 private:
    typedef std::function<void()> CallBack;

 public:
    explicit Channel(EventLoop* loop);
    Channel(EventLoop* loop, int fd);
    ~Channel();

    int get_fd() {
        return fd_;
    }
    
    void set_fd(int fd) {
        fd_ = fd;
    }

    std::shared_ptr<HttpData> get_holder() {
        std::shared_ptr<HttpData> ret(holder_.lock());
        return ret;
    }

    void set_holder(std::shared_ptr<HttpData> holder) {
        holder_ = holder;
    }

    void set_read_handler(CallBack&& readHandler) {
        readHandler_ = readHandler;
    }

    void set_write_handler(CallBack&& writeHandler) {
        writeHandler_ = writeHandler;
    }

    void set_error_handler(CallBack&& errorHandler) {
        errorHandler_ = errorHandler;
    }

    void set_conn_handler(CallBack&& connHandler) {
        connHandler_ = connHandler;
    }

    void set_revents(int ev) {
        revents_ = ev;
    }

    void set_events(int ev) {
        events_ = ev;
    }

    int& getEvents() {
        return events_;
    }

    bool EqualAndUpdateLastEvents() {
        bool ret = (lastEvents_ == events_);
        lastEvents_ = events_;
        return ret;
    }

    int getLastEvents() {
        return lastEvents_;
    }

    //IO事件的回调函数
    void HandleEvents();
    void handleConn();
    void handleRead();
    void handleWrite();
    void handleError(int fd, int err_num, std::string short_msg);

 private:
    int parse_URI();
    int parse_Headers();
    int analysisRequest();

 private:
    EventLoop* loop_;
    int fd_;
    int events_;
    int revents_;
    int lastEvents_;

    // 方便找到上层持有该Channel的对象
    std::weak_ptr<HttpData> holder_;
    
    CallBack readHandler_;
    CallBack writeHandler_;
    CallBack errorHandler_;
    CallBack connHandler_;
};
