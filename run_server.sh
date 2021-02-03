#!/bin/bash
build_dir=./build
flush_core_cmd=~/tools/flush_core.sh

#服务器绑定端口号
port=8888               

#日志路径
log_file_name=./server.log
#是否打开日志
open_log=0 
#日志也输出到标准错误流
log_to_stderr=0
#输出日志颜色
color_log_to_stderr=1
#打印的最小日志等级
min_log_level=1

#数据库连接池
connection_num=6

#线程池
thread_num=6

#Makefile
#make clean && make -j8

#cmake 判断文件夹是否存在
if [ ! -d $build_dir ]
then
    mkdir $build_dir 
fi

cd $build_dir
rm -rf * && cmake .. && make -j8 && make install 
cd ..
$flush_core_cmd
rm -f $log_file_name

export LD_LIBRARY_PATH=./lib:$LD_LIBRARY_PATH
./web_server -p $port -t $thread_num -f $log_file_name \
             -o $open_log -s $log_to_stderr \
             -c $color_log_to_stderr -l $min_log_level
