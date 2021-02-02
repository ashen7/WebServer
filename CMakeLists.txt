project(WebServer LANGUAGES CXX)
cmake_minimum_required(VERSION 3.5.1)

set(DEBUG OFF)

#g++ 编译选项
if (${DEBUG} STREQUAL "ON")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -DOPEN_LOGGING -DDEBUG -O3 -g -Wfatal-errors -Wno-unused-parameter")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -DNDEBUG -O3 -Wfatal-errors -Wno-unused-parameter")
endif()

#项目根目录
set(ROOT_DIR .)
#源码目录
set(SRC_DIR ${ROOT_DIR}/src)
#头文件路径
set(INCLUDE_DIR ${ROOT_DIR}/include)
#库文件路径
set(LIB_DIR /usr/lib/x86_64-linux-gnu)

#CPP源文件
file(GLOB UTILITY_SRC_FILE  ${SRC_DIR}/utility/*.cpp)
file(GLOB THREAD_SRC_FILE   ${SRC_DIR}/thread/*.cpp)
file(GLOB TIMER_SRC_FILE    ${SRC_DIR}/timer/*.cpp)
file(GLOB LOG_SRC_FILE      ${SRC_DIR}/log/*.cpp)
file(GLOB EVENT_SRC_FILE    ${SRC_DIR}/event/*.cpp)
file(GLOB HTTP_SRC_FILE     ${SRC_DIR}/http/*.cpp)
file(GLOB SERVER_SRC_FILE   ${SRC_DIR}/server/*.cpp)
set(MAIN_SRC_FILE           ${ROOT_DIR}/main.cpp)

set(WEB_SERVER_SOURCES  ${UTILITY_SRC_FILE}
                        ${THREAD_SRC_FILE}
                        ${TIMER_SRC_FILE}
                        ${LOG_SRC_FILE} 
                        ${EVENT_SRC_FILE}
                        ${HTTP_SRC_FILE}
                        ${SERVER_SRC_FILE}
                        ${MAIN_SRC_FILE})

#头文件路径添加
include_directories(${INCLUDE_DIR})

#库文件路径添加
link_directories(${LIB_DIR})

#链接库
set(LINK_LIBRARY pthread) 

#添加可执行文件
add_executable(web_server ${WEB_SERVER_SOURCES})
#链接库
target_link_libraries(web_server ${LINK_LIBRARY})
