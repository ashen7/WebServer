#include "http.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <iostream>

#include "event/channel.h"
#include "event/event_loop.h"
#include "utility/utils.h"

//PTHREAD_ONCE_INIT初始化的变量保证pthread_once()函数只执行一次
pthread_once_t MimeType::once_control = PTHREAD_ONCE_INIT;
std::unordered_map<std::string, std::string> MimeType::mime_map;

const int DEFAULT_EVENT = EPOLLIN | EPOLLET | EPOLLONESHOT;
const int DEFAULT_EXPIRE_TIME = 2000;               // ms
const int DEFAULT_KEEP_ALIVE_TIME = 5 * 60 * 1000;  // ms

char favicon[555] = {
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

//HTML协议的文件传输类型
void MimeType::Init() {
    mime[".html"] = "text/html";
    mime[".avi"] = "video/x-msvideo";
    mime[".bmp"] = "image/bmp";
    mime[".c"] = "text/plain";
    mime[".doc"] = "application/msword";
    mime[".gif"] = "image/gif";
    mime[".gz"] = "application/x-gzip";
    mime[".htm"] = "text/html";
    mime[".ico"] = "image/x-icon";
    mime[".jpg"] = "image/jpeg";
    mime[".png"] = "image/png";
    mime[".txt"] = "text/plain";
    mime[".mp3"] = "audio/mp3";
    mime["default"] = "text/html";
}

std::string MimeType::get_mime(const std::string& type) {
    //只会执行一次的函数
    pthread_once(&once_control, MimeType::OnceInit);
    if (mime_map.find(type) == mime.end()) {
        return mime_map["default"];
    } else {
        return mime_map[type];
    }
}

//http类
Http::Http(EventLoop* event_loop, int connect_fd)
    : event_loop_(event_loop),
      channel_(new Channel(event_loop, connect_fd)),
      connect_fd_(connect_fd),
      cur_read_pos_(0),
      connection_state_(CONNECTED),
      process_state_(STATE_PARSE_URI),
      parse_state_(START),
      method_(METHOD_GET),
      version_(HTTP_11),
      is_error_(false),
      is_keep_alive_(false) {
    channel_->set_read_handler(std::bind(&Http::HandleRead, this));
    channel_->set_write_handler(std::bind(&Http::HandleWrite, this));
    channel_->set_connect_handler(std::bind(&Http::HandleConnect, this));
}

Http:~Http() {
    close(connect_fd_);
}

void Http::NewEvent() {
    channel_->set_events(DEFAULT_EVENT);
    event_loop_->PollerAdd(channel_, DEFAULT_EXPIRE_TIME);
}

void Http::HandleRead() {
    int& events_ = channel_->get_events();
    do {
        bool read_zero_bytes = false;
        int read_bytes = Read(connect_fd_, read_buffer_, is_zero);
        LOG << "Request: " << read_buffer_;
        if (connection_state_ == DISCONNECTING) {
            read_buffer_.clear();
            break;
        }
        if (read_bytes < 0) {
            perror("1");
            is_error_ = true;
            HandleError(connect_fd_, 400, "Bad Request");
            break;
        } else if (is_zero) {
            // 有请求出现但是读不到数据，可能是Request Aborted，或者来自网络的数据没有达到等原因
            // 最可能是对端已经关闭了，统一按照对端已经关闭处理
            connection_state_ = DISCONNECTING;
            if (read_bytes == 0) {
                // is_error_ = true;
                break;
            }
        }

        if (process_state_ == STATE_PARSE_URI) {
            URIState flag = this->ParseURI();
            if (flag == PARSE_URI_AGAIN) {
                break;
            } else if (flag == PARSE_URI_ERROR) {
                perror("2");
                LOG << "FD = " << connect_fd_ << "," << read_buffer_ << "******";
                read_buffer_.clear();
                is_error_ = true;
                HandleError(connect_fd_, 400, "Bad Request");
                break;
            } else {
                process_state_ = STATE_PARSE_HEADERS;
            }
        }

        if (process_state_ == STATE_PARSE_HEADERS) {
            HeaderState flag = this->parseHeaders();
            if (flag == PARSE_HEADER_AGAIN) {
                break;
            } else if (flag == PARSE_HEADER_ERROR) {
                perror("3");
                is_error_ = true;
                HandleError(connect_fd_, 400, "Bad Request");
                break;
            }
            if (method_ == METHOD_POST) {
                // POST方法准备
                process_state_ = STATE_RECV_BODY;
            } else {
                process_state_ = STATE_RESPONSE;
            }
        }

        if (process_state_ == STATE_RECV_BODY) {
            int content_length = -1;
            if (headers_.find("Content-length") != headers_.end()) {
                content_length = stoi(headers_["Content-length"]);
            } else {
                is_error_ = true;
                HandleError(connect_fd_, 400, "Bad Request: Lack of argument (Content-length)");
                break;
            }
            if (static_cast<int>(read_buffer_.size()) < content_length) {
                break;
            }
            process_state_ = STATE_RESPONSE;
        }

        if (process_state_ == STATE_RESPONSE) {
            ResponseState flag = this->analysisRequest();
            if (flag == RESPONSE_SUCCESS) {
                process_state_ = STATE_FINISH;
                break;
            } else {
                is_error_ = true;
                break;
            }
        }
    } while (false);

    if (!is_error_) {
        if (write_buffer_.size() > 0) {
            HandleWrite();
            // events_ |= EPOLLOUT;
        }
        // is_error_ may change
        if (!is_error_ && process_state_ == STATE_FINISH) {
            this->reset();
            if (read_buffer_.size() > 0) {
                if (connection_state_ != DISCONNECTING) {
                    HandleRead();
                }
            }
            // if ((is_keep_alive_ || read_buffer_.size() > 0) && connection_state_ ==
            // CONNECTED)
            // {
            //     this->reset();
            //     events_ |= EPOLLIN;
            // }
        } else if (!is_error_ && connection_state_ != DISCONNECTED) {
            events_ |= EPOLLIN;
        }
    }
}

void Http::HandleWrite() {
    if (!is_error_ && connection_state_ != DISCONNECTED) {
        int& events_ = channel_->get_events();
        if (Write(connect_fd_, write_buffer_) < 0) {
            perror("Write");
            events_ = 0;
            is_error_ = true;
        }
        if (write_buffer_.size() > 0) {
            events_ |= EPOLLOUT;
        }
    }
}

void Http::HandleConnect() {
    seperateTimer();
    int& events_ = channel_->get_events();
    if (!is_error_ && connection_state_ == CONNECTED) {
        if (events_ != 0) {
            int timeout = DEFAULT_EXPIRE_TIME;
            if (is_keep_alive_)
                timeout = DEFAULT_KEEP_ALIVE_TIME;
            if ((events_ & EPOLLIN) && (events_ & EPOLLOUT)) {
                events_ = int(0);
                events_ |= EPOLLOUT;
            }
            // events_ |= (EPOLLET | EPOLLONESHOT);
            events_ |= EPOLLET;
            event_loop_->updatePoller(channel_, timeout);

        } else if (is_keep_alive_) {
            events_ |= (EPOLLIN | EPOLLET);
            // events_ |= (EPOLLIN | EPOLLET | EPOLLONESHOT);
            int timeout = DEFAULT_KEEP_ALIVE_TIME;
            event_loop_->updatePoller(channel_, timeout);
        } else {
            // cout << "close normally" << endl;
            // event_loop_->shutdown(channel_);
            // event_loop_->RunInLoop(std::bind(&Http::HandleClose,
            // shared_from_this()));
            events_ |= (EPOLLIN | EPOLLET);
            // events_ |= (EPOLLIN | EPOLLET | EPOLLONESHOT);
            int timeout = (DEFAULT_KEEP_ALIVE_TIME >> 1);
            event_loop_->updatePoller(channel_, timeout);
        }
    } else if (!is_error_ && connection_state_ == DISCONNECTING &&
               (events_ & EPOLLOUT)) {
        events_ = (EPOLLOUT | EPOLLET);
    } else {
        // cout << "close with errors" << endl;
        event_loop_->RunInLoop(std::bind(&Http::HandleClose, shared_from_this()));
    }
}

void Http::HandleError(int fd, int err_num, string short_msg) {
    short_msg = " " + short_msg;
    char send_buff[4096];
    string body_buff, header_buff;
    body_buff += "<html><title>哎~出错了</title>";
    body_buff += "<body bgcolor=\"ffffff\">";
    body_buff += to_string(err_num) + short_msg;
    body_buff += "<hr><em> LinYa's Web Server</em>\n</body></html>";

    header_buff += "HTTP/1.1 " + to_string(err_num) + short_msg + "\r\n";
    header_buff += "Content-Type: text/html\r\n";
    header_buff += "Connection: Close\r\n";
    header_buff += "Content-Length: " + to_string(body_buff.size()) + "\r\n";
    header_buff += "Server: LinYa's Web Server\r\n";
    header_buff += "\r\n";
    
    // 错误处理不考虑writen不完的情况
    sprintf(send_buff, "%s", header_buff.c_str());
    Write(fd, send_buff, strlen(send_buff));
    sprintf(send_buff, "%s", body_buff.c_str());
    Write(fd, send_buff, strlen(send_buff));
}

void Http::HandleClose() {
    connection_state_ = DISCONNECTED;
    shared_ptr<Http> guard(shared_from_this());
    event_loop_->PollerDel(channel_);
}

UriState Http::ParseUri() {
    string& str = read_buffer_;
    string cop = str;
    // 读到完整的请求行再开始解析请求
    size_t pos = str.find('\r', cur_read_pos_);
    if (pos < 0) {
        return PARSE_URI_AGAIN;
    }
    // 去掉请求行所占的空间，节省空间
    string request_line = str.substr(0, pos);
    if (str.size() > pos + 1) {
        str = str.substr(pos + 1);
    } else {
        str.clear();
    }
    // Method
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

    // filename
    pos = request_line.find("/", pos);
    if (pos < 0) {
        file_name_ = "index.html";
        version_ = HTTP_11;
        return PARSE_URI_SUCCESS;
    } else {
        size_t _pos = request_line.find(' ', pos);
        if (_pos < 0) {
            return PARSE_URI_ERROR;
        } else {
            if (_pos - pos > 1) {
                file_name_ = request_line.substr(pos + 1, _pos - pos - 1);
                size_t __pos = file_name_.find('?');
                if (__pos >= 0) {
                    file_name_ = file_name_.substr(0, __pos);
                }
            } else {
                file_name_ = "index.html";
            }
        }
        pos = _pos;
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
            string ver = request_line.substr(pos + 1, 3);
            if (ver == "1.0") {
                version_ = HTTP_10;
            } else if (ver == "1.1") {
                version_ = HTTP_11;
            } else {
                return PARSE_URI_ERROR;
            }
        }
    }

    return PARSE_URI_SUCCESS;
}

