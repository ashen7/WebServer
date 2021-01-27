#ifndef WEB_SERVER_H_
#define WEB_SERVER_H_

#include "event/channel.h"
#include "evnet/event_loop.h"

class WebServer {
 public:
    WebServer(EventLoop* event_loop, int thread_num, int port);
    ~WebServer() {}
  
    EventLoop* get_event_loop() const {
        return event_loop_;
    }
  
    void Start();

    void HandleNewConnect();

    void handelCurConnect();

 private:
    static const int MAX_FD_NUM = 100000;

    int port_;
    int listen_fd_;
    int thread_num_;
    EventLoop* event_loop_;
    std::unique_ptr<EventLoopThreadPool> event_loop_thread_pool_;
    bool started_;
    std::shared_ptr<Channel> accept_channel_;
};

#endif