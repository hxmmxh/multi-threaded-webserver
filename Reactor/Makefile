CPPFLAGS= -pthread

HEADERS=$(wildcard *.h) 
SRCS=$(wildcard *.cpp) 
OBJS=$(SRCS:.cpp=.o)
LIB=Reactor.a

TESTSRCS=$(wildcard Test/*.cpp)
TEST=$(TESTSRCS:.cpp=)

all: $(TEST)
lib: $(LIB)

$(OBJS):$(HEADERS)

$(LIB) : $(OBJS) ../Timer/Timer.a ../../Log/Log.a ../Sockets/Sockets.a
	ar -x ../Timer/Timer.a;\
	ar -x ../../Log/Log.a;\
	ar -x ../Sockets/Sockets.a;\
	ar rcs $@ *.o

$(TEST):$(LIB)  

../Timer/Timer.a:
	cd ../Timer;make

../../Log/Log.a:
	cd ../../Log;make

../Sockets/Sockets.a:
	cd ../Sockets;make

clean: cleanT cleanL cleanS
	-rm *.o  $(TEST) $(LIB)

cleanT:
	cd ../Timer;make clean

cleanL:
	cd ../../Log;make clean

cleanS:
	cd ../Sockets;make clean
