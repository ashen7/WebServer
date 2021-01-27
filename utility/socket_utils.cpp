#include "socket_utils.h"

#include <string.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>

//最大buffer size
const int MAX_BUFFER_SIZE = 4096;

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

void SetSocketNoDelay(int fd) {
    int enable = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void*)&enable, sizeof(enable));
}

void SetSocketNoLinger(int fd) {
    struct linger linger_struct;
    linger_struct.l_onoff = 1;
    linger_struct.l_linger = 30;
    setsockopt(fd, SOL_SOCKET, SO_LINGER, (const char*)&linger_struct, sizeof(linger_struct));
}

void ShutDownWR(int fd) {
    shutdown(fd, SHUT_WR);
}

void HandlePipeSignal() {
    struct sigaction signal_action;
    memset(&signal_action, '\0', sizeof(signal_action));
    signal_action.sa_handler = SIG_IGN;
    signal_action.sa_flags = 0;
    if (sigaction(SIGPIPE, &signal_action, NULL)) {
        return;
    }
}

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

int Read(int fd, void* buffer, int n) {
    int nleft = n;
    int nread = 0;
    int read_sum = 0;
    char* ptr = (char*)buffer;
    while (nleft > 0) {
        if ((nread = read(fd, ptr, nleft)) < 0) {
            if (errno == EINTR)
                nread = 0;
            else if (errno == EAGAIN) {
                return read_sum;
            } else {
                return -1;
            }
        } else if (nread == 0) {
            break;
        }

        read_sum += nread;
        nleft -= nread;
        ptr += nread;
    }

    return read_sum;
}

int Read(int fd, std::string& buffer, bool& zero) {
    int nread = 0;
    int read_sum = 0;
    while (true) {
        char buffer[MAX_BUFFER_SIZE];
        if ((nread = read(fd, buffer, MAX_BUFFER_SIZE)) < 0) {
            if (errno == EINTR)
                continue;
            else if (errno == EAGAIN) {
                return read_sum;
            } else {
                perror("read error");
                return -1;
            }
        } else if (nread == 0) {
            // printf("redsum = %d\n", read_sum);
            zero = true;
            break;
        }
        // printf("before buffer.size() = %d\n", buffer.size());
        // printf("nread = %d\n", nread);
        read_sum += nread;
        // buffer += nread;
        buffer += std::string(buffer, buffer + nread);
        // printf("after buffer.size() = %d\n", buffer.size());
    }

    return read_sum;
}

int Read(int fd, std::string& buffer) {
    int nread = 0;
    int read_sum = 0;
    while (true) {
        char buffer[MAX_BUFFER_SIZE];
        if ((nread = read(fd, buffer, MAX_BUFFER_SIZE)) < 0) {
            if (errno == EINTR)
                continue;
            else if (errno == EAGAIN) {
                return read_sum;
            } else {
                perror("read error");
                return -1;
            }
        } else if (nread == 0) {
            // printf("redsum = %d\n", read_sum);
            break;
        }
        // printf("before buffer.size() = %d\n", buffer.size());
        // printf("nread = %d\n", nread);
        read_sum += nread;
        // buffer += nread;
        buffer += std::string(buffer, buffer + nread);
        // printf("after buffer.size() = %d\n", buffer.size());
    }

    return read_sum;
}

int Write(int fd, void* buffer, int n) {
    int nleft = n;
    int nwritten = 0;
    int write_sum = 0;
    char* ptr = (char*)buffer;
    while (nleft > 0) {
        if ((nwritten = write(fd, ptr, nleft)) <= 0) {
            if (nwritten < 0) {
                if (errno == EINTR) {
                    nwritten = 0;
                    continue;
                } else if (errno == EAGAIN) {
                    return write_sum;
                } else
                    return -1;
            }
        }
        write_sum += nwritten;
        nleft -= nwritten;
        ptr += nwritten;
    }

    return write_sum;
}

int Write(int fd, std::string& buffer) {
    int nleft = buffer.size();
    int nwritten = 0;
    int write_sum = 0;
    const char* ptr = buffer.c_str();
    while (nleft > 0) {
        if ((nwritten = write(fd, ptr, nleft)) <= 0) {
            if (nwritten < 0) {
                if (errno == EINTR) {
                    nwritten = 0;
                    continue;
                } else if (errno == EAGAIN) {
                    break;
                } else {
                    return -1;
                }
            }
        }
        write_sum += nwritten;
        nleft -= nwritten;
        ptr += nwritten;
    }

    if (write_sum == static_cast<int>(buffer.size())) {
        buffer.clear();
    } else {
        buffer = buffer.substr(write_sum);
    }

    return write_sum;
}
