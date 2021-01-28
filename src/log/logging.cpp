
#include "logging.h"

#include <assert.h>
#include <sys/time.h>
#include <time.h>

#include <iostream>

#include "async_logging.h"
#include "thread/thread.h"

static pthread_once_t once_control = PTHREAD_ONCE_INIT;
static AsyncLogging* async_logging = nullptr;

std::string Logging::log_filename_ = "./WebServer.log";

void once_init() {
    async_logging = new AsyncLogging(Logging::log_filename());
    async_logging->Start();
}

void output(const char* msg, int len) {
    pthread_once(&once_control, once_init);
    async_logging->Append(msg, len);
}

Logging::Logging(const char* filename, int line) 
    : impl_(filename, line) {
}

Logging::~Logging() {
    impl_.stream_ << " -- " << impl_.basename_ << ':' << impl_.line_ << '\n';
    const LogStream::Buffer& buf(stream().buffer());
    output(buf.data(), buf.length());
}

Logging::Impl::Impl(const char* filename, int line)
    : stream_(), line_(line), basename_(filename) {
    formatTime();
}

void Logging::Impl::formatTime() {
    struct timeval tv;
    time_t time;
    char str_t[26] = {0};
    gettimeofday(&tv, NULL);
    time = tv.tv_sec;
    struct tm* p_time = localtime(&time);
    strftime(str_t, 26, "%Y-%m-%d %H:%M:%S\n", p_time);
    stream_ << str_t;
}