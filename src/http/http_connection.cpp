#include "http/http_connection.h"

#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <iostream>
#include <string>

#include "event/channel.h"
#include "event/event_loop.h"
#include "utility/socket_utils.h"
#include "log/logging.h"

namespace http {
//http类 给连接套接字绑定事件处理回调
HttpConnection::HttpConnection(event::EventLoop* event_loop, int connect_fd)
    : event_loop_(event_loop),
      channel_(new event::Channel(connect_fd)),
      connect_fd_(connect_fd),
      cur_read_pos_(0),
      connection_state_(CONNECTED),
      process_state_(STATE_PARSE_URI),
      parse_state_(START),
      method_(METHOD_GET),
      version_(HTTP_11),
      is_error_(false),
      is_keep_alive_(false) {
    //给连接套接字绑定读、写、连接的回调函数
    channel_->set_read_handler(std::bind(&HttpConnection::HandleRead, this));
    channel_->set_write_handler(std::bind(&HttpConnection::HandleWrite, this));
    channel_->set_update_handler(std::bind(&HttpConnection::HandleUpdate, this));
}

//一个http连接析构后 关闭连接套接字
HttpConnection::~HttpConnection() {
    close(connect_fd_);
}

//给fd注册默认事件, 这里给了超时时间，所以会绑定定时器和http对象
void HttpConnection::Register() {
    channel_->set_events(kDefaultEvent);
    event_loop_->PollerAdd(channel_, kDefaultExpireTime);
}

//处理关闭 从epoll事件表中删除fd(绑定的定时器被删除时 会调用此函数)
void HttpConnection::HandleClose() {
    //连接状态变为已关闭
    connection_state_ = DISCONNECTED;
    std::shared_ptr<HttpConnection> guard(shared_from_this());
    event_loop_->PollerDel(channel_);
}

//处理读   读请求报文数据到read_buffer 解析请求报文 构建响应报文并写入write_buffer
void HttpConnection::HandleRead() {
    int& events = channel_->events();
    do {
        bool is_read_zero_bytes = false;
        //读客户端发来的请求数据 存入read_buffer
        int read_bytes = utility::Read(connect_fd_, read_buffer_, is_read_zero_bytes);
        LOG(DEBUG) << "Request " << read_buffer_;
        if (connection_state_ == DISCONNECTING) {
            read_buffer_.clear();
            break;
        }
        if (read_bytes < 0) {
            LOG(WARNING) << "Read bytes < 0, " << strerror(errno);
            //标记为error的都是直接返回错误页面
            is_error_ = true;
            HandleError(connect_fd_, 400, "Bad Request");
            break;
        } else if (is_read_zero_bytes) {
            // 有请求出现但是读不到数据，可能是Request Aborted，或者来自网络的数据没有达到等原因
            // 最可能是对端已经关闭了，统一按照对端已经关闭处理
            connection_state_ = DISCONNECTING;
            if (read_bytes == 0) {
                break;
            }
        }

        //解析请求行
        if (process_state_ == STATE_PARSE_URI) {
            UriState uri_state = ParseUri();
            if (uri_state == PARSE_URI_AGAIN) {
                break;
            } else if (uri_state == PARSE_URI_ERROR) {
                LOG(WARNING) << "Parse request uri error, Sockfd: " 
                             << connect_fd_ << ", " << read_buffer_ << "******";
                read_buffer_.clear();
                //标记为error的都是直接返回错误页面
                is_error_ = true;
                HandleError(connect_fd_, 400, "Bad Request");
                break;
            } else {
                process_state_ = STATE_PARSE_HEADERS;
            }
        }

        //解析请求头
        if (process_state_ == STATE_PARSE_HEADERS) {
            HeaderState header_state = ParseHeaders();
            if (header_state == PARSE_HEADER_AGAIN) {
                break;
            } else if (header_state == PARSE_HEADER_ERROR) {
                LOG(WARNING) << "Parse request header error, Sockfd: " 
                             << connect_fd_ << ", " << read_buffer_ << "******";
                //标记为error的都是直接返回错误页面
                is_error_ = true;
                HandleError(connect_fd_, 400, "Bad Request");
                break;
            }
            //如果是GET方法此时已解析完成 如果是POST方法 继续解析请求体
            if (method_ == METHOD_POST) {
                // POST方法
                process_state_ = STATE_RECV_BODY;
            } else {
                process_state_ = STATE_RESPONSE;
            }
        }

        //post方法
        if (process_state_ == STATE_RECV_BODY) {
            //读content_length字段 看请求体有多少字节的数据
            int content_length = -1;
            if (request_headers_.find("Content-length") != request_headers_.end()) {
                content_length = stoi(request_headers_["Content-length"]);
            } else {
                //标记为error的都是直接返回错误页面
                is_error_ = true;
                HandleError(connect_fd_, 400, "Bad Request: Lack of argument (Content-length)");
                break;
            }
            if (read_buffer_.size() < content_length) {
                break;
            }
            process_state_ = STATE_RESPONSE;
        }
        
        //构建响应报文并写入write_buffer
        if (process_state_ == STATE_RESPONSE) {
            ResponseState response_state = BuildResponse();
            if (response_state == RESPONSE_SUCCESS) {
                process_state_ = STATE_FINISH;
                break;
            } else {
                is_error_ = true;
                break;
            }
        }
    } while (false);

    if (!is_error_) {
        //如果成功解析 将响应报文数据发送给客户端
        if (write_buffer_.size() > 0) {
            HandleWrite();
        }
        if (!is_error_ && process_state_ == STATE_FINISH) {
            //完成后 reset状态置为初始化值
            Reset();
            //如果read_buffer还有数据 就继续读
            if (read_buffer_.size() > 0) {
                if (connection_state_ != DISCONNECTING) {
                    HandleRead();
                }
            }
        } else if (!is_error_ && connection_state_ != DISCONNECTED) {
            events |= EPOLLIN;
        }
    }
}

//处理写   向客户端发送write_buffer中的响应报文数据
void HttpConnection::HandleWrite() {
    //如果没有发生错误 并且连接没断开 就把写缓冲区的数据 发送给客户端
    if (!is_error_ && connection_state_ != DISCONNECTED) {
        int& events = channel_->events();
        if (utility::Write(connect_fd_, write_buffer_) < 0) {
            LOG(WARNING) << "Send response to client error, " << strerror(errno);
            events = 0;
            is_error_ = true;
        }
        //如果还没有写完 就等待下次写事件就绪接着写
        if (write_buffer_.size() > 0) {
            events |= EPOLLOUT;
        }
    }
}

//处理更新事件回调 
void HttpConnection::HandleUpdate() {
    //重新绑定定时器，相当于更新到期时间
    ResetTimer();
    int& events = channel_->events();

    if (!is_error_ && connection_state_ == CONNECTED) {
        //还在连接， 修改监听事件 绑定新定时器
        if (events != 0) {
            int timeout = kDefaultExpireTime;
            if (is_keep_alive_) {
                timeout = kDefaultKeepAliveTime;
            }
            if ((events & EPOLLIN) && (events & EPOLLOUT)) {
                events = 0;
                events |= EPOLLOUT;
            }

            events |= EPOLLET;
            event_loop_->PollerMod(channel_, timeout);
        } else if (is_keep_alive_) {
            events |= (EPOLLIN | EPOLLET);
            int timeout = kDefaultKeepAliveTime;
            event_loop_->PollerMod(channel_, timeout);
        } else {
            events |= (EPOLLIN | EPOLLET);
            int timeout = (kDefaultKeepAliveTime >> 1);
            event_loop_->PollerMod(channel_, timeout);
        }

    } else if (!is_error_ && connection_state_ == DISCONNECTING 
                    && (events & EPOLLOUT)) {
        //连接半关闭
        events = (EPOLLOUT | EPOLLET);
    } else {
        //连接已经关闭
        event_loop_->RunInLoop(std::bind(&HttpConnection::HandleClose, shared_from_this()));
    }
}

//处理错误（返回错误信息）
void HttpConnection::HandleError(int fd, int error_code, std::string error_message) {
    //写缓冲区
    char write_buffer[4096];
    error_message = " " + error_message;

    //响应体
    std::string response_body;
    response_body += "<html><title>请求出错了</title>";
    response_body += "<body bgcolor=\"ffffff\">";
    response_body += std::to_string(error_code) + error_message;
    response_body += "<hr><em> 阿神QAQ Web Server</em>\n</body></html>";

    //响应头
    std::string response_header;
    response_header += "HTTP/1.1 " + std::to_string(error_code) + error_message + "\r\n";
    response_header += "Content-Type: text/html\r\n";
    response_header += "Connection: Close\r\n";
    response_header += "Content-Length: " + std::to_string(response_body.size()) + "\r\n";
    response_header += "Server: 阿神QAQ Web Server\r\n";
    response_header += "\r\n";
    
    // 错误处理不考虑write不完的情况
    sprintf(write_buffer, "%s", response_header.c_str());
    utility::Write(fd, write_buffer, strlen(write_buffer));
    sprintf(write_buffer, "%s", response_body.c_str());
    utility::Write(fd, write_buffer, strlen(write_buffer));
}

//解析请求行
UriState HttpConnection::ParseUri() {
    std::string& request_data = read_buffer_;
    // 读到完整的请求行再开始解析请求
    size_t pos = request_data.find('\r', cur_read_pos_);
    if (pos < 0) {
        return PARSE_URI_AGAIN;
    }
    // 去掉请求行所占的空间，节省空间
    std::string request_line = request_data.substr(0, pos);
    if (request_data.size() > pos + 1) {
        request_data = request_data.substr(pos + 1);
    } else {
        request_data.clear();
    }
    // 请求方法
    int get_pos = request_line.find("GET");
    int post_pos = request_line.find("POST");
    int head_pos = request_line.find("HEAD");

    if (get_pos >= 0) {
        pos = get_pos;
        method_ = METHOD_GET;
    } else if (post_pos >= 0) {
        pos = post_pos;
        method_ = METHOD_POST;
    } else if (head_pos >= 0) {
        pos = head_pos;
        method_ = METHOD_HEAD;
    } else {
        return PARSE_URI_ERROR;
    }

    // 请求文件名
    pos = request_line.find("/", pos);
    if (pos < 0) {
        //没找到 就默认访问主页，http版本默认1.1
        file_name_ = "index.html";
        version_ = HTTP_11;
        return PARSE_URI_SUCCESS;
    } else {
        //找到了文件名 再找空格
        size_t pos2 = request_line.find(' ', pos);
        if (pos2 < 0) {
            return PARSE_URI_ERROR;
        } else {
            //找到了空格 
            if (pos2 - pos > 1) {
                file_name_ = request_line.substr(pos + 1, pos2 - pos - 1);
                size_t pos3 = file_name_.find('?');
                if (pos3 >= 0) {
                    file_name_ = file_name_.substr(0, pos3);
                }
            } else {
                file_name_ = "index.html";
            }
        }
        pos = pos2;
    }
    
    // HTTP 版本号
    pos = request_line.find("/", pos);
    if (pos < 0) {
        return PARSE_URI_ERROR;
    } else {
        if (request_line.size() - pos <= 3) {
            return PARSE_URI_ERROR;
        } else {
            std::string version = request_line.substr(pos + 1, 3);
            if (version == "1.0") {
                version_ = HTTP_10;
            } else if (version == "1.1") {
                version_ = HTTP_11;
            } else {
                return PARSE_URI_ERROR;
            }
        }
    }

    return PARSE_URI_SUCCESS;
}

//解析请求头
HeaderState HttpConnection::ParseHeaders() {
    std::string& request_data = read_buffer_;
    int key_start = -1;
    int key_end = -1;
    int value_start = -1;
    int value_end = -1;
    int now_read_line_begin = 0;
    bool is_finish = false;
    size_t i = 0;

    for (; i < request_data.size() && !is_finish; ++i) {
        switch (parse_state_) {
            case START: {
                if (request_data[i] == '\n' || request_data[i] == '\r') {
                    break;
                }
                parse_state_ = KEY;
                key_start = i;
                now_read_line_begin = i;
                break;
            }
            case KEY: {
                if (request_data[i] == ':') {
                    key_end = i;
                    if (key_end - key_start <= 0) {
                        return PARSE_HEADER_ERROR;
                    }
                    parse_state_ = COLON;
                } else if (request_data[i] == '\n' || request_data[i] == '\r') {
                    return PARSE_HEADER_ERROR;
                }
                break;
            }
            case COLON: {
                if (request_data[i] == ' ') {
                    parse_state_ = SPACES_AFTER_COLON;
                } else {
                    return PARSE_HEADER_ERROR;
                }
                break;
            }
            case SPACES_AFTER_COLON: {
                parse_state_ = VALUE;
                value_start = i;
                break;
            }
            case VALUE: {
                if (request_data[i] == '\r') {
                    parse_state_ = CR;
                    value_end = i;
                    if (value_end - value_start <= 0)
                        return PARSE_HEADER_ERROR;
                } else if (i - value_start > 255)
                    return PARSE_HEADER_ERROR;
                break;
            }
            case CR: {
                if (request_data[i] == '\n') {
                    parse_state_ = LF;
                    std::string key(request_data.begin() + key_start, request_data.begin() + key_end);
                    std::string value(request_data.begin() + value_start, request_data.begin() + value_end);
                    request_headers_[key] = value;
                    now_read_line_begin = i;
                } else {
                    return PARSE_HEADER_ERROR;
                }
                break;
            }
            case LF: {
                if (request_data[i] == '\r') {
                    parse_state_ = END_CR;
                } else {
                    key_start = i;
                    parse_state_ = KEY;
                }
                break;
            }
            case END_CR: {
                if (request_data[i] == '\n') {
                    parse_state_ = END_LF;
                } else {
                    return PARSE_HEADER_ERROR;
                }
                break;
            }
            case END_LF: {
                is_finish = true;
                key_start = i;
                now_read_line_begin = i;
                break;
            }
        }
    }

    if (parse_state_ == END_LF) {
        request_data = request_data.substr(i);
        return PARSE_HEADER_SUCCESS;
    }
    request_data = request_data.substr(now_read_line_begin);
    
    return PARSE_HEADER_AGAIN;
}

//构建响应报文并写入write_buffer
ResponseState HttpConnection::BuildResponse() {
    if (method_ == METHOD_POST) {
    } else if (method_ == METHOD_GET || method_ == METHOD_HEAD) {
        std::string response_header;
        response_header += "HTTP/1.1 200 OK\r\n";
        //如果在请求头中找到Connection字段 并且为keep-alive
        if (request_headers_.find("Connection") != request_headers_.end() 
                && (request_headers_["Connection"] == "Keep-Alive" 
                || request_headers_["Connection"] == "keep-alive")) {
            is_keep_alive_ = true;
            response_header += std::string("Connection: Keep-Alive\r\n") +
                                           "Keep-Alive: timeout=" + 
                                            std::to_string(kDefaultKeepAliveTime) +
                                            "\r\n";
        }

        //根据文件类型 来设置mime类型
        int file_type_pos = file_name_.find('.');
        std::string file_type;
        if (file_type_pos < 0) {
            file_type = MimeType::get_mime("default");
        } else {
            file_type = MimeType::get_mime(file_name_.substr(file_type_pos));
        }

        // echo test
        if (file_name_ == "echo") {
            write_buffer_ = "HTTP/1.1 200 OK\r\nContent-type: text/plain\r\n\r\nHello World";
            return RESPONSE_SUCCESS;
        }
        // 如果是请求图标 把请求头+图标数据(请求体)写入write_buffer
        if (file_name_ == "web_server_favicon.ico") {
            response_header += "Content-Type: image/png\r\n";
            response_header += "Content-Length: " + std::to_string(sizeof(web_server_favicon)) + "\r\n";
            response_header += "Server: 阿神QAQ Web Server\r\n";
            response_header += "\r\n";
            write_buffer_ += response_header;
            write_buffer_ += std::string(web_server_favicon, web_server_favicon + sizeof(web_server_favicon));
            return RESPONSE_SUCCESS;
        }

        //查看请求的文件权限
        struct stat file_stat;
        //请求的文件没有权限返回403
        if (stat(file_name_.c_str(), &file_stat) < 0) {
            response_header.clear();
            HandleError(connect_fd_, 403, "Forbidden!");
            return RESPONSE_ERROR;
        }
        response_header += "Content-Type: " + file_type + "\r\n";
        response_header += "Content-Length: " + std::to_string(file_stat.st_size) + "\r\n";
        response_header += "Server: 阿神QAQ Web Server\r\n";
        // 请求头后的空行
        response_header += "\r\n";
        write_buffer_ += response_header;

        //请求HEAD方法 不必返回响应体数据 此时就已经完成 
        if (method_ == METHOD_HEAD) {
            return RESPONSE_SUCCESS;
        }

        //请求的文件不存在返回404
        int file_fd = open(file_name_.c_str(), O_RDONLY, 0);
        if (file_fd < 0) {
            write_buffer_.clear();
            HandleError(connect_fd_, 404, "Not Found!");
            return RESPONSE_ERROR;
        }
        
        //将文件内容通过mmap映射到一块共享内存中
        void* mmap_address = mmap(NULL, file_stat.st_size, PROT_READ, 
                                  MAP_PRIVATE, file_fd, 0);
        close(file_fd);
        //映射共享内存失败 也返回404
        if (mmap_address == (void*)-1) {
            munmap(mmap_address, file_stat.st_size);
            write_buffer_.clear();
            HandleError(connect_fd_, 404, "Not Found!");
            return RESPONSE_ERROR;
        }
        
        //将共享内存里的内容 写入write_buffer
        char* filer_address = static_cast<char*>(mmap_address);
        write_buffer_ += std::string(filer_address, filer_address + file_stat.st_size);
        //关闭映射
        munmap(mmap_address, file_stat.st_size);
        return RESPONSE_SUCCESS;
    }

    return RESPONSE_ERROR;
}

// 重置HTTP状态 
void HttpConnection::Reset() {
    file_name_.clear();
    path_.clear();
    cur_read_pos_ = 0;
    process_state_ = STATE_PARSE_URI;
    parse_state_ = START;
    request_headers_.clear();
    
    //删除定时器
    if (timer_.lock()) {
        std::shared_ptr<timer::Timer> timer(timer_.lock());
        timer->Release();
        timer_.reset();
    }
}

//重新加入定时器
void HttpConnection::ResetTimer() {
    if (timer_.lock()) {
        std::shared_ptr<timer::Timer> timer(timer_.lock());
        timer->Release();
        timer_.reset();
    }
}

}  // namespace http
