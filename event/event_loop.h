#ifndef EVENT_LOOP_H_
#define EVENT_LOOP_H_

#include <functional>
#include <memory>
#include <vector>

#include "channel.h"
#include "epoll.h"
#include "thread/thread.h"
#include "utility/utils.h"
#include "log/logging.h"

#include <iostream>

class EventLoop {
 public:
    typedef std::function<void()> Functor;

    EventLoop();
    ~EventLoop();
    void Loop();
    void Quit();
    void RunInLoop(Functor&& cb);
    void QueueInLoop(Functor&& cb);
    bool isInLoopThread() const {
        return threadId_ == CurrentThread::tid();
    }

    void AssertInLoopThread() {
        assert(isInLoopThread());
    }

    void ShutDown(std::shared_ptr<Channel> channel) {
        shutDownWR(channel->getFd());
    }

    void RemoveFromPoller(std::shared_ptr<Channel> channel) {
        // shutDownWR(channel->getFd());
        poller_->DelEpoll(channel);
    }

    void UpdatePoller(std::shared_ptr<Channel> channel, int timeout = 0) {
        poller_->epoll_mod(channel, timeout);
    }

    void AddToPoller(std::shared_ptr<Channel> channel, int timeout = 0) {
        poller_->epoll_add(channel, timeout);
    }

 private:
    // 声明顺序 wakeupfd_ > wakeup_channel_
    bool looping_;
    std::shared_ptr<Epoll> poller_;
    int wakeupfd_;
    bool quit_;
    bool eventHandling_;
    mutable MutexLock mutex_;
    std::vector<Functor> pending_functors_;
    bool callingPendingFunctors_;
    const pid_t threadId_;
    std::shared_ptr<Channel> wakeup_channel_;

    void wakeup();
    void handleRead();
    void doPendingFunctors();
    void handleConn();
};

class EventLoopThread : NonCopyAble {
 public:
    EventLoopThread()
        : loop_(NULL), exiting_(false),
          mutex_(), cond_(mutex_) {
        thread_(bind(&EventLoopThread::threadFunc, this), 
                "EventLoopThread"),
    }
    ~EventLoopThread() {
        exiting_ = true;
        if (loop_ != NULL) {
            loop_->quit();
            thread_.join();
        }
    }

    EventLoop* startLoop() {
        assert(!thread_.started());
        thread_.start();
        {
            MutexLockGuard lock(mutex_);
            // 一直等到threadFun在Thread里真正跑起来
            while (loop_ == NULL)
                cond_.wait();
        }
        return loop_;
    }

 private:
    void threadFunc() {
        EventLoop loop;
        {
            MutexLockGuard lock(mutex_);
            loop_ = &loop;
            cond_.notify();
        }
        loop.loop();
        // assert(exiting_);
        loop_ = NULL;
    }

 private:
    EventLoop* loop_;
    bool exiting_;
    Thread thread_;
    MutexLock mutex_;
    ConditionVariable condition_;
};

class EventLoopThreadPool : NonCopyAble {
 public:
    EventLoopThreadPool(EventLoop* baseLoop, int numThreads)
        : baseLoop_(baseLoop), started_(false), 
          numThreads_(numThreads), next_(0) {
        if (numThreads_ <= 0) {
            LOG << "numThreads_ <= 0";
            abort();
    }

    ~EventLoopThreadPool() {
        LOG << "~EventLoopThreadPool()";
    }

    void start() {
        baseLoop_->assertInLoopThread();
        started_ = true;
        for (int i = 0; i < numThreads_; ++i) {
            std::shared_ptr<EventLoopThread> t(new EventLoopThread());
            threads_.push_back(t);
            loops_.push_back(t->startLoop());
        }
    }

    EventLoop* getNextLoop() {
        baseLoop_->assertInLoopThread();
        assert(started_);
        EventLoop* loop = baseLoop_;
        if (!loops_.empty()) {
            loop = loops_[next_];
            next_ = (next_ + 1) % numThreads_;
        }
        return loop;
    }

 private:
    EventLoop* baseLoop_;
    bool started_;
    int numThreads_;
    int next_;
    std::vector<std::shared_ptr<EventLoopThread>> threads_;
    std::vector<EventLoop*> loops_;
};

#endif