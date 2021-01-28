#include "timer/timer.h"

#include <sys/time.h>
#include <unistd.h>

#include <memory>
#include <queue>

#include "http/http.h"

namespace timer {

Timer::Timer(std::shared_ptr<http::Http> http, int timeout)
    : http_(http),
      is_deleted_(false) {
    // 以毫秒计算 到期时间 = 当前时间 + 超时时间 
    struct timeval now;
    gettimeofday(&now, NULL);
    expire_time_ = (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000)) + timeout;
}

//拷贝构造函数
Timer::Timer(Timer& timer)
    : http_(timer.http_), 
      expire_time_(0) {
}

//如果http没有释放 就调用Close关闭
Timer::~Timer() {
    if (http_) {
        http_->HandleClose();
    }
}

//更新到期时间 = 当前时间 + 超时时间
void Timer::Update(int timeout) {
    struct timeval now;
    gettimeofday(&now, NULL);
    expire_time_ = (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000)) + timeout;
}

//是否到期
bool Timer::is_expired() {
    struct timeval now;
    gettimeofday(&now, NULL);
    size_t current_time = (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000));
    if (current_time >= expire_time_) {
        is_deleted_ = true;
        return true;
    }
    
    return false;
}

//释放http
void Timer::Clear() {
    http_.reset();
    is_deleted_ = true;
}

}  // namespace timer