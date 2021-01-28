#ifndef TIMER_TIMER_H_
#define TIMER_TIMER_H_

#include <unistd.h>

#include <memory>

#include "utility/noncopyable.h"

namespace timer {
//类的前置声明
class http::Http;

//定时器类
class Timer {
 public:
    Timer(std::shared_ptr<http::Http> http, int timeout);
    //拷贝构造
    Timer(Timer& timer);
    ~Timer();

    //更新到期时间 = 当前时间 + 超时时间
    void Update(int timeout);
    //释放http
    void Clear();

    //是否到期
    bool is_expired() {
        struct timeval now;
        gettimeofday(&now, NULL);
        size_t current_time = (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000));
        if (current_time >= expired_time_) {
            is_deleted_ = true;
            return true;
        }
        
        return false;
    }

    //得到到期时间
    int expire_time() const {
        return expire_time_;
    }

    //http是否已经删除
    bool is_deleted() const {
        return is_deleted_;
    }

 private:
    std::shared_ptr<http::Http> http_;
    int expire_time_;
    bool is_deleted_;
};

}  // namespace timer

#endif  // namespace TIMER_TIMER_H_