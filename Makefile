all: boxer.cpp boxer.h lamport.cpp lamport.h
	mpic++ boxer.cpp lamport.cpp -o boxers.out

clean:
	rm -f *.out *.o *~
