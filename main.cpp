#include <getopt.h>

#include "server/web_server.h"
#include "event/event_loop.h"
// #include "log/logging.h"

namespace configure {
//默认值
static int thread_num = 8;
static int port = 80;
static std::string log_path = "./WebServer.log";

static void ParseArg(int argc, char* argv[]) {
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
}

}  // namespace configure

int main(int argc, char* argv[]) {
    //解析参数
    configure::ParseArg(argc, argv);
    //设置日志文件    
    // Logging::set_log_filename(configure::log_path);

    //主loop  初始化poller, event_fd，给event_fd注册到epoll中并注册其事件处理回调
    event::EventLoop main_loop;
    //创建监听套接字绑定服务器，监听端口，设置监听套接字为NIO，屏蔽管道信号
    server::WebServer::GetInstance()->Initialize(&main_loop, configure::thread_num, configure::port);
    //主loop创建事件循环线程池(子loop),每个线程都run起来（调用SubLoop::Loop）
    //给监听套接字设置监听事件，绑定事件处理回调，注册到epoll内核事件表中
    server::WebServer::GetInstance()->Start();
    // 主loop开始事件循环  epoll_wait阻塞 等待就绪事件 处理每个就绪事件 执行正在等待的函数 处理超时
    main_loop.Loop();
    
    return 0;
}
z