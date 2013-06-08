all: sprawozdanie.pdf

sprawozdanie.pdf: sprawozdanie.tex ../boxer.cpp ../boxer.h ../lamport.cpp ../lamport.h
	rubber --pdf sprawozdanie.tex

clean:
	rm sprawozdanie.pdf sprawozdanie.log sprawozdanie.aux
