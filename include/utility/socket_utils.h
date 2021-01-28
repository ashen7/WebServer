#ifndef UTILITY_SOCKET_UTILS_H_
#define UTILITY_SOCKET_UTILS_H_

#include <cstdlib>
#include <string>

namespace utility {

//设置非阻塞套接字
int SetSocketNonBlocking(int fd);
//设置tcp算法nodelay
void SetSocketNoDelay(int fd);
//优雅关闭套接字
void SetSocketNoLinger(int fd);
//关闭套接字
void ShutDownWR(int fd);
//处理管道信号
void HandlePipeSignal();
//服务器绑定地址并监听端口
int SocketListen(int port);

//从fd中读n个字节到buffer
int Read(int fd, void* read_buffer, int size);
int Read(int fd, std::string& read_buffer, bool& is_read_zero_bytes);
int Read(int fd, std::string& read_buffer);

//从buffer中写n个字节到fd
int Write(int fd, void* write_buffer, int size);
int Write(int fd, std::string& write_buffer);

}  // namespace utility

#endif  // UTILITY_SOCKET_UTILS_H_
