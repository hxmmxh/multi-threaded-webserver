CPPFLAGS= -pthread

HEADERS=$(wildcard *.h)
SRCS=$(wildcard *.cpp)
OBJS=$(SRCS:.cpp=.o)
LIB=Sockets.a

TESTSRCS=$(wildcard Test/*.cpp)
TEST=$(TESTSRCS:.cpp=)

all: $(TEST)
lib: $(LIB)

$(OBJS):$(HEADERS) 

$(LIB) : $(OBJS)
	ar rcs $@ $^

$(TEST):$(LIB) ../../Log/Log.a


../../Log/Log.a:
	cd ../../Log;make


clean: cleanLog
	rm $(OBJS) $(TEST) $(LIB) 

cleanLog:
	cd ../../Log;make clean

