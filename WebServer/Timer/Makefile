CPPFLAGS= -pthread

include ../Reactor/Makefile ../../Time/Makefile

HEADERS=$(wildcard *.h)
SRCS=$(wildcard *.cpp)
OBJS=$(SRCS:.cpp=.o)
LIB=Timer.a

TESTSRCS=$(wildcard Test/*.cpp)
TEST=$(TESTSRCS:.cpp=)

all: $(TEST)
lib: $(LIB)

$(OBJS):$(HEADERS)

$(LIB) : $(OBJS)
	ar rcs $@ $^

$(TEST):$(LIB) ../Reactor/Reactor.a ../../Time/Time.a ../../Log/Log.a

../../Time/Time.a:
	cd ../../Time;make

../Reactor/Reactor.a:
	cd ../Reactor;make	

../../Log/Log.a:
	cd ../../Log;make

clean: cleanR cleanT cleanL
	rm $(OBJS) $(TEST) $(LIB)

cleanR:
	cd ../Reactor;make clean

cleanT:
	cd ../../Time;make clean

cleanL:
	cd ../../Log;make clean


