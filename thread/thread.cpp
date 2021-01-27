#include "thread.h"

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>
#include <memory>

namespace current_thread {
// TLS
__thread int tls_thread_id = 0;
__thread char tls_thread_id_str[32];
__thread int tls_thread_id_str_len = 6;
__thread const char* tls_thread_name = "default";

pid_t get_tid() {
    return static_cast<pid_t>(::syscall(SYS_gettid));
}

void current_thread::cache_thread_id() {
    if (tls_thread_id == 0) {
        tls_thread_id = get_tid();
        tls_thread_id_str_len = snprintf(tls_thread_id_str, sizeof(tls_thread_id_str), "%5d ", tls_thread_id);
    }
}

}  // namespace current_thread

// 为了在线程中保留thread_name,thread_id这些数据
struct ThreadData {
 public:
    typedef Thread::Worker Worker;

    ThreadData(const Worker& worker,
               const string& thread_name,
               pid_t* thread_id,
               CountDownLatch* count_down_latch)
        : worker_(worker), 
          thread_name_(thread_name), 
          thread_id_(thread_id), 
          count_down_latch_(count_down_latch) {
    }

    void RunInThread() {
        //给线程id和倒计时赋值
        *thread_id_ = current_thread::thread_id();
        thread_id_ = NULL;
        count_down_latch_->count_down();
        count_down_latch_ = NULL;

        current_thread::tls_thread_name = thread_name_.empty() ? "Thread" : thread_name_.c_str();
        prctl(PR_SET_NAME, current_thread::tls_thread_name);

        //执行线程函数
        worker_();
        current_thread::tls_thread_name = "finished";
    }

 public:
    Worker worker_;
    string thread_name_;
    pid_t* thread_id_;
    CountDownLatch* count_down_latch_;
};

// Thread类
Thread::Thread(const Worker& worker, const std::string& thread_name)
    : worker_(worker),
      thread_name_(thread_name), 
      pthread_id_(0),
      thread_id_(0),
      is_start_(false),
      is_join_(false),
      count_down_latch_(1) {
    if (thread_name_.empty()) {
        thread_name_ = "Thread";
    }
}

Thread::~Thread() {
    if (is_start_ && !is_join_) {
        pthread_detach(pthread_id_);
    }
}

void Thread::Start() {
    assert(!is_start_);
    is_start_ = true;

    ThreadData* thread_data = new ThreadData(worker_, thread_name_, &thread_id_, &count_down_latch_);
    if (pthread_create(&pthread_id_, NULL, &Run, (void*)thread_data)) {
        is_start_ = false;
        delete thread_data;
    } else {
        count_down_latch_.wait();
        assert(thread_id_ > 0);
    }
}

int Thread::Join() {
    assert(is_start_);
    assert(!is_join_);
    is_join_ = true;

    return pthread_join(pthread_id_, NULL);
}

static void* Run(void* thread_obj) {
    ThreadData* thread_data = static_cast<ThreadData*>(thread_obj);
    thread_data->RunInThread();
    delete thread_data;

    return NULL;
}