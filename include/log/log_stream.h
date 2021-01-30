#ifndef LOG_LOG_STREAM_H_
#define LOG_LOG_STREAM_H_

#include <assert.h>
#include <string.h>
#include <string>

#include "utility/noncopyable.h"

namespace log {
//类的前置声明
class AsyncLogging;
constexpr int kSmallBufferSize = 4000;
constexpr int kLargeBufferSize = 4000 * 1000;

//固定的缓冲区
template <int BufferSize>
class FixedBuffer : utility::NonCopyAble {
 public:
    FixedBuffer() 
        : current_buffer_(buffer_) {
    }

    ~FixedBuffer() {
    }

    //copy buffer的size个字节到current_buffer_来
    void write(const char* buffer, int size) {
        if (capacity() > size) {
            memcpy(current_buffer_, buffer, size);
            this->add(size);
        }
    }

    //源buffer
    const char* buffer() const {
        return buffer_;
    }

    //当前buffer(buffer+偏移量)
    char* current_buffer() {
        return current_buffer_;
    }

    //当前buufer的偏移量（相对buffer起始地址的偏移量）
    int size() const {
        return static_cast<int>(current_buffer_ - buffer_);
    }

    //剩余buffer偏移量 = 总buffer的偏移量 - 当前buffer的偏移量
    int capacity() const {
        return static_cast<int>(this->end() - current_buffer_);
    }

    //当前buffer偏移size个字节
    void add(size_t size) {
        current_buffer_ += size;
    }

    //将当前buffer偏移量置为0,即为buffer
    void reset() {
        current_buffer_ = buffer_;
    }

    //给buffer置0
    void bzero() {
        memset(buffer_, 0, sizeof(buffer_));
    }

 private:
    const char* end() const {
        return buffer_ + sizeof(buffer_);
    }

    char buffer_[BufferSize];
    char* current_buffer_;
};

//输出流对象 重载输出流运算符<<  将值写入buffer中 
class LogStream : utility::NonCopyAble {
 public:
    typedef FixedBuffer<kSmallBufferSize> Buffer;

    LogStream() {
    }
    ~LogStream() {
    }

    //重载输出流运算符<<  将值写入buffer中 
    LogStream& operator<<(bool value);

    LogStream& operator<<(short number);
    LogStream& operator<<(unsigned short);
    LogStream& operator<<(int);
    LogStream& operator<<(unsigned int);
    LogStream& operator<<(long);
    LogStream& operator<<(unsigned long);
    LogStream& operator<<(long long);
    LogStream& operator<<(unsigned long long);

    LogStream& operator<<(float value);
    LogStream& operator<<(double);
    LogStream& operator<<(long double);

    LogStream& operator<<(char str);
    LogStream& operator<<(const char*);
    LogStream& operator<<(const unsigned char*);
    LogStream& operator<<(const std::string&);

    void Write(const char* buffer, int size) {
        buffer_.write(buffer, size);
    }

    const Buffer& buffer() const {
        return buffer_;
    }

    void reset_buffer() {
        buffer_.reset();
    }

 private:
    template <typename T>
    void FormatInt(T);

 private:
    static constexpr int kMaxNumberSize = 32;

    Buffer buffer_;
};

}  // namespace log

#endif  // LOG_LOG_STREAM_H_