#define _XOPEN_SOURCE
#include <mpi.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "lamport.h"
#include "boxer.h"

#define MSG_REQUEST 1
#define MSG_RELEASE 2
#define MSG_REPLY 3

using namespace std;

struct MessageStruct {
    int timestamp;
    int ringId;
    ProcessType type;
};

int size, rank;
Lamport lamport;

void fight()
{
    printf("Boxer %d fighting\n", rank);
    sleep(1 + (random() % 5));
}

void rest()
{
    printf("Boxer %d resting\n", rank);
    sleep(1 + (random() % 5));
}

void acquire()
{
    printf("Boxer %d wants to acquire a ring\n", rank);

    QueueElement request(lamport.getTimestamp(), rank, BOXER);
    lamport.enqueue(request);

    // send request to every node
    for (int i = 0; i < size; i++) {
        if (i != rank) {
            lamport.increment();
            MessageStruct message;
            message.timestamp = lamport.getTimestamp();
            MPI_Send(&message, sizeof(message), MPI_BYTE,
                     i, MSG_REQUEST, MPI_COMM_WORLD);
        }
    }

    int nReplies = 0;
    while (!(nReplies == size - 1 && lamport.front().id == rank)) {
        // wait
        // receive msgs etc
        if (receive()) {
            nReplies++;
        }
    }

    // tell second boxer?

}

void release()
{
    printf("Boxer %d releasing ring\n", rank);

    // send release msg to every node
    for (int i = 0; i < size; i++) {
        if (i != rank) {
            lamport.increment();
            MessageStruct message;
            message.timestamp = lamport.getTimestamp();
            MPI_Send(&message, sizeof(message), MPI_BYTE,
                     i, MSG_RELEASE, MPI_COMM_WORLD);
        }
    }
}

bool receive()
{
    // update timestamp
    // if request -> enqueue and reply with timestamp
    // if release -> remove request from queue
    // if reply -> return true (calling function can count replies)

    lamport.increment();
    MessageStruct message;
    MPI_Status status;
    MPI_Recv(&message, sizeof(message), MPI_BYTE,
            MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    lamport.update(message.timestamp);
    int processId = status.MPI_SOURCE;

    if (status.MPI_TAG == MSG_REQUEST) {
        QueueElement request(message.timestamp, processId, message.type);
        lamport.enqueue(request);

        MessageStruct reply;
        reply.timestamp = lamport.getTimestamp();
        MPI_Send(&reply, sizeof(reply), MPI_BYTE,
                processId, MSG_REPLY, MPI_COMM_WORLD);
        return false;
    }

    if (status.MPI_TAG == MSG_RELEASE) {
        lamport.remove(processId);
    }

    if (status.MPI_TAG == MSG_REPLY) {
        return true;
    }

}

int main(int argc, char **argv)
{
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