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

$(LIB) : $(OBJS) ../Time/Time.a ../Thread/Thread.a
	ar -x ../Time/Time.a;ar -x ../Thread/Thread.a;ar rcs $@ *.o

$(TEST):$(LIB) 


../Time/Time.a:
	cd ../Time;make

../Thread/Thread.a:
	cd ../Thread;make


clean: cleanTime cleanThread
	-rm *.o  $(TEST) $(LIB) $(LOGFILE)

cleanTime:
	cd ../Time;make clean

cleanThread:
	cd ../Thread;make clean
