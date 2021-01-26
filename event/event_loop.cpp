#include "event_loop.h"

#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <iostream>

#include "utility/utils.h"
#include "log/logging.h"

__thread EventLoop* tls_loop_in_this_thread = NULL;

int CreateEventfd() {
    int event_fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (eventfd < 0) {
        LOG << "Failed in eventfd";
        abort();
    }

    return event_fd;
}

EventLoop::EventLoop()
    : looping_(false), 
      quit_(false), 
      eventHandling_(false),
      callingPendingFunctors_(false) {
    poller_ = new Epoll();
    wakeupfd_ = createEventfd();
    threadId_ = CurrentThread::tid();
    wakeup_channel_ = new Channel(this, wakeupfd_);
    if (tls_loop_in_this_thread) {
        // LOG << "Another EventLoop " << tls_loop_in_this_thread << " exists in this
        // thread " << threadId_;
    } else {
        tls_loop_in_this_thread = this;
    }
    // wakeup_channel_->set_events(EPOLLIN | EPOLLET | EPOLLONESHOT);
    wakeup_channel_->set_events(EPOLLIN | EPOLLET);
    wakeup_channel_->set_read_handler(bind(&EventLoop::HandleRead, this));
    wakeup_channel_->set_connect_handler(bind(&EventLoop::HandleConnect, this));
    poller_->epoll_add(wakeup_channel_, 0);
}

void EventLoop::HandleConnect() {
    // poller_->epoll_mod(wakeupfd_, wakeup_channel_, (EPOLLIN | EPOLLET |
    // EPOLLONESHOT), 0);
    updatePoller(wakeup_channel_, 0);
}

EventLoop::~EventLoop() {
    // wakeupChannel_->disableAll();
    // wakeupChannel_->remove();
    close(wakeupfd_);
    tls_loop_in_this_thread = NULL;
}

void EventLoop::WakeUp() {
    uint64_t one = 1;
    ssize_t n = writen(wakeupfd_, (char*)(&one), sizeof one);
    if (n != sizeof one) {
        LOG << "EventLoop::WakeUp() writes " << n << " bytes instead of 8";
    }
}

void EventLoop::HandleRead() {
    uint64_t one = 1;
    ssize_t n = readn(wakeupfd_, &one, sizeof one);
    if (n != sizeof one) {
        LOG << "EventLoop::HandleRead() reads " << n << " bytes instead of 8";
    }
    // wakeup_channel_->set_events(EPOLLIN | EPOLLET | EPOLLONESHOT);
    wakeup_channel_->set_events(EPOLLIN | EPOLLET);
}

void EventLoop::RunInLoop(Functor&& callback) {
    if (isInLoopThread()) {
        callback();
    } else {
        QueueInLoop(std::move(callback));
    }
}

void EventLoop::QueueInLoop(Functor&& callback) {
    {
        LockGuard lock(mutex_);
        pendingFunctors_.emplace_back(std::move(callback));
    }

    if (!isInLoopThread() || callingPendingFunctors_) {
        WakeUp();
    }
}

void EventLoop::Loop() {
    assert(!looping_);
    assert(isInLoopThread());
    looping_ = true;
    quit_ = false;
    // LOG_TRACE << "EventLoop " << this << " start looping";
    std::vector<SP_Channel> ret;
    while (!quit_) {
        // cout << "doing" << endl;
        ret.clear();
        ret = poller_->poll();
        eventHandling_ = true;
        for (auto& it : ret) {
            it->handleEvents();
        }
        eventHandling_ = false;
        DoPendingFunctors();
        poller_->handleExpired();
    }
    looping_ = false;
}

void EventLoop::DoPendingFunctors() {
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        LockGuard lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for (size_t i = 0; i < functors.size(); ++i)
        functors[i]();
    callingPendingFunctors_ = false;
}

void EventLoop::Quit() {
    quit_ = true;
    if (!isInLoopThread()) {
        WakeUp();
    }
}
