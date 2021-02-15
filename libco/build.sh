#!/bin/bash

build_dir=./build

#cmake 判断文件夹是否存在
if [ ! -d $build_dir ]
then
    mkdir $build_dir 
else
    rm -r $build_dir
    mkdir $build_dir 
fi

cd $build_dir
cmake .. && make -j8 && make install 
cd ..

