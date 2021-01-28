#ifndef EVENT_EVENT_LOOP_H_
#define EVENT_EVENT_LOOP_H_

#include <assert.h>

#include <iostream>
#include <functional>
#include <memory>
#include <vector>

#include "channel.h"
#include "poller.h"
#include "thread/thread.h"
#include "utility/socket_utils.h"
// #include "log/logging.h"

namespace event {
// Reactor模式的核心 每个Reactor线程内部调用一个EventLoop
// 内部不停的进行epoll_wait调用 然后调用fd对应Channel的相应回调函数进行处理
class EventLoop {
 public:
    typedef std::function<void()> CallBack;

    EventLoop();
    ~EventLoop();

    //开始事件循环 调用该函数的线程必须是该EventLoop所在线程，也就是Loop函数不能跨线程调用
    void Loop();

    //退出，这个可以跨线程调用
    void Quit();
    
    void RunInLoop(CallBack&& cb);
    void QueueInLoop(CallBack&& cb);

    void ShutDown(std::shared_ptr<Channel> channel) {
        utility::ShutDownWR(channel->fd());
    }

    void PollerAdd(std::shared_ptr<Channel> channel, int timeout = 0) {
        poller_->EpollAdd(channel, timeout);
    }

    void PollerMod(std::shared_ptr<Channel> channel, int timeout = 0) {
        poller_->EpollMod(channel, timeout);
    }

    void PollerDel(std::shared_ptr<Channel> channel) {
        poller_->EpollDel(channel);
    }

    void AssertInLoopThread() {
        assert(is_in_loop_thread());
    }

    bool is_in_loop_thread() const {
        return thread_id_ == thread_local_storage::thread_id();
    }

 private:
    static int CreateEventfd();

    void HandleRead();
    void HandleConnect();
    void WakeUp();
    void DoPendingFunctors();

 private:
    std::shared_ptr<Poller> poller_;
    int event_fd_;
    pid_t thread_id_;
    std::shared_ptr<Channel> wakeup_channel_;

    mutable locker::MutexLock mutex_;
    std::vector<CallBack> pending_functors_;

    bool is_looping_;
    bool is_quit_;
    bool is_event_handling_;
    bool is_calling_pending_functors_;
};

}  // namespace event

#endif  // EVENT_EVENT_LOOP_H_