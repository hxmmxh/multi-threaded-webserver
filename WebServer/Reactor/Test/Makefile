.PHONY: clean 

target=EventLoopTest EventLoopThreadTest EventLoopThreadPoolTest

SRCH= EventLoop.h EventLoopThread.h EventLoopThreadPool.h
SRCS= $(patsubst %.h, %.cpp, $(SRCH))
THREAD=$(wildcard ../../../Thread/*.cpp)
THREAD+=$(wildcard ../../../Thread/*.h)
TIME=
TIMER=../../Timer/TimerQueue.cpp ../../Timer/TimerQueue.h

all : $(target)

$(target): $(SRCH) $(SRCS) $(THREAD) $(TIMER)

$(target):
	g++ -o $@ $(filter %.cpp, $^) -lpthread



EventLoopTest: EventLoopTest.cpp
EventLoopThreadTest: EventLoopThreadTest.cpp
EventLoopThreadPoolTest: EventLoopThreadPoolTest.cpp