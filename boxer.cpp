#define _XOPEN_SOURCE
#include <mpi.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
using namespace std;

void fight()
{
    sleep(1 + (random() % 5));
}

void rest()
{
    sleep(1 + (random() % 5));
}

void acquire()
{
}

void release()
{
}

int main(int argc, char **argv)
{
    int size, rank;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    srand(time(NULL));

    while (1) {
        acquire();
        fight();
        release();
        rest();
    }

    MPI_Finalize();
    return 0;
}
