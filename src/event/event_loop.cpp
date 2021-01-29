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

//初始化poller, event_fd，给event_fd注册到epoll中并注册其事件处理回调
EventLoop::EventLoop()
    : is_looping_(false), 
      is_stop_(false), 
      is_event_handling_(false),
      is_calling_pending_functions_(false) {
    //创建poller io复用 
    poller_ = std::make_shared<Poller>();
    //创建进程间通信event_fd 
    event_fd_ = CreateEventfd();
    //这个eventloop是属于这个线程id的
    thread_id_ = current_thread::thread_id();
    wakeup_channel_ = std::make_shared<Channel>(event_fd_);

    //如果本线程已经拥有一个EventLoop 就不能再拥有了
    if (tls_event_loop) {
        // LOG << "Another EventLoop " << tls_event_loop << " exists in this thread " << thread_id_;
    } else {
        tls_event_loop = this;
    }
    
    //给event_fd设置事件以及发生事件后调用的CallBack回调函数
    wakeup_channel_->set_events(EPOLLIN | EPOLLET);
    wakeup_channel_->set_read_handler(std::bind(&EventLoop::HandleRead, this));
    wakeup_channel_->set_connect_handler(std::bind(&EventLoop::HandleConnect, this));
    //将event_fd注册到epoll内核事件表中
    poller_->EpollAdd(wakeup_channel_, 0);
}

EventLoop::~EventLoop() {
    close(event_fd_);
    tls_event_loop = NULL;
}

//开始事件循环 调用该函数的线程必须是该EventLoop所在线程 
//1. epoll_wait阻塞 等待就绪事件
//2. 处理每个就绪事件
//3. 执行正在等待的函数
//4. 处理超时
void EventLoop::Loop() {
    //判断是否在当前线程
    assert(!is_looping_);
    assert(is_in_loop_thread());
    is_looping_ = true;
    is_stop_ = false;

    while (!is_stop_) {
        //epoll_wait阻塞 等待就绪事件
        auto ready_channels = poller_->Poll();
        is_event_handling_ = true;
        //处理每个就绪事件（读事件就去处理读，写事件就处理写，连接事件就处理连接，错误事件就处理错误）
        for (auto& channel : ready_channels) {
            channel->HandleEvents();
        }

        is_event_handling_ = false;
        //执行正在等待的函数
        PefrormPendingFunctions();
        //处理超时
        poller_->HandleExpire();
    }

    is_looping_ = false;
}

//停止Loop, 如果是跨线程 则唤醒(向event_fd中写入数据)
void EventLoop::StopLoop() {
    is_stop_ = true;
    if (!is_in_loop_thread()) {
        WakeUp();
    }
}


//如果当前线程就是创建此EventLoop的线程 就调用callback 否则就放入等待执行函数区
void EventLoop::RunInLoop(CallBack&& callback) {
    if (is_in_loop_thread()) {
        callback();
    } else {
        QueueInLoop(std::move(callback));
    }
}

// 把此函数放入等待执行函数区 如果当前是跨线程 或者正在调用等待的函数则唤醒
void EventLoop::QueueInLoop(CallBack&& callback) {
    {
        locker::LockGuard lock(mutex_);
        pending_functions_.emplace_back(std::move(callback));
    }

    //如果跨线程 或者正在运行等待的函数 就唤醒（向event_fd中写入数据）
    if (!is_in_loop_thread() || is_calling_pending_functions_) {
        WakeUp();
    }
}

//执行正在等待的函数
void EventLoop::PefrormPendingFunctions() {
    std::vector<CallBack> callbacks;
    is_calling_pending_functions_ = true;

    //先将正在等待执行的函数拿到本地 再执行 这样锁的时间就会小很多
    {
        locker::LockGuard lock(mutex_);
        callbacks.swap(pending_functions_);
    }   

    for (const auto& callback : callbacks) {
        callback();
    }
    is_calling_pending_functions_ = false;
}

//唤醒 向event_fd中写入数据
void EventLoop::WakeUp() {
    uint64_t one = 1;
    ssize_t n = utility::Write(event_fd_, (char*)(&one), sizeof(one));
    if (n != sizeof(one)) {
        // LOG << "EventLoop::WakeUp() writes " << n << " bytes instead of 8";
    }
}

//eventfd的读回调函数
void EventLoop::HandleRead() {
    uint64_t one = 1;
    ssize_t n = utility::Read(event_fd_, &one, sizeof(one));
    if (n != sizeof(one)) {
        // LOG << "EventLoop::HandleRead() reads " << n << " bytes instead of 8";
    }

    wakeup_channel_->set_events(EPOLLIN | EPOLLET);
}

//eventfd的连接回调函数
void EventLoop::HandleConnect() {
    PollerMod(wakeup_channel_, 0);
}

//创建eventfd 类似管道的 进程间通信方式
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
