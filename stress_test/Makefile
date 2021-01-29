# $^  代表所有依赖文件
# $@  代表所有目标文件
# $<  代表第一个依赖文件
# %   代表通配符

CXX = g++
CXXFLAGS = -w
EXEC = web_bench
DEBUG = 0
SOURCES = web_bench.cpp 

ifeq ($(DEBUG), 1)
    CXXFLAGS += -g
else
	CXXFLAGS += -O3 
endif

web_server: 
	$(CXX) -o $(EXEC) $(SOURCES) $^ $(CXXFLAGS) 

clean:
	rm -f $(EXEC)
