#ifndef UTILS_H_
#define UTILS_H_

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

//
void handle_for_sigpipe();

//
int InitListen(int port);

//读n个字节
int Read(int fd, void* buffer, int n);
int Read(int fd, std::string& buffer, bool& zero);
int Read(int fd, std::string& buffer);

//写n个字节
int Write(int fd, void* buffer, int n);
int Write(int fd, std::string& buffer);

#endif
