
#include "log/logging.h"

#include <assert.h>
#include <sys/time.h>
#include <time.h>

#include <iostream>

#include "log/async_logging.h"
#include "thread/thread.h"

namespace log {
static pthread_once_t once_control = PTHREAD_ONCE_INIT;
static AsyncLogging* async_logging = nullptr;

std::string Logging::file_name_ = "./web_server.log";

//初始化异步日志 并start线程
void OnceInit() {
    async_logging = new AsyncLogging(Logging::file_name());
    async_logging->Start();
}

//运行异步日志线程, 写日志
void Write(const char* single_log, int size) {
    //只会执行一次的函数
    pthread_once(&once_control, OnceInit);
    async_logging->Write(single_log, size);
}

//logging
Logging::Logging(const char* file_name, int line) 
    : impl_(file_name, line) {
}

Logging::~Logging() {
    impl_.stream_ << " -- " << impl_.file_name_ << ':' << impl_.line_ << '\n';
    const LogStream::Buffer& buffer(stream().buffer());
    Write(buffer.buffer(), buffer.size());
}

Logging::Impl::Impl(const char* file_name, int line)
    : stream_(), 
      file_name_(file_name), 
      line_(line) {
    FormatTime();
}

void Logging::Impl::FormatTime() {
    struct timeval now;
    gettimeofday(&now, NULL);
    struct tm* current_time = localtime(&now.tv_sec);

    char time_str[26] = { 0 };
    strftime(time_str, 26, "%Y-%m-%d %H:%M:%S\n", current_time);
    stream_ << time_str;
}

}  // namespace log
