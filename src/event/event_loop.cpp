#include "event/event_loop.h"

#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <iostream>

#include "utility/socket_utils.h"
// #include "log/logging.h"

namespace event {
//线程局部变量 记录本线程持有的EventLoop的指针 
//一个线程最多持有一个EventLoop 所以创建EventLoop时检查该指针是否为空
__thread EventLoop* tls_event_loop = NULL;

EventLoop::EventLoop()
    : is_looping_(false), 
      is_quit_(false), 
      is_event_handling_(false),
      is_calling_pending_functors_(false) {
    //创建poller io复用 
    poller_ = std::make_shared<Poller>();
    //创建进程间通信event_fd 
    event_fd_ = CreateEventfd();
    //这个eventloop是属于这个线程id的
    thread_id_ = thread_local_storage::thread_id();
    wakeup_channel_ = std::make_shared<Channel>(this, event_fd_);

    //如果本线程已经拥有一个EventLoop 就不能再拥有了
    if (tls_event_loop) {
        // LOG << "Another EventLoop " << tls_event_loop << " exists in this thread " << thread_id_;
    } else {
        tls_event_loop = this;
    }
    
    //给event_fd设置事件以及发生事件后调用的CallBack回调函数
    wakeup_channel_->set_events(EPOLLIN | EPOLLET);
    wakeup_channel_->set_connect_handler(std::bind(&EventLoop::HandleConnect, this));
    wakeup_channel_->set_read_handler(std::bind(&EventLoop::HandleRead, this));
    //将event_fd注册到epoll内核事件表中
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
        channel_array = poller_->Poll();
        is_event_handling_ = true;
        for (auto& channel : channel_array) {
            channel->HandleEvents();
        }

        is_event_handling_ = false;
        DoPendingFunctors();
        poller_->HandleExpire();
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
        locker::LockGuard lock(mutex_);
        pending_functors_.emplace_back(std::move(callback));
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

void EventLoop::DoPendingFunctors() {
    std::vector<CallBack> functors;
    is_calling_pending_functors_ = true;

    {
        locker::LockGuard lock(mutex_);
        functors.swap(pending_functors_);
    }

    for (size_t i = 0; i < functors.size(); ++i)
        functors[i]();
    is_calling_pending_functors_ = false;
}

void EventLoop::WakeUp() {
    uint64_t one = 1;
    ssize_t n = utility::Write(event_fd_, (char*)(&one), sizeof one);
    if (n != sizeof one) {
        // LOG << "EventLoop::WakeUp() writes " << n << " bytes instead of 8";
    }
}

void EventLoop::HandleConnect() {
    // poller_->EpollMod(event_fd_, wakeup_channel_, (EPOLLIN | EPOLLET |
    // EPOLLONESHOT), 0);
    PollerMod(wakeup_channel_, 0);
}

void EventLoop::HandleRead() {
    uint64_t one = 1;
    ssize_t n = utility::Read(event_fd_, &one, sizeof one);
    if (n != sizeof one) {
        // LOG << "EventLoop::HandleRead() reads " << n << " bytes instead of 8";
    }
    // wakeup_channel_->set_events(EPOLLIN | EPOLLET | EPOLLONESHOT);
    wakeup_channel_->set_events(EPOLLIN | EPOLLET);
}

//类似管道的 进程间通信
int EventLoop::CreateEventfd() {
    //设置非阻塞套接字
    int event_fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (eventfd < 0) {
        // LOG << "Failed in eventfd";
        abort();
    }

    return event_fd;
}

}  // namespace event
