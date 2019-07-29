obj:CurrentThreadTest ThreadPoolTest

CurrentThreadTest: CurrentThread.cpp CurrentThread.h CurrentThreadTest.cpp
	g++ -o CurrentThreadTest CurrentThread.cpp CurrentThreadTest.cpp -lpthread

ThreadPoolTest: ThreadPool.cpp ThreadPoolTest.cpp ThreadPool.h CurrentThread.h
	g++ -o ThreadPoolTest ThreadPool.cpp ThreadPoolTest.cpp CurrentThread.cpp -lpthread
