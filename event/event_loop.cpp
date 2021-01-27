#include "event_loop.h"

#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <iostream>

#include "utility/socket_utils.h"
#include "log/logging.h"

//线程局部变量 记录本线程持有的EventLoop的指针 
//一个线程最多持有一个EventLoop 所以创建EventLoop时检查该指针是否为空
__thread EventLoop* tls_event_loop = NULL;

//类似管道的 线程间通信
int CreateEventfd() {
    int event_fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (eventfd < 0) {
        LOG << "Failed in eventfd";
        abort();
    }

    return event_fd;
}

EventLoop::EventLoop()
    : is_looping_(false), 
      is_quit_(false), 
      is_event_handling_(false),
      is_calling_pending_functors_(false) {
    poller_ = new Epoll();
    event_fd_ = CreateEventfd();
    thread_id_ = current_thread::thread_id();
    wakeup_channel_ = new Channel(this, event_fd_);

    //如果本线程已经拥有一个EventLoop 就不能再拥有了
    if (tls_event_loop) {
        LOG << "Another EventLoop " << tls_event_loop << " exists in this thread " << thread_id_;
    } else {
        tls_event_loop = this;
    }
    
    wakeup_channel_->set_events(EPOLLIN | EPOLLET);
    wakeup_channel_->set_read_handler(bind(&EventLoop::HandleRead, this));
    wakeup_channel_->set_connect_handler(bind(&EventLoop::HandleConnect, this));
    poller_->EpollAdd(wakeup_channel_, 0);
}

EventLoop::~EventLoop() {
    close(event_fd_);
    tls_event_loop = NULL;
}

//开始事件循环 调用该函数的线程必须是该EventLoop所在线程
void EventLoop::Loop() {
    assert(!is_looping_);
    assert(is_in_loop_thread());
    is_looping_ = true;
    is_quit_ = false;
    std::vector<std::shared_ptr<Channel>> channel_array;

    while (!is_quit_) {
        channel_array.clear();
        channel_array = poller_->poll();
        is_event_handling_ = true;
        for (auto& channel : channel_array) {
            channel->HandleEvents();
        }

        is_event_handling_ = false;
        DoPendingFunctors();
        poller_->HandleExpired();
    }
    is_looping_ = false;
}

void EventLoop::RunInLoop(CallBack&& callback) {
    if (is_in_loop_thread()) {
        callback();
    } else {
        QueueInLoop(std::move(callback));
    }
}

void EventLoop::QueueInLoop(CallBack&& callback) {
    {
        LockGuard lock(mutex_);
        pendingFunctors_.emplace_back(std::move(callback));
    }

    if (!is_in_loop_thread() || is_calling_pending_functors_) {
        WakeUp();
    }
}

void EventLoop::Quit() {
    is_quit_ = true;
    if (!is_in_loop_thread()) {
        WakeUp();
    }
}

void EventLoop::RunPendingFunctors() {
    std::vector<CallBack> functors;
    is_calling_pending_functors_ = true;

    {
        LockGuard lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for (size_t i = 0; i < functors.size(); ++i)
        functors[i]();
    is_calling_pending_functors_ = false;
}

void EventLoop::HandleConnect() {
    // poller_->EpollMod(event_fd_, wakeup_channel_, (EPOLLIN | EPOLLET |
    // EPOLLONESHOT), 0);
    PollerMod(wakeup_channel_, 0);
}

void EventLoop::HandleRead() {
    uint64_t one = 1;
    ssize_t n = readn(event_fd_, &one, sizeof one);
    if (n != sizeof one) {
        LOG << "EventLoop::HandleRead() reads " << n << " bytes instead of 8";
    }
    // wakeup_channel_->set_events(EPOLLIN | EPOLLET | EPOLLONESHOT);
    wakeup_channel_->set_events(EPOLLIN | EPOLLET);
}

void EventLoop::WakeUp() {
    uint64_t one = 1;
    ssize_t n = writen(event_fd_, (char*)(&one), sizeof one);
    if (n != sizeof one) {
        LOG << "EventLoop::WakeUp() writes " << n << " bytes instead of 8";
    }
}