#ifndef ASYNC_LOGGING_H_
#define ASYNC_LOGGING_H_

#include <functional>
#include <string>
#include <vector>

#include "log_stream.h"
#include "locker/mutex_lock.h"
#include "thread/thread.h"

class AppendFile {
 public:
    explicit AppendFile(std::string filename)
        : fp_(fopen(filename.c_str(), "ae")) {
        setbuffer(fp_, buffer_, sizeof buffer_);
    }

    ~AppendFile() {
        fclose(fp_);
    }

    // append 会向文件写
    void append(const char* logline, const size_t len) {
        size_t n = this->write(logline, len);
        size_t remain = len - n;
        while (remain > 0) {
            size_t x = this->write(logline + n, remain);
            if (x == 0) {
                int err = ferror(fp_);
                if (err)
                    fprintf(stderr, "AppendFile::append() failed !\n");
                break;
            }
            n += x;
            remain = len - n;
        }
    }

    void flush() {
        fflush(fp_);
    }

 private:
    size_t write(const char* logline, size_t len) {
        return fwrite_unlocked(logline, 1, len, fp_);
    }

 private:
    FILE* fp_;
    char buffer_[64 * 1024];
};

// TODO 提供自动归档功能
class LogFile : utility::NonCopyAble {
 public:
    // 每被append
    // flushEveryN次，flush一下，会往文件写，只不过，文件也是带缓冲区的
    LogFile(const std::string& basename, int flushEveryN = 1024)
        : basename_(basename),
          flushEveryN_(flushEveryN),
          count_(0),
          mutex_(new MutexLock) {
        // assert(basename.find('/') >= 0);
        file_.reset(new AppendFile(basename));
    
    ~LogFile() {}

    void append(const char* logline, int len) {
        LockGuard lock(*mutex_);
        append_unlocked(logline, len);
    }

    void flush() {
        LockGuard lock(*mutex_);
        file_->flush();
    }

    bool rollFile();

 private:
    void append_unlocked(const char* logline, int len) {
        file_->append(logline, len);
        ++count_;
        if (count_ >= flushEveryN_) {
            count_ = 0;
            file_->flush();
        }
    }

 private:
    const std::string basename_;
    const int flushEveryN_;

    int count_;
    std::unique_ptr<MutexLock> mutex_;
    std::unique_ptr<AppendFile> file_;
};

class AsyncLogging : utility::NonCopyAble {
 public:
    AsyncLogging(const std::string basename, int flushInterval = 2);
    ~AsyncLogging() {
        if (running_)
            stop();
    }
    void append(const char* logline, int len);

    void start() {
        running_ = true;
        thread_.start();
        latch_.wait();
    }

    void stop() {
        running_ = false;
        cond_.notify();
        thread_.join();
    }

 private:
    void threadFunc();

 private:
    typedef FixedBuffer<kLargeBuffer> Buffer;
    typedef std::vector<std::shared_ptr<Buffer>> BufferVector;
    typedef std::shared_ptr<Buffer> BufferPtr;
    
    const int flushInterval_;
    bool running_;
    std::string basename_;
    thread::Thread thread_;
    locker:;MutexLock mutex_;
    locker::ConditionVariable condition_;
    utility::CountDownLatch latch_;
    BufferPtr currentBuffer_;
    BufferPtr nextBuffer_;
    BufferVector buffers_;
};

#endif