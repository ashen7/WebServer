#include "timer/timer_heap.h"

#include <memory>

#include "http/http.h"

namespace timer {
//向小根堆中添加定时器
void TimerHeap::AddTimer(std::shared_ptr<http::Http> http, int timeout) {
    auto timer = std::make_shared<Timer>(http, timeout);
    timer_heap_.push(timer);
    http->set_timer(timer);
}

//处理到期事件 如果定时器被删除或者已经到期 就从小根堆中删除
void TimerHeap::HandleExpireEvent() {
    while (!timer_heap_.empty()) {
        auto timer = timer_heap_.top();
        
        if (timer->is_deleted()) {
            timer_heap_.pop();
        } else if (timer->is_expired()) {
            timer_heap_.pop();
        } else {
            break;
        }
    }
}

}  // namespace timer
