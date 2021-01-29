#服务器绑定端口号
port=8888               
#是否打开日志
open_log=0     
#日志输出到标准输出
log_to_stdout=1
#异步写入日志
async_write_log=0
#epoll模式 0:listen为LT, conn为LT, 1:listen为LT, conn为ET, 2:listen为ET, conn为LT, 3:listen为ET, conn为ET
epoll_mode=2
#数据库连接池
connection_num=8
#线程池
thread_num=8
#并发模式 0:proactor 1:reactor
concurrent_model=1

make clean && make -j8
#./server -p $port -o $open_log -s $log_to_stdout \
#         -a $async_write_log -e $epoll_mode -c $connection_num \
#         -t $thread_num -m $concurrent_model
./web_server -p $port -t $thread_num 
