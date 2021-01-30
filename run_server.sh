#!/bin/bash

#服务器绑定端口号
port=8888               
#是否打开日志
open_log=0     
#日志路径
log_path=./server.log
#日志输出到标准输出
log_to_stdout=1
#异步写入日志
async_write_log=0
#数据库连接池
connection_num=8
#线程池
thread_num=6

#Makefile
#make clean && make -j8

#cmake 判断文件夹是否存在
if [ ! -d "./build" ]
then
    mkdir build
fi

cd build
rm -rf * && cmake .. && make -j8 && cp web_server ../ 
cd ..

./web_server -p $port -t $thread_num -l $log_path 