HeaderState Http::ParseHeaders() {
    string& str = read_buffer_;
    int key_start = -1, key_end = -1, value_start = -1, value_end = -1;
    int now_read_line_begin = 0;
    bool is_finish = false;
    size_t i = 0;

    for (; i < str.size() && !is_finish; ++i) {
        switch (parse_state_) {
            case START: {
                if (str[i] == '\n' || str[i] == '\r') {
                    break;
                }
                parse_state_ = KEY;
                key_start = i;
                now_read_line_begin = i;
                break;
            }
            case KEY: {
                if (str[i] == ':') {
                    key_end = i;
                    if (key_end - key_start <= 0) {
                        return PARSE_HEADER_ERROR;
                    }
                    parse_state_ = COLON;
                } else if (str[i] == '\n' || str[i] == '\r') {
                    return PARSE_HEADER_ERROR;
                }
                break;
            }
            case COLON: {
                if (str[i] == ' ') {
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
                if (str[i] == '\r') {
                    parse_state_ = CR;
                    value_end = i;
                    if (value_end - value_start <= 0)
                        return PARSE_HEADER_ERROR;
                } else if (i - value_start > 255)
                    return PARSE_HEADER_ERROR;
                break;
            }
            case CR: {
                if (str[i] == '\n') {
                    parse_state_ = LF;
                    string key(str.begin() + key_start, str.begin() + key_end);
                    string value(str.begin() + value_start,
                                 str.begin() + value_end);
                    headers_[key] = value;
                    now_read_line_begin = i;
                } else {
                    return PARSE_HEADER_ERROR;
                }
                break;
            }
            case LF: {
                if (str[i] == '\r') {
                    parse_state_ = END_CR;
                } else {
                    key_start = i;
                    parse_state_ = KEY;
                }
                break;
            }
            case END_CR: {
                if (str[i] == '\n') {
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
        str = str.substr(i);
        return PARSE_HEADER_SUCCESS;
    }
    str = str.substr(now_read_line_begin);
    
    return PARSE_HEADER_AGAIN;
}

ResponseState Http::Response() {
    if (method_ == METHOD_POST) {
        string header;
        header += string("HTTP/1.1 200 OK\r\n");
        if (headers_.find("Connection") != headers_.end() 
                && headers_["Connection"] == "Keep-Alive") {
            is_keep_alive_ = true;
            header += std::string("Connection: Keep-Alive\r\n") + 
                                  "Keep-Alive: timeout=" + 
                                  std::to_string(DEFAULT_KEEP_ALIVE_TIME) + 
                                  "\r\n";
        }

        int length = stoi(headers_["Content-length"]);
        std::vector<char> data(read_buffer_.begin(), read_buffer_.begin() + length);
        cv::Mat src = cv::imdecode(data, CV_LOAD_IMAGE_ANYDEPTH | CV_LOAD_IMAGE_ANYCOLOR);
        //imwrite("receive.bmp", src);
        cv::Mat res = stitch(src);
        vector<uchar> data_encode;
        cv::imencode(".png", res, data_encode);
        header += string("Content-length: ") +
                  to_string(data_encode.size()) +
                  "\r\n\r\n";
        write_buffer_ += header + string(data_encode.begin(),
        data_encode.end()); read_buffer_ = read_buffer_.substr(length); 

        return RESPONSE_SUCCESS;
    } else if (method_ == METHOD_GET || method_ == METHOD_HEAD) {
        string header;
        header += "HTTP/1.1 200 OK\r\n";
        if (headers_.find("Connection") != headers_.end() 
                && (headers_["Connection"] == "Keep-Alive" 
                || headers_["Connection"] == "keep-alive")) {
            is_keep_alive_ = true;
            header += string("Connection: Keep-Alive\r\n") +
                             "Keep-Alive: timeout=" + 
                             to_string(DEFAULT_KEEP_ALIVE_TIME) +
                             "\r\n";
        }
        int dot_pos = file_name_.find('.');
        string file_type;
        if (dot_pos < 0) {
            file_type = MimeType::get_mime("default");
        } else {
            file_type = MimeType::get_mime(file_name_.substr(dot_pos));
        }

        // echo test
        if (file_name_ == "hello") {
            write_buffer_ = "HTTP/1.1 200 OK\r\nContent-type: text/plain\r\n\r\nHello World";
            return RESPONSE_SUCCESS;
        }
        if (file_name_ == "favicon.ico") {
            header += "Content-Type: image/png\r\n";
            header += "Content-Length: " + to_string(sizeof favicon) + "\r\n";
            header += "Server: LinYa's Web Server\r\n";

            header += "\r\n";
            write_buffer_ += header;
            write_buffer_ += string(favicon, favicon + sizeof favicon);
            return RESPONSE_SUCCESS;
        }

        struct stat sbuf;
        if (stat(file_name_.c_str(), &sbuf) < 0) {
            header.clear();
            HandleError(connect_fd_, 404, "Not Found!");
            return RESPONSE_ERROR;
        }
        header += "Content-Type: " + file_type + "\r\n";
        header += "Content-Length: " + to_string(sbuf.st_size) + "\r\n";
        header += "Server: LinYa's Web Server\r\n";
        // 头部结束
        header += "\r\n";
        write_buffer_ += header;

        if (method_ == METHOD_HEAD) {
            return RESPONSE_SUCCESS;
        }

        int src_fd = open(file_name_.c_str(), O_RDONLY, 0);
        if (src_fd < 0) {
            write_buffer_.clear();
            HandleError(connect_fd_, 404, "Not Found!");
            return RESPONSE_ERROR;
        }

        void* mmap_address = mmap(NULL, sbuf.st_size, PROT_READ, MAP_PRIVATE, src_fd, 0);
        close(src_fd);
        if (mmap_address == (void*)-1) {
            munmap(mmap_address, sbuf.st_size);
            write_buffer_.clear();
            HandleError(connect_fd_, 404, "Not Found!");
            return RESPONSE_ERROR;
        }
        char* src_addr = static_cast<char*>(mmap_address);
        write_buffer_ += string(src_addr, src_addr + sbuf.st_size);
        munmap(mmap_address, sbuf.st_size);
        return RESPONSE_SUCCESS;
    }

    return RESPONSE_ERROR;
}

void Http::Reset() {
    file_name_.clear();
    path_.clear();
    cur_read_pos_ = 0;
    process_state_ = STATE_PARSE_URI;
    parse_state_ = START;
    headers_.clear();
    if (timer_.lock()) {
        shared_ptr<Timer> my_timer(timer_.lock());
        my_timer->clearReq();
        timer_.reset();
    }
}

void Http::SeperateTimer() {
    if (timer_.lock()) {
        shared_ptr<Timer> timer(timer_.lock());
        timer->clearReq();
        timer_.reset();
    }
}