#ifndef LOGGING_H_
#define LOGGING_H_

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <string>

#include "log_stream.h"

class AsyncLogging;

class Logging {
 public:
    Logging(const char* fileName, int line);
    ~Logging();

    LogStream& stream() {
        return impl_.stream_;
    }

    static std::string log_filename() {
        return log_filename_;
    }

    static void set_log_filename(std::string filename) {
        log_filename_ = filename;
    }

 private:
    class Impl {
     public:
        Impl(const char* filename, int line);
        void formatTime();

        LogStream stream_;
        int line_;
        std::string basename_;
    };

 private:
    static std::string log_filename_;
    Impl impl_;
};

#define LOG Logging(__FILE__, __LINE__).stream()

#endif