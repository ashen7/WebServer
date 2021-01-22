#ifndef MUTEX_LOCK_H_
#define MUTEX_LOCK_H_

#include <stdio.h>
#include <time.h>
#include <pthread.h>

#include "noncopyable.h"

class ConditionVariable;

//c++类不指定默认private方式继承 
//private继承的父类的public, protected都会成为子类的private
class MutexLock : NonCopyAble {
 public:
    MutexLock() {
        pthread_mutex_init(&mutex_, NULL);
    }

    ~MutexLock() {
        pthread_mutex_destroy(&mutex_);
    }

    void lock() {
        pthread_mutex_lock(&mutex_);
    }

    void unlock() {
        pthread_mutex_unlock(&mutex_);
    }

    pthread_mutex_t* get() {
        return &mutex_;
    }

 private:
    pthread_mutex_t mutex_;

    // 友元类不受访问权限影响
    friend class ConditionVariable;
};

class LockGuard : NonCopyAble {
 public:
    explicit LockGuard(Lock& mutex) 
        : mutex_(mutex) {
        mutex_.lock();
    }

    ~LockGuard() {
        mutex_.unlock();
    }

 private:
    MutexLock& mutex_;
};

class ConditionVariable : NonCopyAble {
 public:
    explicit ConditionVariable(MutexLock& mutex) 
        : mutex_(mutex_) {
        pthread_cond_init(&cond_, NULL);
    }

    ~ConditionVariable() {
        pthread_cond_destroy(&cond_);
    }

    void wait() {
        pthread_cond_wait(&cond_, mutex_.get());
    }
    
    bool wait_for_seconds(int seconds) {
        struct timespec abstime;
        clock_gettime(CLOCK_REALTIME, &abstime);
        abstime.tv_sec += static_cast<time_t>(seconds);
        return ETIMEDOUT == pthread_cond_timedwait(&cond_, mutex_.get(), &abstime);
    }

    void notify() {
        pthread_cond_signal(&cond_);
    }
    
    void notify_all() {
        pthread_cond_broadcast(&cond_);
    }

 private:
    MutexLock& mutex_;
    pthread_cond_t cond_;
};

// CountDownLatch的主要作用是确保Thread中传进去的func真的启动了以后
// 外层的start才返回
class CountDownLatch : NonCopyAble {
 public:
    explicit CountDownLatch(int count)
        : mutex_(), condition_(mutex_), count_(count) {}
    
    ~CountDownLatch() {}

    void wait() {
        LockGuard lock(mutex_);
        while (count_ > 0) {
            condition_.wait();
        }
    }

    void count_down() {
        LockGuard lock(mutex_);
        --count_;
        if (count_ == 0) {
            condition_.notify_all();
        }
    }

 private:
    mutable MutexLock mutex_;
    ConditionVariable condition_;
    int count_;
};

#endif
