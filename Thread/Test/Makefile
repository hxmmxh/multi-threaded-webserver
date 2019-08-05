.PHONY: clean 

FILE= CurrentThread.o ThreadPool.o Thread.o 
HERADER=$(wildcard ../*.h)
SRCS=$(wildcard ../*.cpp)
TEST=$(wildcard *.cpp)
target=CurrentThreadTest ThreadPoolTest ThreadTest

all : $(target)

$(target) : $(HEADER) $(SRCS)

$(target) :  
	g++ -o $@ $(filter %.cpp,$^) -lpthread

CurrentThreadTest: CurrentThreadTest.cpp

ThreadPoolTest: ThreadPoolTest.cpp

ThreadTest: ThreadTest.cpp

clean:
	-rm $(target)

