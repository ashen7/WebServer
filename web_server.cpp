#include "web_server.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <functional>

#include "utility/socket_utils.h"
#include "log/logging.h"

WebServer::WebServer(EventLoop* event_loop, int thread_num, int port)
    : event_loop_(event_loop), 
      thread_num_(thread_num),
      port_(port), 
      started_(false) {
    // new一个事件循环线程池 和用于接收的Channel
    event_loop_thread_pool_ = new EventLoopThreadPool(event_loop_, thread_num);
    accept_channel_ = new Channel(event_loop_);
    
    //绑定服务器ip和端口 监听端口
    listen_fd_ = SocketListen(port_); 
    accept_channel_->set_fd(listen_fd_);
    HandlePipeSignal();
    //设置NIO非阻塞套接字
    if (SetSocketNonBlocking(listen_fd_) < 0) {
        perror("set socket non block failed");
        abort();
    }
}

//开始
void WebServer::Start() {
    //开启event_loop线程池
    event_loop_thread_pool_->Start();
    //accept的管道
    accept_channel_->set_events(EPOLLIN | EPOLLET);
    accept_channel_->set_read_handler(bind(&WebServer::HandleNewConnect, this));
    accept_channel_->set_connect_handler(bind(&WebServer::HandelCurConnect, this));
    
    event_loop_->PollerAdd(accept_channel_, 0);
    started_ = true;
}

void WebServer::HandleNewConnect() {
    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(struct sockaddr_in));
    socklen_t client_addr_len = sizeof(client_addr);

    while (true) {
        int connect_fd = accept(listen_fd_, (struct sockaddr*)&client_addr, &client_addr_len);
        EventLoop* event_loop = event_loop_thread_pool_->getNextLoop();
        LOG << "New connection from " << inet_ntoa(client_addr.sin_addr) 
            << ":" << ntohs(client_addr.sin_port);
        if (connect_fd < 0) {
            LOG << "Accept failed!";
            break;
        }
        // 限制服务器的最大并发连接数
        if (connect_fd >= MAX_FD_NUM) {
            LOG << "Internal server busy!";
            close(connect_fd);
            break;
        }
        // 设为非阻塞模式
        if (SetSocketNonBlocking(connect_fd) < 0) {
            LOG << "set socket nonblock failed!";
            close(connect_fd);
            break;
        }
        //设置套接字
        SetSocketNodelay(connect_fd);
        // setSocketNoLinger(connect_fd);

        shared_ptr<Http> req_info(new Http(event_loop, connect_fd));
        req_info->get_channel()->set_holder(req_info);
        
        event_loop->QueueInLoop(std::bind(&Http::NewEvent, req_info));
    }

    accept_channel_->set_events(EPOLLIN | EPOLLET);
}

void WebServer::HandelCurConnect() {
        event_loop_->PollerMod(accept_channel_);
}