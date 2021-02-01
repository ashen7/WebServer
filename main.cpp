#include <getopt.h>

#include "server/web_server.h"
#include "event/event_loop.h"
#include "log/logging.h"

namespace configure {
//默认值
static int thread_num = 8;
static int port = 8888;
static std::string log_file_name = "./web_server.log";

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
                log_file_name = optarg;
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
    log::Logging::set_log_file_name(configure::log_file_name);
    //开启日志
    log::Logging::set_open_log(false);
    //设置日志输出标准错误流
    log::Logging::set_log_to_stderr(true);
    //设置日志输出颜色
    log::Logging::set_open_log_color(true);

    //主loop  初始化poller, 给event_fd注册到epoll中并注册其事件处理回调
    event::EventLoop main_loop;

    //创建监听套接字绑定服务器，监听端口，设置监听套接字为NIO，屏蔽管道信号
    server::WebServer::GetInstance()->Initialize(&main_loop, configure::thread_num, configure::port);
    
    //主loop创建事件循环线程池(子loop),每个线程都run起来（调用SubLoop::Loop）
    //给监听套接字设置监听事件，绑定事件处理回调，注册到主loop的epoll内核事件表中
    server::WebServer::GetInstance()->Start();
    
    // 主loop开始事件循环  epoll_wait阻塞 等待就绪事件(主loop只注册了监听套接字的fd，所以只会处理新连接事件)
    main_loop.Loop();
    
    return 0;
}
