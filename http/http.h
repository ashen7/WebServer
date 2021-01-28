#ifndef HTTP_H_
#define HTTP_H_

#include <sys/epoll.h>
#include <unistd.h>

#include <string>
#include <memory>
#include <functional>
#include <map>
#include <unordered_map>

#include "timer/timer.h"

class EventLoop;
class Timer;
class Channel;

class MimeType {
 public:
    static std::string get_mime(const std::string& type);

 private:
    MimeType();
    MimeType(const MimeType& mime_type);

    static void Init();

 private:
    static std::unordered_map<std::string, std::string> mime_map;
    static pthread_once_t once_control;
};

class Http : public std::enable_shared_from_this<Http> {
 public:
    enum ProcessState {
        STATE_PARSE_URI,
        STATE_PARSE_HEADERS,
        STATE_RECV_BODY,
        STATE_RESPONSE,
        STATE_FINISH
    };

    enum UriState {
        PARSE_URI_SUCCESS,
        PARSE_URI_AGAIN,
        PARSE_URI_ERROR,
    };

    enum HeaderState {
        PARSE_HEADER_SUCCESS,
        PARSE_HEADER_AGAIN,
        PARSE_HEADER_ERROR
    };

    enum ResponseState { 
        RESPONSE_SUCCESS,
        RESPONSE_ERROR 
    };

    enum ParseState {
        START,
        KEY,
        COLON,
        SPACES_AFTER_COLON,
        VALUE,
        CR,
        LF,
        END_CR,
        END_LF
    };

    enum ConnectionState { 
        CONNECTED, 
        DISCONNECTING, 
        DISCONNECTED 
    };

    enum RequestMethod { 
        METHOD_GET, 
        METHOD_POST, 
        METHOD_HEAD 
    };

    enum HttpVersion { 
        HTTP_10, 
        HTTP_11 
    };

 public:
    Http(EventLoop* loop, int connfd);
    ~Http();

    void HandleClose();
    void NewEvent();
    void Reset();
    void SeperateTimer();
    
    void LinkTimer(std::shared_ptr<Timer> timer) {
        timer_ = timer;
    }
    
    std::shared_ptr<Channel> get_channel() {
        return channel_;
    }

    EventLoop* get_event_loop() {
        return event_loop_;
    }

 private:
    void HandleRead();
    void HandleWrite();
    void HandleConnect();
    void HandleError(int fd, int err_num, std::string short_msg);

    UriState ParseUri();
    HeaderState ParseHeaders();
    ResponseState Response();

 private:
    int connect_fd_;
    EventLoop* event_loop_;
    std::shared_ptr<Channel> channel_;
    std::weak_ptr<Timer> timer_;
    std::string read_buffer_;
    std::string write_buffer_;

    bool is_error_;
    bool is_keep_alive_;
    ConnectionState connection_state_;
    ProcessState process_state_;
    ParseState parse_state_;
    RequestMethod method_;
    HttpVersion version_;
    
    std::string file_name_;
    std::string path_;
    int cur_read_pos_;
    std::map<std::string, std::string> headers_;
};

#endif