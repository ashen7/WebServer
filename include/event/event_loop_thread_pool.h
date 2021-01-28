#ifndef EVENT_LOOP_THREAD_POOL_H_
#define EVENT_LOOP_THREAD_POOL_H_

#include "event_loop.h"

#include <iostream>
#include <functional>
#include <memory>
#include <vector>

#include "channel.h"
#include "epoll.h"
#include "locker/mutex_lock.h"
#include "thread/thread.h"
#include "utility/socket_utils.h"
#include "log/logging.h"

class EventLoopThread : NonCopyAble {
 public:
    EventLoopThread();
    ~EventLoopThread();
     
    EventLoop* StartLoop();

 private:
    void Worker();
   
 private:
    EventLoop* event_loop_;
    bool is_exiting_;
    Thread thread_;
    MutexLock mutex_;
    ConditionVariable condition_;
};

class EventLoopThreadPool : NonCopyAble {
 public:
    EventLoopThreadPool(EventLoop* event_loop, int thread_num);
    ~EventLoopThreadPool();

    void Start();
    EventLoop* getNextLoop();

 private:
    EventLoop* event_loop;
    bool is_started_;
    int thread_num_;
    int next_;
    std::vector<std::shared_ptr<EventLoopThread>> thread_array_;
    std::vector<EventLoop*> event_loop_array_;
};

#endif
