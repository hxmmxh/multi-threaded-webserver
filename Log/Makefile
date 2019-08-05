target : Log.o

Log.o : $(wildcard *.cpp) $(wildcard *.h)
	g++ -c $(filter %.cpp,$^)