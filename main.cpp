#include <getopt.h>

#include "web_server.h"
#include "event/event_loop.h"
#include "log/logging.h"

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
            default:
                break;
        }
    }
    Logging::set_log_filename(log_path);
    
    EventLoop main_loop;
    WebServer web_server(&main_loop, thread_num, port);
    web_server.Start();
    main_loop.Loop();
    
    return 0;
}
