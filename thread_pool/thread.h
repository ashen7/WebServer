#ifndef THREAD_H_
#define THREAD_H_

#include <pthread.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <functional>
#include <memory>
#include <string>

#include "utility/mutex_lock.h"

namespace current_thread {

// __thread: TLS线程局部存储 每个当前线程都有一个该变量的实例
extern __thread int tls_cached_tid;
extern __thread char tls_tid_string[32];
extern __thread int tls_tid_length;
extern __thread const char* tls_thread_name;

void cache_tid();

inline int tid() {
    if (__builtin_expect(t_cachedTid == 0, 0)) {
        cache_tid();
    }

    return tls_cached_tid;
}

inline const char* tid_string() {
    return tls_tid_string;
}

inline int tid_length() {
    return tls_tid_string_len;
}

inline const char* name() {
    return tls_thread_name;
}

}  // namespace current_thread

class Thread {
 public:
    typedef std::function<void()> ThreadFunc;

    explicit Thread(const ThreadFunc&, const std::string& name = std::string());
    ~Thread();

    void start();
    int join();

    bool started() const {
        return started_;
    }

    pid_t tid() const {
        return process_id_;
    }

    const std::string& name() const {
        return name_;
    }

 private:
    void set_default_name();

 private:
    bool started_;
    bool joined_;
    pthread_t thread_id_;
    pid_t process_id_;
    ThreadFunc func_;
    std::string name_;
    CountDownLatch latch_;
};

#endif