#ifndef LOG_LOGGING_H_
#define LOG_LOGGING_H_

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <string>

#include "log_stream.h"

namespace log {
class Logging {
 public:
    Logging(const char* file_name, int line);
    ~Logging();

    static std::string file_name() {
        return file_name_;
    }

    static void set_file_name(std::string file_name) {
        file_name_ = file_name;
    }

    LogStream& stream() {
        return impl_.stream_;
    }

 private:
    class Impl {
     public:
        Impl(const char* file_name, int line);
        void FormatTime();

        LogStream stream_;
        int line_;
        std::string file_name_;
    };

 private:
    static std::string file_name_;
    Impl impl_;
};

}  // namespace log

//宏定义
#define LOG log::Logging(__FILE__, __LINE__).stream()

#endif  // LOG_LOGGING_H_