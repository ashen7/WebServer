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

namespace http {
//PTHREAD_ONCE_INIT初始化的变量保证pthread_once()函数只执行一次
pthread_once_t MimeType::once_control = PTHREAD_ONCE_INIT;
std::unordered_map<std::string, std::string> MimeType::mime_map;

//web server的图标favicon
static char favicon[555] = {
    '\x89', 'P',    'N',    'G',    '\xD',  '\xA',  '\x1A', '\xA',  '\x0',
    '\x0',  '\x0',  '\xD',  'I',    'H',    'D',    'R',    '\x0',  '\x0',
    '\x0',  '\x10', '\x0',  '\x0',  '\x0',  '\x10', '\x8',  '\x6',  '\x0',
    '\x0',  '\x0',  '\x1F', '\xF3', '\xFF', 'a',    '\x0',  '\x0',  '\x0',
    '\x19', 't',    'E',    'X',    't',    'S',    'o',    'f',    't',
    'w',    'a',    'r',    'e',    '\x0',  'A',    'd',    'o',    'b',
    'e',    '\x20', 'I',    'm',    'a',    'g',    'e',    'R',    'e',
    'a',    'd',    'y',    'q',    '\xC9', 'e',    '\x3C', '\x0',  '\x0',
    '\x1',  '\xCD', 'I',    'D',    'A',    'T',    'x',    '\xDA', '\x94',
    '\x93', '9',    'H',    '\x3',  'A',    '\x14', '\x86', '\xFF', '\x5D',
    'b',    '\xA7', '\x4',  'R',    '\xC4', 'm',    '\x22', '\x1E', '\xA0',
    'F',    '\x24', '\x8',  '\x16', '\x16', 'v',    '\xA',  '6',    '\xBA',
    'J',    '\x9A', '\x80', '\x8',  'A',    '\xB4', 'q',    '\x85', 'X',
    '\x89', 'G',    '\xB0', 'I',    '\xA9', 'Q',    '\x24', '\xCD', '\xA6',
    '\x8',  '\xA4', 'H',    'c',    '\x91', 'B',    '\xB',  '\xAF', 'V',
    '\xC1', 'F',    '\xB4', '\x15', '\xCF', '\x22', 'X',    '\x98', '\xB',
    'T',    'H',    '\x8A', 'd',    '\x93', '\x8D', '\xFB', 'F',    'g',
    '\xC9', '\x1A', '\x14', '\x7D', '\xF0', 'f',    'v',    'f',    '\xDF',
    '\x7C', '\xEF', '\xE7', 'g',    'F',    '\xA8', '\xD5', 'j',    'H',
    '\x24', '\x12', '\x2A', '\x0',  '\x5',  '\xBF', 'G',    '\xD4', '\xEF',
    '\xF7', '\x2F', '6',    '\xEC', '\x12', '\x20', '\x1E', '\x8F', '\xD7',
    '\xAA', '\xD5', '\xEA', '\xAF', 'I',    '5',    'F',    '\xAA', 'T',
    '\x5F', '\x9F', '\x22', 'A',    '\x2A', '\x95', '\xA',  '\x83', '\xE5',
    'r',    '9',    'd',    '\xB3', 'Y',    '\x96', '\x99', 'L',    '\x6',
    '\xE9', 't',    '\x9A', '\x25', '\x85', '\x2C', '\xCB', 'T',    '\xA7',
    '\xC4', 'b',    '1',    '\xB5', '\x5E', '\x0',  '\x3',  'h',    '\x9A',
    '\xC6', '\x16', '\x82', '\x20', 'X',    'R',    '\x14', 'E',    '6',
    'S',    '\x94', '\xCB', 'e',    'x',    '\xBD', '\x5E', '\xAA', 'U',
    'T',    '\x23', 'L',    '\xC0', '\xE0', '\xE2', '\xC1', '\x8F', '\x0',
    '\x9E', '\xBC', '\x9',  'A',    '\x7C', '\x3E', '\x1F', '\x83', 'D',
    '\x22', '\x11', '\xD5', 'T',    '\x40', '\x3F', '8',    '\x80', 'w',
    '\xE5', '3',    '\x7',  '\xB8', '\x5C', '\x2E', 'H',    '\x92', '\x4',
    '\x87', '\xC3', '\x81', '\x40', '\x20', '\x40', 'g',    '\x98', '\xE9',
    '6',    '\x1A', '\xA6', 'g',    '\x15', '\x4',  '\xE3', '\xD7', '\xC8',
    '\xBD', '\x15', '\xE1', 'i',    '\xB7', 'C',    '\xAB', '\xEA', 'x',
    '\x2F', 'j',    'X',    '\x92', '\xBB', '\x18', '\x20', '\x9F', '\xCF',
    '3',    '\xC3', '\xB8', '\xE9', 'N',    '\xA7', '\xD3', 'l',    'J',
    '\x0',  'i',    '6',    '\x7C', '\x8E', '\xE1', '\xFE', 'V',    '\x84',
    '\xE7', '\x3C', '\x9F', 'r',    '\x2B', '\x3A', 'B',    '\x7B', '7',
    'f',    'w',    '\xAE', '\x8E', '\xE',  '\xF3', '\xBD', 'R',    '\xA9',
    'd',    '\x2',  'B',    '\xAF', '\x85', '2',    'f',    'F',    '\xBA',
    '\xC',  '\xD9', '\x9F', '\x1D', '\x9A', 'l',    '\x22', '\xE6', '\xC7',
    '\x3A', '\x2C', '\x80', '\xEF', '\xC1', '\x15', '\x90', '\x7',  '\x93',
    '\xA2', '\x28', '\xA0', 'S',    'j',    '\xB1', '\xB8', '\xDF', '\x29',
    '5',    'C',    '\xE',  '\x3F', 'X',    '\xFC', '\x98', '\xDA', 'y',
    'j',    'P',    '\x40', '\x0',  '\x87', '\xAE', '\x1B', '\x17', 'B',
    '\xB4', '\x3A', '\x3F', '\xBE', 'y',    '\xC7', '\xA',  '\x26', '\xB6',
    '\xEE', '\xD9', '\x9A', '\x60', '\x14', '\x93', '\xDB', '\x8F', '\xD',
    '\xA',  '\x2E', '\xE9', '\x23', '\x95', '\x29', 'X',    '\x0',  '\x27',
    '\xEB', 'n',    'V',    'p',    '\xBC', '\xD6', '\xCB', '\xD6', 'G',
    '\xAB', '\x3D', 'l',    '\x7D', '\xB8', '\xD2', '\xDD', '\xA0', '\x60',
    '\x83', '\xBA', '\xEF', '\x5F', '\xA4', '\xEA', '\xCC', '\x2',  'N',
    '\xAE', '\x5E', 'p',    '\x1A', '\xEC', '\xB3', '\x40', '9',    '\xAC',
    '\xFE', '\xF2', '\x91', '\x89', 'g',    '\x91', '\x85', '\x21', '\xA8',
    '\x87', '\xB7', 'X',    '\x7E', '\x7E', '\x85', '\xBB', '\xCD', 'N',
    'N',    'b',    't',    '\x40', '\xFA', '\x93', '\x89', '\xEC', '\x1E',
    '\xEC', '\x86', '\x2',  'H',    '\x26', '\x93', '\xD0', 'u',    '\x1D',
    '\x7F', '\x9',  '2',    '\x95', '\xBF', '\x1F', '\xDB', '\xD7', 'c',
    '\x8A', '\x1A', '\xF7', '\x5C', '\xC1', '\xFF', '\x22', 'J',    '\xC3',
    '\x87', '\x0',  '\x3',  '\x0',  'K',    '\xBB', '\xF8', '\xD6', '\x2A',
    'v',    '\x98', 'I',    '\x0',  '\x0',  '\x0',  '\x0',  'I',    'E',
    'N',    'D',    '\xAE', 'B',    '\x60', '\x82',
};

//HTML协议的文件类型
void MimeType::Init() {
    mime_map[".html"] = "text/html";
    mime_map[".avi"] = "video/x-msvideo";
    mime_map[".bmp"] = "image/bmp";
    mime_map[".c"] = "text/plain";
    mime_map[".doc"] = "application/msword";
    mime_map[".gif"] = "image/gif";
    mime_map[".gz"] = "application/x-gzip";
    mime_map[".htm"] = "text/html";
    mime_map[".ico"] = "image/x-icon";
    mime_map[".jpg"] = "image/jpeg";
    mime_map[".png"] = "image/png";
    mime_map[".txt"] = "text/plain";
    mime_map[".mp3"] = "audio/mp3";
    mime_map["default"] = "text/html";
}

//根据type得到mime文件类型
std::string MimeType::get_mime(const std::string& type) {
    //只会执行一次的函数
    pthread_once(&once_control, MimeType::Init);
    if (mime_map.find(type) == mime_map.end()) {
        return mime_map["default"];
    } else {
        return mime_map[type];
    }
}

//http类
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
    channel_->set_connect_handler(std::bind(&HttpConnection::HandleConnect, this));
}

