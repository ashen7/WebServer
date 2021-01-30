#ifndef EVENT_EVENT_LOOP_THREAD_POOL_H_
#define EVENT_EVENT_LOOP_THREAD_POOL_H_

#include "event_loop.h"

#include <iostream>
#include <functional>
#include <memory>
#include <vector>

namespace event {
class EventLoopThread : utility::NonCopyAble {
 public:
    EventLoopThread();
    ~EventLoopThread();
     
    //执行线程函数, 在线程函数还没运行起来的时候 一直阻塞 运行起来了才唤醒返回该event_loop
    EventLoop* StartLoop();

 private:
    //线程函数(就是EventLoop的Loop函数)
    void Worker();
   
 private:
    EventLoop* sub_loop_;    //子loop
    thread::Thread thread_;  //线程对象
    mutable locker::MutexLock mutex_;
    locker::ConditionVariable condition_;
    bool is_exiting_;       
};

class EventLoopThreadPool : utility::NonCopyAble {
 public:
    EventLoopThreadPool(EventLoop* main_loop, int thread_num);
    ~EventLoopThreadPool();

    void Start();
    EventLoop* GetNextLoop();

 private:
    EventLoop* main_loop_;
    bool is_started_;
    int thread_num_;
    int next_;
    std::vector<std::shared_ptr<EventLoopThread>> event_loop_thread_array_;
    std::vector<EventLoop*> event_loop_array_;
};

}  // namespace event

#endif  // EVENT_EVENT_LOOP_THREAD_POOL_H_
