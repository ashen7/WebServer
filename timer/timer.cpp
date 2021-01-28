#include "timer.h"

#include <sys/time.h>
#include <unistd.h>

#include <queue>

Timer::Timer(Timer& timer)
    : http_data(timer.http_data), 
      expire_time_(0) {
}

Timer::Timer(std::shared_ptr<HttpData> http_data, int timeout)
    : http_data_(http_data),
      is_deleted_(false) {
    // 以毫秒计
    struct timeval now;
    gettimeofday(&now, NULL);
    expire_time_ = (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000)) + timeout;
}

Timer::~Timer() {
    if (http_data) {
        http_data->HandleClose();
    }
}

void Timer::Update(int timeout) {
    struct timeval now;
    gettimeofday(&now, NULL);
    expire_time_ = (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000)) + timeout;
}

void Timer::ClearReq() {
    http_data.reset();
    is_deleted_ = true;
}

TimerQueue::TimerQueue() {
}

TimerQueue::~TimerQueue() {
}

void TimerQueue::AddTimer(std::shared_ptr<HttpData> http_data, int timeout) {
    std::shared_ptr<Timer> timer(new Timer(http_data, timeout));
    timer_queue_.push(timer);
    http_data->linkTimer(timer);
}

void TimerQueue::HandleExpireEvent() {
    while (!timer_queue_.empty()) {
        std::shared_ptr<Timer> timer = timer_queue_.top();
        if (timer->is_deleted()) {
            timer_queue_.pop();
        } else if (timer->is_expired()) {
            timer_queue_.pop();
        } else {
            break;
        }
    }
}