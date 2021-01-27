#ifndef THREAD_H_
#define THREAD_H_

#include <pthread.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <functional>
#include <memory>
#include <string>

#include "locker/mutex_lock.h"

namespace current_thread {
// __thread: TLS线程局部存储 每个当前线程都有一个该变量的实例
extern __thread int tls_thread_id;
extern __thread char tls_thread_id_str[32];
extern __thread int tls_thread_id_str_len;
extern __thread const char* tls_thread_name;

void cache_thread_id();

inline int thread_id() {
    if (__builtin_expect(tls_thread_id == 0, 0)) {
        cache_thread_id();
    }

    return tls_thread_id;
}

inline const char* thread_id_str() {
    return tls_thread_id_str;
}

inline int thread_id_str_len() {
    return tls_thread_id_str_len;
}

inline const char* thread_name() {
    return tls_thread_name;
}

}  // namespace current_thread

class Thread {
 public:
    typedef std::function<void()> Worker;

    explicit Thread(const Worker&, const std::string& thread_name = "");
    ~Thread();

    void Start();
    int Join();

    bool is_start() const {
        return is_start_;
    }

    pid_t thread_id() const {
        return thread_id_;
    }

    const std::string& thread_name() const {
        return thread_name_;
    }

 private:
    static void* Run(void* thread_obj);

 private:
    Worker worker_;
    pthread_t pthread_id_;
    pid_t thread_id_;
    std::string thread_name_;
    CountDownLatch count_down_latch_;

    bool is_start_;
    bool is_join_;
};

#endif