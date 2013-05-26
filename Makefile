all: boxer.cpp boxer.h lamport.cpp lamport.h
	mpic++ boxer.cpp lamport.cpp

clean:
	rm a.out
