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

#include "current_thread.h"

namespace current_thread {

__thread int tls_cached_tid = 0;
__thread char tls_tid_string[32];
__thread int tls_tid_length = 6;
__thread const char* tls_thread_name = "default";

}  // namespace current_thread

pid_t get_tid() {
    return static_cast<pid_t>(::syscall(SYS_gettid));
}

void current_thread::cache_tid() {
    if (tls_cached_tid == 0) {
        tls_cached_tid = gettid();
        tls_tid_length = snprintf(tls_tid_string, sizeof tls_tid_string, "%5d ", tls_cached_tid);
    }
}

// 为了在线程中保留name,tid这些数据
struct ThreadData {
 public:
    typedef Thread::ThreadFunc ThreadFunc;

    ThreadData(const ThreadFunc& func,
               const string& name,
               pid_t* tid,
               CountDownLatch* latch)
        : func_(func), name_(name), process_id_(tid), latch_(latch) {}

    void RunInThread() {
        *process_id_ = current_thread::tid();
        process_id_ = NULL;
        latch_->count_down();
        latch_ = NULL;

        current_thread::tls_thread_name = name_.empty() ? "Thread" : name_.c_str();
        prctl(PR_SET_NAME, current_thread::tls_thread_name);

        func_();
        current_thread::tls_thread_name = "finished";
    }

 public:
    ThreadFunc func_;
    string name_;
    pid_t* process_id_;
    CountDownLatch* latch_;
};

void* StartThread(void* obj) {
    ThreadData* thread_data = static_cast<ThreadData*>(obj);
    thread_data->RunInThread();
    delete thread_data;

    return NULL;
}

Thread::Thread(const ThreadFunc& func, const string& n)
    : started_(false),
      joined_(false),
      thread_id_(0),
      process_id_(0),
      func_(func),
      name_(n),
      latch_(1) {
    set_default_name();
}

Thread::~Thread() {
    if (started_ && !joined_) {
        pthread_detach(thread_id_);
    }
}

void Thread::set_default_name() {
    if (name_.empty()) {
        char buf[32];
        snprintf(buf, sizeof buf, "Thread");
        name_ = buf;
    }
}

void Thread::start() {
    assert(!started_);
    started_ = true;
    ThreadData* data = new ThreadData(func_, name_, &process_id_, &latch_);
    if (pthread_create(&thread_id_, NULL, &StartThread, data)) {
        started_ = false;
        delete data;
    } else {
        latch_.wait();
        assert(process_id_ > 0);
    }
}

int Thread::join() {
    assert(started_);
    assert(!joined_);
    joined_ = true;

    return pthread_join(thread_id_, NULL);
}