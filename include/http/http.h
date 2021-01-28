#ifndef HTTP_HTTP_H_
#define HTTP_HTTP_H_

#include <sys/epoll.h>
#include <unistd.h>

#include <string>
#include <memory>
#include <functional>
#include <map>
#include <unordered_map>

#include "timer/timer.h"

namespace http {
//类的前置声明
class event::EventLoop;
class event::Channel;
class timer::Timer;

//http mime文件类型
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

// std::enable_shared_from_this 当类http被std::shared_ptr管理，
// 且在类http的成员函数里需要把当前类对象作为参数传给其他函数时，就需要传递一个指向自身的std::shared_ptr
class Http : public std::enable_shared_from_this<Http> {
 public:
    //处理状态
    enum ProcessState {
        STATE_PARSE_URI,
        STATE_PARSE_HEADERS,
        STATE_RECV_BODY,
        STATE_RESPONSE,
        STATE_FINISH
    };

    //解析uri（请求行）的状态
    enum UriState {
        PARSE_URI_SUCCESS,
        PARSE_URI_AGAIN,
        PARSE_URI_ERROR,
    };

    //解析请求头的状态
    enum HeaderState {
        PARSE_HEADER_SUCCESS,
        PARSE_HEADER_AGAIN,
        PARSE_HEADER_ERROR
    };

    //构建响应报文时的状态
    enum ResponseState { 
        RESPONSE_SUCCESS,
        RESPONSE_ERROR 
    };

    //解析状态
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

    //服务器和客户端的连接状态
    enum ConnectionState { 
        CONNECTED, 
        DISCONNECTING, 
        DISCONNECTED 
    };

    //Http请求方法
    enum RequestMethod { 
        METHOD_GET, 
        METHOD_POST, 
        METHOD_HEAD 
    };

    //Http版本
    enum HttpVersion { 
        HTTP_10, 
        HTTP_11 
    };

 public:
    Http(EventLoop* loop, int connfd);
    ~Http();

    void SetDefaultEvent();  //给fd注册默认事件
    void HandleClose();      
    void Reset();
    void SeperateTimer();
    
    void set_timer(std::shared_ptr<Timer> timer) {
        timer_ = timer;
    }
    
    std::shared_ptr<Channel> channel() {
        return channel_;
    }

    EventLoop* event_loop() {
        return event_loop_;
    }

 private:
    void HandleRead();      //处理读
    void HandleWrite();     //处理写
    void HandleConnect();   //处理连接
    void HandleError(int fd, int err_num, std::string short_msg);   //处理错误

    UriState ParseUri();          //解析uri（请求行）
    HeaderState ParseHeaders();   //解析请求头
    ResponseState Response();     //构建响应报文

 private:
    static constexpr int DEFAULT_EVENT = EPOLLIN | EPOLLET | EPOLLONESHOT;
    static constexpr int DEFAULT_EXPIRE_TIME = 2000;               // ms
    static constexpr int DEFAULT_KEEP_ALIVE_TIME = 5 * 60 * 1000;  // ms

    int connect_fd_;                             //连接套接字fd
    event::EventLoop* event_loop_;               //事件循环
    std::shared_ptr<event::Channel> channel_;    //管道
    std::weak_ptr<timer::Timer> timer_;          //定时器
    std::string read_buffer_;                    //读缓冲区
    std::string write_buffer_;                   //写缓冲区

    ProcessState process_state_;                 //处理状态
    ConnectionState connection_state_;           //服务器和客户端的连接状态
    ParseState parse_state_;                     //解析状态 
    RequestMethod method_;                       //http请求方法
    HttpVersion version_;                        //http版本
    
    std::string file_name_;                      //请求文件名
    std::string path_;                           //请求路径
    int cur_read_pos_;                           //当前读的位置
    std::map<std::string, std::string> headers_; //头
    bool is_error_;                              //是否发生错误
    bool is_keep_alive_;                         //是否长连接
};

}  // namespace http

#endif  // HTTP_HTTP_H_