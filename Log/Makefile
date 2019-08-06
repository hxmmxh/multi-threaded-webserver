CPPFLAGS= -pthread

HEADERS=$(wildcard *.h)
SRCS=$(wildcard *.cpp)
OBJS=$(SRCS:.cpp=.o)
OTHER=../Time/Time.a ../Thread/Thread.a

LIB=Log.a

TESTSRCS=$(wildcard Test/*.cpp)
TEST=$(TESTSRCS:.cpp=)

LOGFILE=$(wildcard Test/*.log)

supp:$(OTHER)
	ar -x $<


all: $(TEST)
lib: $(LIB)
obj: $(OBJS) supp

$(OBJS):$(HEADERS) 

$(LIB) : obj
	ar rcs $@ *.o

$(TEST):$(LIB) 


../Time/Time.a:
	cd ../Time;make

../Thread/Thread.a:
	cd ../Thread;make


clean: cleanTime cleanThread
	rm $(OBJS) $(TEST) $(LIB) $(LOGFILE)

cleanTime:
	cd ../Time;make clean

cleanThread:
	cd ../Thread;make clean
