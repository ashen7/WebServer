#pragma once
#include <functional>
#include <memory>
#include <vector>
#include "Channel.h"
#include "Epoll.h"
#include "Util.h"
#include "base/CurrentThread.h"
#include "base/Logging.h"
#include "base/Thread.h"

#include <iostream>

class EventLoop {
 public:
    typedef std::function<void()> Functor;

    EventLoop();
    ~EventLoop();
    void loop();
    void quit();
    void runInLoop(Functor&& cb);
    void queueInLoop(Functor&& cb);
    bool isInLoopThread() const {
        return threadId_ == CurrentThread::tid();
    }
    void assertInLoopThread() {
        assert(isInLoopThread());
    }
    void shutdown(std::shared_ptr<Channel> channel) {
        shutDownWR(channel->getFd());
    }
    void removeFromPoller(std::shared_ptr<Channel> channel) {
        // shutDownWR(channel->getFd());
        poller_->epoll_del(channel);
    }
    void updatePoller(std::shared_ptr<Channel> channel, int timeout = 0) {
        poller_->epoll_mod(channel, timeout);
    }
    void addToPoller(std::shared_ptr<Channel> channel, int timeout = 0) {
        poller_->epoll_add(channel, timeout);
    }

 private:
    // 声明顺序 wakeupFd_ > pwakeupChannel_
    bool looping_;
    std::shared_ptr<Epoll> poller_;
    int wakeupFd_;
    bool quit_;
    bool eventHandling_;
    mutable MutexLock mutex_;
    std::vector<Functor> pendingFunctors_;
    bool callingPendingFunctors_;
    const pid_t threadId_;
    std::shared_ptr<Channel> pwakeupChannel_;

    void wakeup();
    void handleRead();
    void doPendingFunctors();
    void handleConn();
};
