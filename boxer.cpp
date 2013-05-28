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
#define MSG_OPPONENT 4

using namespace std;

int size, rank;
int opponent = -1;
Lamport lamport;

void fight()
{
    printf(">>>>>>>>>> Boxer %d fighting with %d\n", rank, opponent);
    sleep(3 + (random() % 5));
}

void clean()
{
    printf(">>>>>>>>>> Cleaner %d cleaning\n", rank, opponent);
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

void acquire()
{
    printf("Boxer %d wants to acquire a ring\n", rank);

    request(BOXER);

    int nReplies = 0;
    while ( !(nReplies == size - 1 &&
              lamport.isFirst(rank) &&
              lamport.isSecondBoxer()) ) {
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

    // tell second boxer?
    opponent = lamport.second().id;
    printf("Boxer %d, opponent: %d\n", rank, opponent);

    lamport.increment();
    MessageStruct message;
    message.timestamp = lamport.getTimestamp();
    MPI_Send(&message, sizeof(message), MPI_BYTE,
             opponent, MSG_OPPONENT, MPI_COMM_WORLD);
    printf("Boxer %d notifying opponent: %d\n", rank, lamport.second());

    printf("Boxer %d acquired ring\n", rank);

}

void cleanerAcquire()
{
    printf("Cleaner %d wants to acquire a ring\n", rank);

    request(CLEANER);

    int nReplies = 0;
    while ( !(nReplies == size - 1 &&
              (lamport.isFirst(rank) || lamport.isSecond(rank))) ) {
        // wait
        // receive msgs etc
        int messageTag = receive();
        printf("messageTag: %d\n", messageTag);
        if (messageTag == MSG_REPLY) {
            nReplies++;
        }
        printf("Cleaner %d queue front: %d, timestamp: %d\n", rank, lamport.front().id, lamport.front().timestamp);
    }

    printf("Cleaner %d acquired ring\n", rank);

}

void release()
{
    printf("Process %d releasing ring\n", rank);

    // send release msg to every node
    lamport.increment();
    for (int i = 0; i < size; i++) {
        if (i != rank) {
            MessageStruct message;
            message.timestamp = lamport.getTimestamp();
            MPI_Send(&message, sizeof(message), MPI_BYTE,
                     i, MSG_RELEASE, MPI_COMM_WORLD);
        }
    }

    printf("Process %d released ring\n", rank);
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
        lamport.remove(processId);
    }

    if (status.MPI_TAG == MSG_OPPONENT) {
        opponent = status.MPI_SOURCE;
        printf("Boxer %d notified by opponent\n", rank);
    }

    return status.MPI_TAG;
}

void boxerLoop()
{
    while (1) {
        acquire();
        fight();
        release();
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
