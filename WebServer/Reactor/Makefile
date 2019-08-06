CPPFLAGS= -pthread

HEADERS=$(wildcard *.h) $(wildcard Poller/*h)
SRCS=$(wildcard *.cpp) $(wildcard Poller/*cpp)
OBJS=$(SRCS:.cpp=.o)
LIB=Reactor.a

TESTSRCS=$(wildcard Test/*.cpp)
TEST=$(TESTSRCS:.cpp=)

OTHERH= ../../Thread/Thread.h ../../Thread/CurrentThread.h ../Timer/TimerQueue.h ../Callbacks.h ../Log/Logging.h \
		../Sockets/SocketsOps.h
all: $(TEST)
lib: $(LIB)

$(OBJS):$(HEADERS) $(OTHERH)

$(LIB) : $(OBJS)
	ar rcs $@ $^

$(TEST):$(LIB) ../../Thread/Thread.a ../Timer/Timer.a  ../../Log/Log.a ../Sockets/Sockets.a

clean:
	rm $(OBJS) $(TEST) $(LIB)
