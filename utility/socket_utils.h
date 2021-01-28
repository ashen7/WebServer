#ifndef SOCKET_UTILS_H_
#define SOCKET_UTILS_H_

#include <cstdlib>
#include <string>

//设置非阻塞套接字
int SetSocketNonBlocking(int fd);
//设置tcp算法nodelay
void SetSocketNoDelay(int fd);
//优雅关闭套接字
void SetSocketNoLinger(int fd);
//关闭套接字
void ShutDownWR(int fd);

//处理信号
void HandlePipeSignal();

//绑定地址并监听端口
int SocketListen(int port);

//读n个字节
int Read(int fd, void* read_buffer, int n);
int Read(int fd, std::string& read_buffer, bool& zero);
int Read(int fd, std::string& read_buffer);

//写n个字节
int Write(int fd, void* write_buffer, int n);
int Write(int fd, std::string& write_buffer);

#endif
