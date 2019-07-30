.PHONY : obj cleano clean cleanlog

Log=LogFile.o Logging.o LogStream.o Timestamp.o CurrentThread.o Buffer.o ThreadPool.o

obj: LogStreamTest LogFileTest LoggingTest

LogStreamTest: LogStreamTest.o LogStream.o
	g++ -o LogStreamTest LogStream.o LogStreamTest.o
LogFileTest: LogFileTest.o $(Log)
	g++ -o LogFileTest LogFileTest.o  $(Log) -lpthread
LoggingTest: LoggingTest.o $(Log)
	g++ -o LoggingTest LoggingTest.o  $(Log) -lpthread
LogStreamTest.o : LogStreamTest.cpp LogStream.h
	g++ -c LogStreamTest.cpp
LogFileTest.o: LogFileTest.cpp
	g++ -c LogFileTest.cpp
LoggingTest.o: LoggingTest.cpp
	g++ -c LoggingTest.cpp
Logging.o:Logging.cpp Logging.h
	g++ -c Logging.cpp
LogStream.o: LogStream.cpp LogStream.h StringPiece.h
	g++ -c LogStream.cpp
LogFile.o: LogFile.cpp LogFile.h StringPiece.h
	g++ -c LogFile.cpp
Timestamp.o: ../Time/Timestamp.cpp ../Time/Timestamp.h
	 g++ -c ../Time/Timestamp.cpp	
CurrentThread.o: ../Thread/CurrentThread.cpp ../Thread/CurrentThread.h
	g++ -c ../Thread/CurrentThread.cpp
Buffer.o: ../Buffer/Buffer.cpp ../Buffer/Buffer.h
	g++ -c ../Buffer/Buffer.cpp
ThreadPool.o: ../Thread/ThreadPool.cpp ../Thread/ThreadPool.h
	g++ -c ../Thread/ThreadPool.cpp
clean: cleano cleanlog
	rm LogStreamTest LogFileTest LoggingTest
cleano:
	rm *.o
cleanlog:
	rm *.log
