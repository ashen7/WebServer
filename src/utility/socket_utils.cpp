#include "utility/socket_utils.h"

#include <string.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>

namespace utility {
//最大buffer size
const int MAX_BUFFER_SIZE = 4096;

//设置非阻塞套接字
int SetSocketNonBlocking(int fd) {
    int old_flag = fcntl(fd, F_GETFL, 0);
    if (old_flag == -1) {
        return -1;
    }

    int new_flag = old_flag | O_NONBLOCK;
    if (fcntl(fd, F_SETFL, new_flag) == -1) {
        return -1;
    }

    return 0;
}

//设置tcp算法nodelay
void SetSocketNoDelay(int fd) {
    int enable = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void*)&enable, sizeof(enable));
}

//优雅关闭套接字
void SetSocketNoLinger(int fd) {
    struct linger linger_struct;
    linger_struct.l_onoff = 1;
    linger_struct.l_linger = 30;
    setsockopt(fd, SOL_SOCKET, SO_LINGER, (const char*)&linger_struct, sizeof(linger_struct));
}

//关闭套接字
void ShutDownWR(int fd) {
    shutdown(fd, SHUT_WR);
}

//处理管道信号
void HandlePipeSignal() {
    struct sigaction signal_action;
    memset(&signal_action, '\0', sizeof(signal_action));
    signal_action.sa_handler = SIG_IGN;
    signal_action.sa_flags = 0;
    if (sigaction(SIGPIPE, &signal_action, NULL)) {
        return;
    }
}

//服务器绑定地址并监听端口
int SocketListen(int port) {
    // 检查port值，取正确区间范围
    if (port < 0 || port > 65535) {
        exit(1);
    }

    // 创建socket(IPv4 + TCP)，返回监听描述符
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1) {
        exit(1);
    }

    // 消除bind时"Address already in use"错误
    int flag = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) == -1) {
        close(listen_fd);
        exit(1);
    }

    // 绑定服务器的ip和端口
    struct sockaddr_in server_addr;
    bzero((char*)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons((unsigned short)port);
    if (bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        close(listen_fd);
        exit(1);
    }

    // 开始监听端口，最大等待队列长为LISTENQ
    if (listen(listen_fd, 2048) == -1) {
        close(listen_fd);
        exit(1);
    }

    // 无效监听描述符
    if (listen_fd == -1) {
        close(listen_fd);
        exit(1);
    }

    return listen_fd;
}

//从fd中读n个字节到buffer
int Read(int fd, void* read_buffer, int size) {
    int read_bytes = 0;
    int read_sum_bytes = 0;
    char* buffer = (char*)read_buffer;

    while (size > 0) {
        if ((read_bytes = read(fd, buffer, size)) < 0) {
            //EINTR是中断引起的 所以重新读就行
            if (errno == EINTR) {
                read_bytes = 0;
            } else if (errno == EAGAIN) {
                return read_sum_bytes;
            } else {
                return -1;
            }
        } else if (read_bytes == 0) {
            break;
        }

        read_sum_bytes += read_bytes;
        size -= read_bytes;
        buffer += read_bytes;
    }

    return read_sum_bytes;
}

//从fd中读n个字节到buffer
int Read(int fd, std::string& read_buffer, bool& is_read_zero_bytes) {
    int read_bytes = 0;
    int read_sum_bytes = 0;

    while (true) {
        char buffer[MAX_BUFFER_SIZE];
        if ((read_bytes = read(fd, buffer, MAX_BUFFER_SIZE)) < 0) {
            //EINTR是中断引起的 所以重新读就行
            if (errno == EINTR) {
                continue;
            } else if (errno == EAGAIN) {
                return read_sum_bytes;
            } else {
                perror("read error");
                return -1;
            }
        } else if (read_bytes == 0) {
            is_read_zero_bytes = true;
            break;
        }
        
        read_sum_bytes += read_bytes;
        read_buffer += std::string(buffer, buffer + read_bytes);
    }

    return read_sum_bytes;
}

//从fd中读n个字节到buffer
int Read(int fd, std::string& read_buffer) {
    int read_bytes = 0;
    int read_sum_bytes = 0;

    while (true) {
        char buffer[MAX_BUFFER_SIZE];
        if ((read_bytes = read(fd, buffer, MAX_BUFFER_SIZE)) < 0) {
            //EINTR是中断引起的 所以重新读就行
            if (errno == EINTR) {
                continue;
            } else if (errno == EAGAIN) {
                return read_sum_bytes;
            } else {
                perror("read error");
                return -1;
            }
        } else if (read_bytes == 0) {
            break;
        }

        read_sum_bytes += read_bytes;
        read_buffer += std::string(buffer, buffer + read_bytes);
    }

    return read_sum_bytes;
}

//从buffer中写n个字节到fd
int Write(int fd, void* write_buffer, int size) {
    int write_bytes = 0;
    int write_sum_bytes = 0;
    char* buffer = (char*)write_buffer;

    while (size > 0) {
        if ((write_bytes = write(fd, buffer, size)) <= 0) {
            if (write_bytes < 0) {
               //EINTR是中断引起的 所以重新写就行
                if (errno == EINTR) {
                    write_bytes = 0;
                    continue;
                } else if (errno == EAGAIN) {
                    return write_sum_bytes;
                } else {
                    return -1;
                }
            }
        }

        write_sum_bytes += write_bytes;
        size -= write_bytes;
        buffer += write_bytes;
    }

    return write_sum_bytes;
}

//从buffer中写n个字节到fd
int Write(int fd, std::string& write_buffer) {
    int size = write_buffer.size();
    int write_bytes = 0;
    int write_sum_bytes = 0;
    const char* buffer = write_buffer.c_str();

    while (size > 0) {
        if ((write_bytes = write(fd, buffer, size)) <= 0) {
            //EINTR是中断引起的 所以重新写就行
            if (write_bytes < 0) {
                if (errno == EINTR) {
                    write_bytes = 0;
                    continue;
                } else if (errno == EAGAIN) {
                    break;
                } else {
                    return -1;
                }
            }
        }

        write_sum_bytes += write_bytes;
        size -= write_bytes;
        buffer += write_bytes;
    }

    if (write_sum_bytes == write_buffer.size()) {
        write_buffer.clear();
    } else {
        write_buffer = write_buffer.substr(write_sum_bytes);
    }

    return write_sum_bytes;
}

}  // namespace utility