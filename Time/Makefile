CPPFLAGS= -pthread

HEADERS=$(wildcard *.h)
SRCS=$(wildcard *.cpp)
OBJS=$(SRCS:.cpp=.o)
LIB=Time.a

TESTSRCS=$(wildcard Test/*.cpp)
TEST=$(TESTSRCS:.cpp=)

all: $(TEST)
lib: $(LIB)

$(OBJS):$(HEADERS)

$(LIB) : $(OBJS)
	ar rcs $@ $^

$(TEST):$(LIB)

clean:
	-rm $(OBJS) $(TEST) $(LIB)
