# $^  代表所有依赖文件
# $@  代表所有目标文件
# $<  代表第一个依赖文件
# %   代表通配符

TARGET      := web_server
STATIC_LIB  := libevent.a 
SHARED_LIB  := libevent.so 
EVENT_LIB	:= event

CXX      	:= g++
CXXFLAGS 	:= -std=gnu++11 -Wfatal-errors -Wno-unused-parameter
#LDFLAGS 	:= -Wl,-rpath=./lib -Wl,-Bstatic
LDFLAGS 	:= -Wl,-rpath=./lib -Wl,-Bdynamic
INC_DIR  	:= -I ./include
LIB_DIR  	:= -L ./
LIBS     	:= -lpthread

DEBUG 	 	:= 0
OPEN_LOGGING := 1

ifeq ($(DEBUG), 1)
	CXXFLAGS += -g -DDEBUG
else
	CXXFLAGS += -O3 -DNDEBUG
endif

ifeq ($(OPEN_LOGGING), 1)
	CXXFLAGS += -DOPEN_LOGGING
endif

all: $(STATIC_LIB) $(SHARED_LIB) $(TARGET) 

#源文件
SOURCES := $(wildcard src/utility/*.cpp \
                      src/thread/*.cpp \
                      src/timer/*.cpp \
					  src/log/*.cpp \
                      src/http/*.cpp \
					  src/event/*.cpp \
					  src/server/*.cpp)
#main文件
MAIN := main.cpp

#链接生成静态库(这只是将源码归档到一个库文件中，什么flag都不加) @执行shell
$(STATIC_LIB): $(SOURCES)
	@echo '正在生成目标静态库: $@'
	@echo 'gcc linker正在链接'
	ar -rcs -o $@ $^ 
	@echo '完成链接目标: $@'
	@echo ' '
	
#链接生成动态库
$(SHARED_LIB): $(SOURCES)
	@echo '正在生成目标动态库: $@'
	@echo 'gcc linker正在链接'
	$(CXX) -fPIC -shared -o $@ $^ $(CXXFLAGS) $(INC_DIR) $(LIB_DIR) $(LIBS)
	@echo '完成链接目标: $@'
	@echo ' '
	
$(TARGET): $(MAIN) $(SHARED_LIB) 
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS) $(INC_DIR) $(LIB_DIR) $(LIBS) 

#声明这是伪目标文件
.PHONY: all clean 

install: $(STATIC_LIB) $(SHARED_LIB)
	@if (test ! -d ./lib); then mkdir -p ./lib; fi
	@mv libevent.a ./lib
	@mv libevent.so ./lib

clean:
	rm -f $(TARGET) $(STATIC_LIB) $(SHARED_LIB)

