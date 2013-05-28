#define _XOPEN_SOURCE
#include <mpi.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "lamport.h"
#include "boxer.h"

#define NRINGS 3

enum MsgTag { MSG_REQUEST, MSG_RELEASE, MSG_REPLY, MSG_OPPONENT, MSG_NOTIFY };

using namespace std;

int size, rank;
int opponent = -1;
int myRing;

Lamport lamport;
int nEmptyRings = NRINGS;
bool ringTaken[NRINGS] = {false};

void fight()
{
    printf(">>>>>>>>>> Boxer %d fighting with %d on ring %d\n", rank, opponent, myRing);
    sleep(3 + (random() % 5));
}

void clean()
{
    printf(">>>>>>>>>> Cleaner %d cleaning ring %d\n", rank, myRing);
    sleep(3 + (random() % 5));
}

void rest()
{
    printf("Boxer %d resting\n", rank);
    sleep(3 + (random() % 5));
}

void cleanerRest()
{
    printf("Cleaner %d resting\n", rank);
    sleep(5 + (random() % 5));
}

void request(ProcessType type)
{
    lamport.increment();
    QueueElement req(lamport.getTimestamp(), rank, type);
    lamport.enqueue(req);

    // send request to every node
    for (int i = 0; i < size; i++) {
        if (i != rank) {
            MessageStruct message;
            message.type = type;
            message.timestamp = lamport.getTimestamp();
            MPI_Send(&message, sizeof(message), MPI_BYTE,
                     i, MSG_REQUEST, MPI_COMM_WORLD);
        }
    }
}

void notifyOpponent()
{
    printf("Boxer %d, opponent: %d\n", rank, opponent);

    lamport.increment();
    MessageStruct message;
    message.timestamp = lamport.getTimestamp();
    message.ringId = myRing;
    MPI_Send(&message, sizeof(message), MPI_BYTE,
             opponent, MSG_OPPONENT, MPI_COMM_WORLD);
    printf("Boxer %d notifying opponent: %d\n", rank, lamport.second());
}

void notifyOthers()
{
    lamport.increment();
    for (int i = 0; i < size; i++) {
        if (i != rank) {
            MessageStruct message;
            message.ringId = myRing;
            message.timestamp = lamport.getTimestamp();
            MPI_Send(&message, sizeof(message), MPI_BYTE,
                     i, MSG_NOTIFY, MPI_COMM_WORLD);
        }
    }
}

int takeRing()
{
    for (int i = 0; i < NRINGS; i++) {
        if (!ringTaken[i]) {
            ringTaken[i] = true;
            return i;
        }
    }
    return -1;
}

void acquire()
{
    printf("Boxer %d wants to acquire a ring\n", rank);

    request(BOXER);

    int nReplies = 0;
    while ( !(nReplies == size - 1 &&
              lamport.isFirst(rank) &&
              lamport.isSecondBoxer() &&
              nEmptyRings > 0) ) {
        // wait
        // receive msgs etc
        int messageTag = receive();
        printf("messageTag: %d\n", messageTag);
        if (messageTag == MSG_REPLY) {
            nReplies++;
        } else if (messageTag == MSG_OPPONENT) {
            return;
        }
        printf("Boxer %d queue front: %d, timestamp: %d\n",
                rank, lamport.front().id, lamport.front().timestamp);
    }

    myRing = takeRing();
    opponent = lamport.second().id;
    notifyOpponent();
    notifyOthers();

    printf("Boxer %d acquired ring %d\n", rank, myRing);
}

void cleanerAcquire()
{
    printf("Cleaner %d wants to acquire a ring\n", rank);

    request(CLEANER);

    int nReplies = 0;
    while ( !(nReplies == size - 1 &&
              (lamport.isFirst(rank) || lamport.isSecond(rank)) &&
              nEmptyRings > 0) ) {
        // wait
        // receive msgs etc
        int messageTag = receive();
        printf("messageTag: %d\n", messageTag);
        if (messageTag == MSG_REPLY) {
            nReplies++;
        }
        printf("Cleaner %d queue front: %d, timestamp: %d\n", rank, lamport.front().id, lamport.front().timestamp);
    }

    myRing = takeRing();
    notifyOthers();

    printf("Cleaner %d acquired ring\n", rank);

}

void release()
{
    printf("Process %d releasing ring\n", rank);

    ringTaken[myRing] = false;
    nEmptyRings++;

    // send release msg to every node
    lamport.increment();
    for (int i = 0; i < size; i++) {
        if (i != rank) {
            MessageStruct message;
            message.timestamp = lamport.getTimestamp();
            message.ringId = myRing;
            MPI_Send(&message, sizeof(message), MPI_BYTE,
                     i, MSG_RELEASE, MPI_COMM_WORLD);
        }
    }

    printf("Process %d released ring %d\n", rank, myRing);
}

int receive()
{
    // update timestamp
    // if request -> enqueue and reply with timestamp
    // if release -> remove request from queue
    // if reply -> just return message type (calling function can count replies)

    MessageStruct message;
    MPI_Status status;
    MPI_Recv(&message, sizeof(message), MPI_BYTE,
            MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    lamport.update(message.timestamp);
    int processId = status.MPI_SOURCE;

    if (status.MPI_TAG == MSG_REQUEST) {
        printf("Boxer %d received request from boxer %d, timestamp %d\n", rank, processId, message.timestamp);
        QueueElement request(message.timestamp, processId, message.type);
        lamport.enqueue(request);

        MessageStruct reply;
        reply.timestamp = lamport.getTimestamp();
        MPI_Send(&reply, sizeof(reply), MPI_BYTE,
                processId, MSG_REPLY, MPI_COMM_WORLD);
    }

    if (status.MPI_TAG == MSG_REPLY) {
        printf("Boxer %d received reply from %d\n", rank, status.MPI_SOURCE);
        return MSG_REPLY;
    }

    if (status.MPI_TAG == MSG_RELEASE) {
        printf("Boxer %d received release from boxer %d, timestamp %d\n",
               rank, processId, message.timestamp);
        if (ringTaken[message.ringId]) {
            ringTaken[message.ringId] = false;
            nEmptyRings++;
        }
    }

    if (status.MPI_TAG == MSG_OPPONENT) {
        opponent = status.MPI_SOURCE;
        myRing = message.ringId;
        printf("Boxer %d notified by opponent\n", rank);
    }

    if (status.MPI_TAG == MSG_NOTIFY) {
        if (!ringTaken[message.ringId]) {
            ringTaken[message.ringId] = true;
            nEmptyRings--;
        }
        lamport.remove(processId);
    }

    return status.MPI_TAG;
}

void boxerLoop()
{
    while (1) {
        acquire();
        fight();
        if (rank < opponent) {
            release();
        }
        rest();
    }
}

void cleanerLoop()
{
    while (1) {
        cleanerRest();
        cleanerAcquire();
        clean();
        release();
    }
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    srand(time(NULL));

    if (rank % 4 == 0) {
        cleanerLoop();
    } else {
        boxerLoop();
    }

    MPI_Finalize();
    return 0;
}
