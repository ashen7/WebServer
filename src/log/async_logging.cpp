#include "log/async_logging.h"

#include <assert.h>
#include <stdio.h>
#include <unistd.h>

#include <functional>

namespace log {
AsyncLogging::AsyncLogging(std::string file_name, int timeout)
    : file_name_(file_name),
      timeout_(timeout),
      mutex_(),
      condition_(mutex_),
      count_down_latch_(1),
      thread_(std::bind(&AsyncLogging::Worker, this), "Logging"),
      current_buffer_(new Buffer),
      next_buffer_(new Buffer),
      buffers_(),
      is_running_(false) {
    assert(file_name_.size() > 1);
    current_buffer_->bzero();
    next_buffer_->bzero();
    buffers_.reserve(10);
}

AsyncLogging::~AsyncLogging() {
    if (is_running_) {
        Stop();
    }
}

//将日志写入buffer输出缓冲区中
void AsyncLogging::Write(const char* single_log, int size, bool is_fatal) {
    locker::LockGuard lock(mutex_);
    {
        //buffer没写满 就一直写
        if (current_buffer_->capacity() > size) {
            current_buffer_->write(single_log, size);
            if (is_fatal) {
                Stop();
                abort();
            }
        } else {
            //这里buffer写满了 将buffer存入buffers
            buffers_.push_back(current_buffer_);
            current_buffer_.reset();
            //当前buffer满了 就new一个新buffer写 如果next buffer有值 就把所有权给当前buffer
            if (next_buffer_) {
                current_buffer_ = std::move(next_buffer_);
            } else {
                current_buffer_.reset(new Buffer);
            }
            current_buffer_->write(single_log, size);
            //这里如果是fatal的日志，那么写完这条日志就停止线程 然后abort
            if (is_fatal) {
                Stop();
                abort();
            }
            //唤醒线程拿buffer的数据 写入日志文件
            condition_.notify();
        }
    }
}

//开始线程
void AsyncLogging::Start() {
    is_running_ = true;
    thread_.Start();
    count_down_latch_.wait();
}

//停止线程
void AsyncLogging::Stop() {
    is_running_ = false;
    condition_.notify();
    thread_.Join();
}

//线程函数 程序里写日志都是写到buffer中，而从buffer写入磁盘速度很慢(文件io)，所以开个线程异步写
void AsyncLogging::Worker() {
    assert(is_running_);
    //倒计时
    count_down_latch_.count_down();
    //log file
    LogFile log_file(file_name_);
    
    std::vector<std::shared_ptr<Buffer>> buffers;
    buffers.reserve(10);

    while (is_running_) {
        assert(buffers.empty());
        {
            locker::LockGuard lock(mutex_);
            //这里等待buffer写满存入buffers
            if (buffers_.empty()) {
                condition_.wait_for_seconds(timeout_);
            }
            //buffers再加上刚才写的buffer
            buffers_.push_back(current_buffer_);
            current_buffer_.reset();
            //将buffers move到本地来
            buffers.swap(buffers_);
        }
        assert(!buffers.empty());
        
        //遍历buffers 每个buffer数据写入log文件
        for (auto buffer : buffers) {
            log_file.Write(buffer->buffer(), buffer->size());
        }

        //清理buffers 
        buffers.clear();
        //flush日志文件
        log_file.Flush();
    }
}

}  // namespace log
