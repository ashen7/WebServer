#include "web_server.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <functional>

#include "utility/utils.h"
#include "log/logging.h"

WebServer::WebServer(EventLoop* loop, int thread_num, int port)
    : loop_(loop), thread_num_(thread_num),
      port_(port), started_(false) {
    event_loop_thread_pool_ = new EventLoopThreadPool(loop_, thread_num);
    accept_channel_ = new Channel(loop_);
    listenfd_ = SocketListen(port_); 
    accept_channel_->set_fd(listenfd_);
    HandleSignalPipe();
    if (SetSocketNonBlocking(listenfd_) < 0) {
        perror("set socket non block failed");
        abort();
    }
}

//开始
void WebServer::Start() {
    //开启event_loop线程池
    event_loop_thread_pool_->Start();
    //accept的管道
    // accept_channel_->set_events(EPOLLIN | EPOLLET | EPOLLONESHOT);
    accept_channel_->set_events(EPOLLIN | EPOLLET);
    accept_channel_->set_read_handler(bind(&WebServer::HandleNewConnect, this));
    accept_channel_->set_connect_handler(bind(&WebServer::handelCurConnect, this));
    loop_->AddToPoller(accept_channel_, 0);
    started_ = true;
}

void WebServer::HandleNewConnect() {
    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(struct sockaddr_in));
    socklen_t client_addr_len = sizeof(client_addr);
    int accept_fd = 0;
    while ((accept_fd = accept(listenfd_, (struct sockaddr*)&client_addr,
                               &client_addr_len)) > 0) {
        EventLoop* loop = event_loop_thread_pool_->getNextLoop();
        LOG << "New connection from " << inet_ntoa(client_addr.sin_addr) << ":"
            << ntohs(client_addr.sin_port);
        // cout << "new connection" << endl;
        // cout << inet_ntoa(client_addr.sin_addr) << endl;
        // cout << ntohs(client_addr.sin_port) << endl;
        /*
        // TCP的保活机制默认是关闭的
        int optval = 0;
        socklen_t len_optval = 4;
        getsockopt(accept_fd, SOL_SOCKET,  SO_KEEPALIVE, &optval, &len_optval);
        cout << "optval ==" << optval << endl;
        */
        // 限制服务器的最大并发连接数
        if (accept_fd >= MAXFDS) {
            close(accept_fd);
            continue;
        }
        // 设为非阻塞模式
        if (SetSocketNonBlocking(accept_fd) < 0) {
            LOG << "Set non block failed!";
            // perror("Set non block failed!");
            return;
        }

        SetSocketNodelay(accept_fd);
        // setSocketNoLinger(accept_fd);

        shared_ptr<HttpData> req_info(new HttpData(loop, accept_fd));
        req_info->getChannel()->setHolder(req_info);
        loop->queueInLoop(std::bind(&HttpData::newEvent, req_info));
    }
    accept_channel_->set_events(EPOLLIN | EPOLLET);
}