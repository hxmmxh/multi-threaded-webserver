CPPFLAGS= -pthread

HEADERS=$(wildcard *.h)
SRCS=$(wildcard *.cpp)
OBJS=$(SRCS:.cpp=.o)
LIB=Log.a

TESTSRCS=$(wildcard Test/*.cpp)
TEST=$(TESTSRCS:.cpp=)

LOGFILE=$(wildcard Test/*.log)
all: $(TEST)
lib: $(LIB)

$(OBJS):$(HEADERS) 

$(LIB) : $(OBJS)
	ar rcs $@ $^

$(TEST):$(LIB) ../Time/Time.a ../Thread/Thread.a

clean:
	rm $(OBJS) $(TEST) $(LIB) $(LOGFILE)
