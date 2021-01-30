#ifndef LOG_ASYNC_LOGGING_H_
#define LOG_ASYNC_LOGGING_H_

#include <functional>
#include <string>
#include <vector>

#include "log_stream.h"
#include "locker/mutex_lock.h"
#include "utility/count_down_latch.h"
#include "thread/thread.h"

namespace log {
//操作文件的工具类
class FileUtils {
 public:
    explicit FileUtils(std::string file_name)
        : fp_(fopen(file_name.c_str(), "ae")) {
        setbuffer(fp_, buffer_, sizeof(buffer_));
    }

    ~FileUtils() {
        fclose(fp_);
    }

    // write 会向文件写
    void Write(const char* single_log, const size_t size) {
        size_t write_size = fwrite_unlocked(single_log, 1, size, fp_);
        size_t remain = size - write_size;
        //如果一次没写完 就继续写
        while (remain > 0) {
            size_t x = fwrite_unlocked(single_log + write_size, 1, remain, fp_);
            if (x == 0) {
                int error = ferror(fp_);
                if (error) {
                    fprintf(stderr, "FileUtils::append() failed !\n");
                }
                break;
            }
            write_size += x;
            remain = size - write_size;
        }
    }

    void Flush() {
        fflush(fp_);
    }

 private:
    FILE* fp_;
    char buffer_[64 * 1024];
};

// TODO: 提供自动归档功能
class LogFile : utility::NonCopyAble {
 public:
    // 每写flush_every_n次，就会flush一次
    LogFile(const std::string& file_name, int flush_every_n = 1024)
        : file_name_(file_name),
          flush_every_n_(flush_every_n),
          count_(0),
          mutex_() {
        file_.reset(new FileUtils(file_name));
    }
    
    ~LogFile() {
    }

    void Write(const char* single_log, int size) {
        locker::LockGuard lock(mutex_);
        {
            // 每写flush_every_n次，就会flush一次
            file_->Write(single_log, size);
            ++count_;
            if (count_ >= flush_every_n_) {
                count_ = 0;
                file_->Flush();
            }
        }
    }

    void Flush() {
        locker::LockGuard lock(mutex_);
        {
            file_->Flush();
        }
    }

 private:

 private:
    const std::string file_name_;
    const int flush_every_n_;
    int count_;
    locker::MutexLock mutex_;
    std::unique_ptr<FileUtils> file_;
};

class AsyncLogging : utility::NonCopyAble {
 public:
    AsyncLogging(const std::string file_name, int flush_interval = 2);
    ~AsyncLogging() {
        if (is_running_) {
            Stop();
        }
    }

    void Write(const char* single_log, int size);

    //开始线程
    void Start() {
        is_running_ = true;
        thread_.Start();
        count_down_latch_.wait();
    }

    //停止线程
    void Stop() {
        is_running_ = false;
        condition_.notify();
        thread_.Join();
    }

 private:
    void Worker();  //线程函数

 private:
    typedef FixedBuffer<kLargeBufferSize> Buffer;
    
    std::string file_name_;
    const int flush_interval_;
    bool is_running_;

    std::shared_ptr<Buffer> current_buffer_;
    std::shared_ptr<Buffer> next_buffer_;
    std::vector<std::shared_ptr<Buffer>> buffers_;

    thread::Thread thread_;
    locker::MutexLock mutex_;
    locker::ConditionVariable condition_;
    utility::CountDownLatch count_down_latch_;
};

}  // namespace log

#endif  // LOG_ASYNC_LOGGING_H_