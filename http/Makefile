CPPFLAGS= -pthread
CC = g++


HEADERS=$(wildcard *.h) 
SRCS=$(wildcard *.cpp) 
OBJS=$(SRCS:.cpp=.o)
LIB=HttpServer.a

TESTSRCS=$(wildcard Test/*.cpp)
TEST=$(TESTSRCS:.cpp=)

all: $(TEST)
lib: $(LIB)

$(OBJS):$(HEADERS)

$(LIB) : $(OBJS) ../WebServer/WebServer.a
	ar -x ../WebServer/WebServer.a;\
	ar rcs $@ *.o

$(TEST):$(LIB)  

../WebServer/WebServer.a:
	cd ../WebServer;make


clean:  cleanW
	-rm *.o  $(TEST) $(LIB)

cleanW:
	cd ../WebServer;make clean