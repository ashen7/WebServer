# $^  代表所有依赖文件
# $@  代表所有目标文件
# $<  代表第一个依赖文件
# %   代表通配符

TARGET   := web_server
CXX      := g++
CXXFLAGS := -std=gnu++11 -Wfatal-errors -Wno-unused-parameter
INC_DIR  := -I ./include
LIB_DIR  := -L ./
LIBS     := -lpthread
DEBUG 	 := 0

ifeq ($(DEBUG), 1)
    CXXFLAGS += -g -DDEBUG
else
	CXXFLAGS += -O3 -DNDEBUG
endif

SOURCES := $(wildcard src/utility/*.cpp \
                      src/thread/*.cpp \
                      src/timer/*.cpp \
					  src/log/*.cpp \
                      src/http/*.cpp \
					  src/event/*.cpp \
					  src/server/*.cpp \
					  main.cpp)

web_server: 
	$(CXX) -o $(TARGET) $(SOURCES) $^ $(CXXFLAGS) $(INC_DIR) $(LIB_DIR) $(LIBS)

clean:
	rm -f $(TARGET)