HttpConnection::~HttpConnection() {
    close(connect_fd_);
}

//给fd注册默认事件
void HttpConnection::AddNewEvent() {
    channel_->set_events(DEFAULT_EVENT);
    event_loop_->PollerAdd(channel_, DEFAULT_EXPIRE_TIME);
}

//处理关闭 删除fd注册的事件
void HttpConnection::HandleClose() {
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
        // LOG << "Request: " << read_buffer_;
        if (connection_state_ == DISCONNECTING) {
            read_buffer_.clear();
            break;
        }
        if (read_bytes < 0) {
            perror("1");
            is_error_ = true;
            HandleError(connect_fd_, 400, "Bad Request");
            break;
        } else if (is_read_zero_bytes) {
            // 有请求出现但是读不到数据，可能是Request Aborted，或者来自网络的数据没有达到等原因
            // 最可能是对端已经关闭了，统一按照对端已经关闭处理
            connection_state_ = DISCONNECTING;
            if (read_bytes == 0) {
                // is_error_ = true;
                break;
            }
        }

        //解析请求行
        if (process_state_ == STATE_PARSE_URI) {
            UriState uri_state = ParseUri();
            if (uri_state == PARSE_URI_AGAIN) {
                break;
            } else if (uri_state == PARSE_URI_ERROR) {
                perror("2");
                // LOG << "FD = " << connect_fd_ << "," << read_buffer_ << "******";
                read_buffer_.clear();
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
                perror("3");
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
            ResponseState response_state = Response();
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
            if (read_buffer_.size() > 0) {
                if (connection_state_ != DISCONNECTING) {
                    HandleRead();
                }
            }
            // if ((is_keep_alive_ || read_buffer_.size() > 0) && connection_state_ ==
            // CONNECTED)
            // {
            //     this->Reset();
            //     events |= EPOLLIN;
            // }
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
            perror("Write");
            events = 0;
            is_error_ = true;
        }
        //如果还没有写完 就等待下次写事件就绪接着写
        if (write_buffer_.size() > 0) {
            events |= EPOLLOUT;
        }
    }
}

