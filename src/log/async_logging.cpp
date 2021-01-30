#include "log/async_logging.h"

#include <assert.h>
#include <stdio.h>
#include <unistd.h>

#include <functional>

namespace log {
AsyncLogging::AsyncLogging(std::string file_name, int flush_interval)
    : file_name_(file_name),
      flush_interval_(flush_interval),
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
    buffers_.reserve(16);
}

//写日志
void AsyncLogging::Write(const char* single_log, int size) {
    locker::LockGuard lock(mutex_);
    {
        if (current_buffer_->capacity() > size) {
            current_buffer_->write(single_log, size);
        } else {
            buffers_.push_back(current_buffer_);
            current_buffer_.reset();
            if (next_buffer_) {
                current_buffer_ = std::move(next_buffer_);
            } else {
                current_buffer_.reset(new Buffer);
            }
            current_buffer_->write(single_log, size);
            condition_.notify();
        }
    }
}

//线程函数
void AsyncLogging::Worker() {
    assert(is_running_ == true);
    count_down_latch_.count_down();
    LogFile log_file(file_name_);
    std::shared_ptr<Buffer> new_buffer1(new Buffer);
    std::shared_ptr<Buffer> new_buffer2(new Buffer);
    new_buffer1->bzero();
    new_buffer2->bzero();
    std::vector<std::shared_ptr<Buffer>> buffers;
    buffers.reserve(16);
    while (is_running_) {
        assert(new_buffer1 && new_buffer1->size() == 0);
        assert(new_buffer2 && new_buffer2->size() == 0);
        assert(buffers.empty());

        {
            locker::LockGuard lock(mutex_);
            if (buffers_.empty()) {
                condition_.wait_for_seconds(flush_interval_);
            }
            buffers_.push_back(current_buffer_);
            current_buffer_.reset();

            current_buffer_ = std::move(new_buffer1);
            buffers.swap(buffers_);
            if (!next_buffer_) {
                next_buffer_ = std::move(new_buffer2);
            }
        }
        assert(!buffers.empty());

        if (buffers.size() > 25) {
            buffers.erase(buffers.begin() + 2, buffers.end());
        }

        for (size_t i = 0; i < buffers.size(); ++i) {
            log_file.Write(buffers[i]->buffer(), buffers[i]->size());
        }

        if (buffers.size() > 2) {
            // drop non-bzero-ed buffers, avoid trashing
            buffers.resize(2);
        }

        if (!new_buffer1) {
            assert(!buffers.empty());
            new_buffer1 = buffers.back();
            buffers.pop_back();
            new_buffer1->reset();
        }

        if (!new_buffer2) {
            assert(!buffers.empty());
            new_buffer2 = buffers.back();
            buffers.pop_back();
            new_buffer2->reset();
        }

        buffers.clear();
        log_file.Flush();
    }
}

}  // namespace log
