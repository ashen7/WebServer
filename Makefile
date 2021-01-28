# $^  代表所有依赖文件
# $@  代表所有目标文件
# $<  代表第一个依赖文件
# %   代表通配符

TARGET     := server
CXX      := g++
CXXFLAGS := -std=gnu++11 -g -w -O3 
INCPATH  := -I ./include
LIBPATH  := -L ./
LIBS     := -lpthread

SOURCES := $(wildcard src/utility/*.cpp \
                      src/thread/thread.cpp \
                      src/timer/*.cpp \
                      src/http/*.cpp \
					  src/event/*.cpp \
					  *.cpp)

web_server: 
	$(CXX) -o $(TARGET) $(SOURCES) $^ $(CXXFLAGS) $(INCPATH) $(LIBPATH) $(LIBS)

clean:
	rm -f $(TARGET)
