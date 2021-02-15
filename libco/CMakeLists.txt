project(libco)
cmake_minimum_required(VERSION 3.5.1)

# 用C和汇编
enable_language(C ASM)

#项目根目录
set(ROOT_DIR ${CMAKE_CURRENT_LIST_DIR})
#源码目录
set(SRC_DIR ${ROOT_DIR}/src)
#头文件路径
set(INC_DIR ${ROOT_DIR}/include)
#库文件路径
set(LIB_DIR /usr/lib/x86_64-linux-gnu)
#可执行文件安装路径
set(EXEC_INSTALL_DIR ${ROOT_DIR}/bin)
#库安装路径
set(LIB_INSTALL_DIR ${ROOT_DIR}/lib)

# C编译选项
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} -fno-strict-aliasing -g -O2 -Wfatal-errors -Wno-unused-parameter -export-dynamic -pipe -D_GNU_SOURCE -D_REENTRANT -m64)

#CPP源文件
set(LIBCO_SOURCES ${SRC_DIR}/co_epoll.cpp
                  ${SRC_DIR}/co_hook_sys_call.cpp
                  ${SRC_DIR}/co_routine.cpp
                  ${SRC_DIR}/coctx.cpp
                  ${SRC_DIR}/coctx_swap.S)
message(${LIBCO_SOURCES})

#头文件路径添加
include_directories(${INC_DIR})
#库文件路径添加
link_directories(${LIB_DIR})
#用到的库
set(LINK_LIBRARY pthread dl) 

#生成动态链接库
add_library(co_shared SHARED ${LIBCO_SOURCES})
#更改动态链接库输出名字
set_target_properties(co_shared PROPERTIES OUTPUT_NAME co)
#设置动态链接库版本号
set_target_properties(co_shared PROPERTIES VERSION 1.2 SOVERSION 1)
#链接库
target_link_libraries(co_shared ${LINK_LIBRARY})
    
#生成静态库
add_library(co_static STATIC ${LIBCO_SOURCES})
#更改静态库输出名字
set_target_properties(co_static PROPERTIES OUTPUT_NAME co)
#链接库
target_link_libraries(co_static ${LINK_LIBRARY})

# Macro宏 
macro(add_demo_target DEMO_TARGET)
    add_executable(${DEMO_TARGET} demo/${DEMO_TARGET}.cpp)
    target_link_libraries(${DEMO_TARGET} ${LINK_LIBRARY} co_static)
endmacro(add_demo_target)

add_demo_target(closure)
add_demo_target(cond)
add_demo_target(copystack)
add_demo_target(echocli)
add_demo_target(echosvr)
add_demo_target(poll)
add_demo_target(setenv)
add_demo_target(specific)
add_demo_target(thread)

#安装
install(TARGETS co_shared co_static DESTINATION ${LIB_INSTALL_DIR})
install(TARGETS closure   DESTINATION ${EXEC_INSTALL_DIR})
install(TARGETS cond      DESTINATION ${EXEC_INSTALL_DIR})
install(TARGETS copystack DESTINATION ${EXEC_INSTALL_DIR})
install(TARGETS echocli   DESTINATION ${EXEC_INSTALL_DIR})
install(TARGETS echosvr   DESTINATION ${EXEC_INSTALL_DIR})
install(TARGETS poll      DESTINATION ${EXEC_INSTALL_DIR})
install(TARGETS setenv    DESTINATION ${EXEC_INSTALL_DIR})
install(TARGETS specific  DESTINATION ${EXEC_INSTALL_DIR})
install(TARGETS thread    DESTINATION ${EXEC_INSTALL_DIR})
