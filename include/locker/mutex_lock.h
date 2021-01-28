#ifndef LOCKER_MUTEX_LOCK_H_
#define LOCKER_MUTEX_LOCK_H_

#include <stdio.h>
#include <time.h>
#include <pthread.h>

#include "utility/noncopyable.h"

namespace locker {
class ConditionVariable;

//c++类不指定默认private方式继承 
//private继承的父类的public, protected都会成为子类的private
class MutexLock : utility::NonCopyAble {
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

//互斥锁RAII
class LockGuard : utility::NonCopyAble {
 public:
    explicit LockGuard(MutexLock& mutex) 
        : mutex_(mutex) {
        mutex_.lock();
    }

    ~LockGuard() {
        mutex_.unlock();
    }

 private:
    MutexLock& mutex_;
};

//条件变量
class ConditionVariable : utility::NonCopyAble {
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

}  // namespace locker

#endif  // LOCKER_MUTEX_LOCK_H_
