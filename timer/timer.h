#ifndef TIMER_H_
#define TIMER_H_

#include <unistd.h>

#include <memory>
#include <deque>
#include <queue>

#include "http/http_data.h"
#include "locker/MutexLock.h"
#include "utility/noncopyable.h"

class HttpData;

class Timer : {
 public:
    Timer(Timer& timer_node);
    Timer(std::shared_ptr<HttpData> http_data, int timeout);
    ~Timer();

    void Update(int timeout);
    void clearReq();

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

    bool is_deleted() const {
        return is_deleted_;
    }
    
    int get_expire_time() const {
        return expire_time_;
    }

 private:
    bool is_deleted_;
    int expire_time_;
    std::shared_ptr<HttpData> http_data_;
};

struct TimerCompare {
    bool operator()(std::shared_ptr<Timer>& a,
                    std::shared_ptr<Timer>& b) const {
        return a->get_expire_time() > b->get_expire_time();
    }
};

class TimerQueue {
 public:
    TimerQueue();
    ~TimerQueue();

    //添加定时器 将其添加到小根堆中
    void AddTimer(std::shared_ptr<HttpData> http_data, int timeout);
    //处理超时事件 超时就从小根堆中删除
    void HandleExpireEvent();

 private:
    std::priority_queue<std::shared_ptr<Timer>, 
                        std::deque<std::shared_ptr<Timer>>, 
                        TimerCompare> timer_queue_;
};

#endif