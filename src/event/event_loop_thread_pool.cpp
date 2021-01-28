#include "event/event_loop_thread_pool.h"

#include <memory>
#include <functional>
#include <vector>

namespace event {

EventLoopThread::EventLoopThread()
    : event_loop_(NULL), 
      is_exiting_(false),
      mutex_(),
      condition_(mutex_),
      thread_(std::bind(&EventLoopThread::Worker, this), "EventLoopThread") {
}

EventLoopThread::~EventLoopThread() {
    is_exiting_ = true;
    if (event_loop_ != NULL) {
        event_loop_->Quit();
        thread_.Join();
    }
}

EventLoop* EventLoopThread::StartLoop() {
    assert(!thread_.is_started());
    thread_.Start();
    {
        locker::LockGuard lock(mutex_);
        // 一直等到threadFun在Thread里真正跑起来
        while (event_loop_ == NULL) {
            condition_.wait();
        }
    }

    return event_loop_;
}

void EventLoopThread::Worker() {
    EventLoop event_loop;
    {
        locker::LockGuard lock(mutex_);
        event_loop_ = &event_loop;
        condition_.notify();
    }

    event_loop.Loop();
    event_loop_ = NULL;
}

EventLoopThreadPool::EventLoopThreadPool(EventLoop* event_loop, int thread_num)
    : event_loop_(event_loop), 
      is_started_(false), 
      thread_num_(thread_num), 
      next_(0) {
    if (thread_num_ <= 0) {
        // LOG << "thread num <= 0";
        abort();
    }
}

EventLoopThreadPool::~EventLoopThreadPool() {
    // LOG << "~EventLoopThreadPool()";
}

void EventLoopThreadPool::Start() {
    event_loop_->AssertInLoopThread();
    is_started_ = true;
    for (int i = 0; i < thread_num_; ++i) {
        std::shared_ptr<EventLoopThread> thread(new EventLoopThread());
        thread_array_.push_back(thread);
        event_loop_array_.push_back(thread->StartLoop());
    }
}

EventLoop* EventLoopThreadPool::GetNextLoop() {
    event_loop_->AssertInLoopThread();
    assert(is_started_);
    EventLoop* event_loop = event_loop_;
    if (!event_loop_array_.empty()) {
        event_loop = event_loop_array_[next_];
        next_ = (next_ + 1) % thread_num_;
    }
    
    return event_loop;
}

}  // namespace event
