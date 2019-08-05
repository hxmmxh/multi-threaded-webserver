
HEADERS=$(wildcard *.h)
SRCS=$(wildcard *.cpp)
OBJS=$(SRCS:.cpp=.o)
LIB=Thread.a

all: $(LIB)

$(OBJS):$(HEADERS)

$(LIB) : $(OBJS)
	ar rcs $@ $^

clean:
	rm $(OBJS)
