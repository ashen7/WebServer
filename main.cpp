#include <getopt.h>

#include "server/web_server.h"
#include "event/event_loop.h"
// #include "log/logging.h"

int main(int argc, char* argv[]) {
    int thread_num = 8;
    int port = 80;
    std::string log_path = "./WebServer.log";

    // parse args
    int opt;
    const char* str = "t:l:p:";
    while ((opt = getopt(argc, argv, str)) != -1) {
        switch (opt) {
            case 't': {
                thread_num = atoi(optarg);
                break;
            }
            case 'l': {
                log_path = optarg;
                if (log_path.size() < 2 || optarg[0] != '/') {
                    printf("log_path should start with \"/\"\n");
                    abort();
                }
                break;
            }
            case 'p': {
                port = atoi(optarg);
                break;
            }
            default: {
                break;
            }
        }
    }
    
    // Logging::set_log_filename(log_path);
    
    //主loop 
    event::EventLoop main_loop;
    //初始化
    //1. 创建一个事件循环线程池（每个线程都有一个EventLoop对象)
    //2. 创建监听套接字绑定服务器，监听端口，设置监听套接字为NIO，屏蔽管道信号
    server::WebServer::GetInstance()->Initialize(&main_loop, thread_num, port);
    //开始运行
    //1. 
    server::WebServer::GetInstance()->Start();
    // 主loop开始事件循环
    main_loop.Loop();
    
    return 0;
}