//处理连接
void HttpConnection::HandleConnect() {
    SeperateTimer();
    int& events = channel_->events();

    if (!is_error_ && connection_state_ == CONNECTED) {
        if (events != 0) {
            int timeout = DEFAULT_EXPIRE_TIME;
            if (is_keep_alive_) {
                timeout = DEFAULT_KEEP_ALIVE_TIME;
            }
            if ((events & EPOLLIN) && (events & EPOLLOUT)) {
                events = 0;
                events |= EPOLLOUT;
            }
            events |= EPOLLET;
            event_loop_->PollerMod(channel_, timeout);
        } else if (is_keep_alive_) {
            events |= (EPOLLIN | EPOLLET);
            int timeout = DEFAULT_KEEP_ALIVE_TIME;
            event_loop_->PollerMod(channel_, timeout);
        } else {
            events |= (EPOLLIN | EPOLLET);
            int timeout = (DEFAULT_KEEP_ALIVE_TIME >> 1);
            event_loop_->PollerMod(channel_, timeout);
        }
    } else if (!is_error_ 
                    && connection_state_ == DISCONNECTING 
                    && (events & EPOLLOUT)) {
        events = (EPOLLOUT | EPOLLET);
    } else {
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
    response_body += "<hr><em> AshenQAQ Web Server</em>\n</body></html>";

    //响应头
    std::string response_header;
    response_header += "HTTP/1.1 " + std::to_string(error_code) + error_message + "\r\n";
    response_header += "Content-Type: text/html\r\n";
    response_header += "Connection: Close\r\n";
    response_header += "Content-Length: " + std::to_string(response_body.size()) + "\r\n";
    response_header += "Server: AshenQAQ Web Server\r\n";
    response_header += "\r\n";
    
    // 错误处理不考虑write不完的情况
    sprintf(write_buffer, "%s", response_header.c_str());
    utility::Write(fd, write_buffer, strlen(write_buffer));
    sprintf(write_buffer, "%s", response_body.c_str());
    utility::Write(fd, write_buffer, strlen(write_buffer));
}

//解析请求行
HttpConnection::UriState HttpConnection::ParseUri() {
    std::string& request_data = read_buffer_;
    std::string cop = request_data;
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
                //
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
    
    // cout << "file_name_: " << file_name_ << endl;
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
HttpConnection::HeaderState HttpConnection::ParseHeaders() {
    std::string& request_data = read_buffer_;
    int key_start = -1, key_end = -1, value_start = -1, value_end = -1;
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
HttpConnection::ResponseState HttpConnection::Response() {
    if (method_ == METHOD_POST) {

    } else if (method_ == METHOD_GET || method_ == METHOD_HEAD) {
        std::string response_header;
        response_header += "HTTP/1.1 200 OK\r\n";
        if (request_headers_.find("Connection") != request_headers_.end() 
                && (request_headers_["Connection"] == "Keep-Alive" 
                || request_headers_["Connection"] == "keep-alive")) {
            is_keep_alive_ = true;
            response_header += std::string("Connection: Keep-Alive\r\n") +
                                           "Keep-Alive: timeout=" + 
                                            std::to_string(DEFAULT_KEEP_ALIVE_TIME) +
                                            "\r\n";
        }

        int file_type_pos = file_name_.find('.');
        std::string file_type;
        if (file_type_pos < 0) {
            file_type = MimeType::get_mime("default");
        } else {
            file_type = MimeType::get_mime(file_name_.substr(file_type_pos));
        }

        // echo test
        if (file_name_ == "hello") {
            write_buffer_ = "HTTP/1.1 200 OK\r\nContent-type: text/plain\r\n\r\nHello World";
            return RESPONSE_SUCCESS;
        }
        // 如果请求图标 把请求头+图标数据(请求体)写入write_buffer
        if (file_name_ == "favicon.ico") {
            response_header += "Content-Type: image/png\r\n";
            response_header += "Content-Length: " + std::to_string(sizeof(favicon)) + "\r\n";
            response_header += "Server: AshenQAQ Web Server\r\n";
            response_header += "\r\n";
            write_buffer_ += response_header;
            write_buffer_ += std::string(favicon, favicon + sizeof(favicon));
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
        response_header += "Server: AshenQAQ Web Server\r\n";
        // 请求头后的空行
        response_header += "\r\n";
        write_buffer_ += response_header;

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
        
        char* filer_address = static_cast<char*>(mmap_address);
        write_buffer_ += std::string(filer_address, filer_address + file_stat.st_size);
        munmap(mmap_address, file_stat.st_size);
        return RESPONSE_SUCCESS;
    }

    return RESPONSE_ERROR;
}

//reset状态
void HttpConnection::Reset() {
    file_name_.clear();
    path_.clear();
    cur_read_pos_ = 0;
    process_state_ = STATE_PARSE_URI;
    parse_state_ = START;
    request_headers_.clear();
    if (timer_.lock()) {
        std::shared_ptr<timer::Timer> timer(timer_.lock());
        timer->Clear();
        timer_.reset();
    }
}

//删除定时器
void HttpConnection::SeperateTimer() {
    if (timer_.lock()) {
        std::shared_ptr<timer::Timer> timer(timer_.lock());
        timer->Clear();
        timer_.reset();
    }
}

}  // namespace http
