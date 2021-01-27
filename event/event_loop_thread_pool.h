#ifndef EVENT_LOOP_THREAD_POOL_H_
#define EVENT_LOOP_THREAD_POOL_H_

#include "event_loop.h"

#include <iostream>
#include <functional>
#include <memory>
#include <vector>

#include "channel.h"
#include "epoll.h"
#include "thread/thread.h"
#include "utility/socket_utils.h"
#include "log/logging.h"

class EventLoopThread : NonCopyAble {
 public:
    EventLoopThread()
        : event_loop_(NULL), exiting_(false),
          mutex_(), cond_(mutex_) {
        thread_(bind(&EventLoopThread::threadFunc, this), 
                "EventLoopThread"),
    }
    ~EventLoopThread() {
        exiting_ = true;
        if (event_loop_ != NULL) {
            event_loop_->quit();
            thread_.join();
        }
    }

    EventLoop* startLoop() {
        assert(!thread_.started());
        thread_.start();
        {
            MutexLockGuard lock(mutex_);
            // 一直等到threadFun在Thread里真正跑起来
            while (event_loop_ == NULL)
                cond_.wait();
        }
        return event_loop_;
    }

 private:
    void threadFunc() {
        EventLoop loop;
        {
            MutexLockGuard lock(mutex_);
            event_loop_ = &loop;
            cond_.notify();
        }
        loop.loop();
        // assert(exiting_);
        event_loop_ = NULL;
    }

 private:
    EventLoop* event_loop_;
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
