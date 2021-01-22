#ifndef UTILS_H_
#define UTILS_H_

#include <cstdlib>
#include <string>

namespace utility {
//设置非阻塞套接字
int set_socket_nonblocking(int fd);
//设置tcp算法nodelay
void set_socket_nodelay(int fd);
//优雅关闭套接字
void set_socket_nolinger(int fd);
//
void shutdown(int fd);
//
int socket_bind_listen(int port);

void handle_for_sigpipe();

//读n个字节
int readn(int fd, void* buffer, int n);
int readn(int fd, std::string& buffer, bool& zero);
int readn(int fd, std::string& buffer);
//写n个字节
int writen(int fd, void* buffer, int n);
int writen(int fd, std::string& buffer);

}  // namespace utility

#endif
