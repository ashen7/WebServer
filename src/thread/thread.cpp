#include "thread/thread.h"

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>
#include <memory>

namespace thread_local_storage {
// TLS
__thread int tls_thread_id = 0;                     //线程id
__thread char tls_thread_id_str[32];                //线程id字符串
__thread int tls_thread_id_str_len = 6;             //线程id字符串长度
__thread const char* tls_thread_name = "default";   //线程名字

//得到线程id
pid_t get_tid() {
    return static_cast<pid_t>(::syscall(SYS_gettid));
}

//得到线程id syscall 并赋值线程id字符串, 线程id字符串长度
void cache_thread_id() {
    if (tls_thread_id == 0) {
        tls_thread_id = get_tid();
        tls_thread_id_str_len = snprintf(tls_thread_id_str, sizeof(tls_thread_id_str), "%5d ", tls_thread_id);
    }
}

}  // namespace thread_local_storage

namespace thread {
// 为了在线程中保留thread_name,thread_id这些数据
class ThreadData : utility::NonCopyAble {
 public:
    typedef Thread::Worker Worker;

    ThreadData(const Worker& worker,
               const std::string& thread_name,
               pid_t* thread_id,
               utility::CountDownLatch* count_down_latch)
        : worker_(worker), 
          thread_name_(thread_name), 
          thread_id_(thread_id), 
          count_down_latch_(count_down_latch) {
    }

    ~ThreadData() {
    }

    //得到线程id 运行线程函数 标记该线程为finished
    void Run() {
        //赋值线程id
        *thread_id_ = thread_local_storage::thread_id();
        thread_id_ = NULL;
        //倒计时 唤醒线程
        count_down_latch_->count_down();
        count_down_latch_ = NULL;
        //设置线程名
        thread_local_storage::tls_thread_name = thread_name_.empty() ? "Thread" : thread_name_.c_str();
        prctl(PR_SET_NAME, thread_local_storage::tls_thread_name);

        //执行线程函数
        worker_();
        thread_local_storage::tls_thread_name = "finished";
    }

 public:
    Worker worker_;
    std::string thread_name_;
    pid_t* thread_id_;
    utility::CountDownLatch* count_down_latch_;
};

// Thread类
Thread::Thread(const Worker& worker, const std::string& thread_name)
    : worker_(worker),
      pthread_id_(0),
      thread_id_(0),
      thread_name_(thread_name), 
      is_started_(false),
      is_joined_(false),
      count_down_latch_(1) {
    if (thread_name_.empty()) {
        thread_name_ = "Thread";
    }
}

Thread::~Thread() {
    //如果线程已经开始 但是没有join 就detach线程
    if (is_started_ && !is_joined_) {
        pthread_detach(pthread_id_);
    }
}

//开始线程  调用Run Run会调用ThreadData的Run函数 执行worker线程函数
void Thread::Start() {
    assert(!is_started_);
    is_started_ = true;

    auto thread_data = std::make_shared<ThreadData>(worker_, thread_name_, &thread_id_, &count_down_latch_);
    if (pthread_create(&pthread_id_, NULL, &Run, (void*)thread_data.get())) {
        is_started_ = false;
    } else {
        //先阻塞 等待count_down来唤醒
        count_down_latch_.wait();
        //唤醒后 查看线程id是否大于0
        assert(thread_id_ > 0);
    }
}

//join线程
int Thread::Join() {
    assert(is_started_);
    assert(!is_joined_);
    is_joined_ = true;

    return pthread_join(pthread_id_, NULL);
}

// 调用ThreadData的Run函数 执行worker线程函数
void* Thread::Run(void* thread_obj) {
    ThreadData* thread_data = static_cast<ThreadData*>(thread_obj);
    thread_data->Run();

    return NULL;
}

}  // namespace thread
