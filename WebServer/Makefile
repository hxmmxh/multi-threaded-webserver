CPPFLAGS= -pthread

HEADERS=$(wildcard *.h) 
SRCS=$(wildcard *.cpp) 
OBJS=$(SRCS:.cpp=.o)
LIB=WebServer.a

TESTSRCS=$(wildcard Test/*.cpp)
TEST=$(TESTSRCS:.cpp=)

all: $(TEST)
lib: $(LIB)

$(OBJS):$(HEADERS)

$(LIB) : $(OBJS) Reactor/Reactor.a
	ar -x Reactor/Reactor.a;\
	ar rcs $@ *.o

$(TEST):$(LIB)  

Reactor/Reactor.a:
	cd Reactor;make


clean: cleanT\ cleanR
	-rm *.o  $(TEST) $(LIB)

cleanR:
	cd Reactor;make clean


