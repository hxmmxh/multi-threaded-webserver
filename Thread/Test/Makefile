.PHONY : obj cleano clean 

vpath %.h ../
vpath %.c ../

FILE= CurrentThread.o ThreadPool.o Thread.o 

obj:CurrentThreadTest ThreadPoolTest ThreadTest

CurrentThreadTest: $(FILE) CurrentThreadTest.o
	g++ -o CurrentThreadTest CurrentThreadTest.o $(FILE) -lpthread

ThreadPoolTest: $(FILE) ThreadPoolTest.o
	g++ -o ThreadPoolTest ThreadPoolTest.o $(FILE) -lpthread

ThreadTest : $(FILE) ThreadTest.obj
	g++ -o ThreadTest ThreadTest.o $(FILE) -lpthread

CurrentThreadTest.o :CurrentThreadTest.cpp
	g++ -c CurrentThreadTest.cpp

ThreadPoolTest.o : ThreadPoolTest.cpp
	g++ -c ThreadPoolTest.cpp

ThreadTest.o : ThreadTest.cpp
	g++ -c ThreadTest.cpp

Thread.o : Thread.cpp Thread.h CurrentThread.h CountDownLatch.h
	g++ -c Thread.cpp

ThreadPool.o : ThreadPool.cpp ThreadPool.h
	g++ -c ThreadPool.cpp

CurrentThread.o : CurrentThread.cpp CurrentThread.h
	g++ -c CurrentThread.cpp

clean: cleano 
	-rm CurrentThreadTest ThreadPoolTest ThreadTest
cleano:
	-rm *.o