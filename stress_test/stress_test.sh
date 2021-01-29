#服务器地址
server_ip=192.168.110.19
server_port=8888
url=http://${server_ip}:${server_port}/

#子进程数量
process_num=10000
#请求时间(单位s)
request_time=30
#keep-alive
is_keep_alive=0

#编译
make clean && make -j8

#删除之前开的进程
kill -9 `ps -ef | grep web_bench | awk '{print $2}'`

#运行
if [ $is_keep_alive -eq 1 ]
then
    ./web_bench -k -c $process_num -t $request_time $url
else
    ./web_bench -c $process_num -t $request_time $url
fi 


