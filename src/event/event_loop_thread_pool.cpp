#include "event/event_loop_thread_pool.h"

#include <memory>
#include <functional>
#include <vector>

#include "log/logging.h"

namespace event {

//绑定线程函数，也就是Loop
EventLoopThread::EventLoopThread()
    : sub_loop_(NULL), 
      is_exiting_(false),
      mutex_(),
      condition_(mutex_),
      thread_(std::bind(&EventLoopThread::Worker, this), "EventLoopThread") {
}

EventLoopThread::~EventLoopThread() {
    //停止loop后 线程join等待停止loop运行完
    is_exiting_ = true;
    if (sub_loop_ != NULL) {
        sub_loop_->StopLoop();
        thread_.Join();
    }
}

//执行线程函数, 在线程函数还没运行起来的时候 一直阻塞 运行起来了才唤醒返回该event_loop
EventLoop* EventLoopThread::StartLoop() {
    assert(!thread_.is_started());
    //执行线程函数
    thread_.Start();
    {
        locker::LockGuard lock(mutex_);
        // 一直阻塞等到线程函数运行起来 再唤醒返回
        while (sub_loop_ == NULL) {
            condition_.wait();
        }
    }

    return sub_loop_;
}

//线程函数(内部先唤醒StartLoop, 然后调用Loop事件循环)
void EventLoopThread::Worker() {
    EventLoop event_loop;
    {
        locker::LockGuard lock(mutex_);
        sub_loop_ = &event_loop;
        //已经运行线程函数了 唤醒StartLoop返回此event_loop对象 
        condition_.notify();
    }

    //Loop事件循环 
    event_loop.Loop();
    sub_loop_ = NULL;
}

//event loop线程池
EventLoopThreadPool::EventLoopThreadPool(EventLoop* main_loop, int thread_num)
    : main_loop_(main_loop), 
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

//主线程（主Loop对象）创建event_loop线程池
void EventLoopThreadPool::Start() {
    assert(main_loop_->is_in_loop_thread());
    is_started_ = true;

    //创建event_loop_thread_pool,并将开始Loop事件循环的EventLoop对象存入array中
    for (int i = 0; i < thread_num_; ++i) {
        auto event_loop_thread = std::make_shared<EventLoopThread>();
        event_loop_thread_array_.push_back(event_loop_thread);
        event_loop_array_.push_back(event_loop_thread->StartLoop());
    }
}

//从已经开始Loop事件循环的EventLoop对象列表中 返回一个EventLoop对象 如果此时还没有 就返回主loop
EventLoop* EventLoopThreadPool::GetNextLoop() {
    //调用这个函数的必须是主loop线程
    assert(main_loop_->is_in_loop_thread());
    assert(is_started_);

    // 如果此时还没有开始Loop的EventLoop对象 就返回主loop
    auto event_loop = main_loop_;
    if (!event_loop_array_.empty()) {
        event_loop = event_loop_array_[next_];
        next_ = (next_ + 1) % thread_num_;
    }
    
    return event_loop;
}

}  // namespace event
