#ifndef WEB_SERVER_H_
#define WEB_SERVER_H_

#include "event/channel.h"
#include "evnet/event_loop.h"

class WebServer {
 public:
    WebServer(EventLoop* loop, int threadNum, int port);
    ~WebServer() {}
  
    EventLoop* get_loop() const {
        return loop_;
    }
  
    void Start();
    
    void HandleNewConnect();
    void handelCurConnect() {
        loop_->updatePoller(accept_channel_);
    }

 private:
    static const int MAXFDS = 100000;

    int port_;
    int listenfd_;
    int thread_num_;
    EventLoop* loop_;
    std::unique_ptr<EventLoopThreadPool> event_loop_thread_pool_;
    bool started_;
    std::shared_ptr<Channel> accept_channel_;
};

#endif