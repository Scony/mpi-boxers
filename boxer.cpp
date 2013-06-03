#define _XOPEN_SOURCE
#include <mpi.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "lamport.h"
#include "boxer.h"

#define NRINGS 5
#define NREFEREES 4
#define NPROCS 32

enum MsgTag { MSG_REQUEST, MSG_RELEASE, MSG_REPLY, MSG_OPPONENT, MSG_NOTIFY };

using namespace std;

int size, rank;
int opponent = -1;
int myRing;
ProcessType type = BOXER;

Lamport lamport;
//int nEmptyRings = NRINGS;
int nAvailableReferees = NREFEREES;
bool ringTaken[NRINGS] = {false};
bool replied[NPROCS] = {false};

void fight()
{
    printf(">>>>>>>>>> Boxer %d fighting with %d on ring %d\n", rank, opponent, myRing);
    sleep(1 + (random() % 5));
}

void clean()
{
    printf(">>>>>>>>>> Cleaner %d cleaning ring %d\n", rank, myRing);
    sleep(1 + (random() % 5));
}

void rest()
{
    printf("Boxer %d resting\n", rank);

    int period = 1 + (random() % 5);
    for (int i = 0; i < period; i++) {
       // ireceive();
       // receive();
       sleep(1);
    }
    // sleep(period);
}

void cleanerRest()
{
    printf("Cleaner %d resting\n", rank);
    int period = 4 + (random() % 5);
    for (int i = 0; i < period; i++) {
       // ireceive();
       // receive();
       sleep(1);
    }
    // sleep(period);
}

void request()
{
    lamport.increment();
    //printf("Process %d timestamp: %d\n", rank, lamport.getTimestamp());
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

    //lamport.increment();
    //printf("Process %d timestamp: %d\n", rank, lamport.getTimestamp());
    MessageStruct message;
    message.timestamp = lamport.getTimestamp();
    message.ringId = myRing;
    MPI_Send(&message, sizeof(message), MPI_BYTE,
             opponent, MSG_OPPONENT, MPI_COMM_WORLD);
    printf("Boxer %d notifying opponent: %d\n", rank, lamport.second().id);
}

void notifyOthers()
{
    //lamport.increment();
    //printf("Process %d timestamp: %d\n", rank, lamport.getTimestamp());
    for (int i = 0; i < size; i++) {
        if (i != rank) {
            MessageStruct message;
            message.ringId = myRing;
            message.timestamp = lamport.getTimestamp();
            message.type = type;
            if (type == BOXER) {
                message.opponent = opponent;
            }
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

int countRings()
{
    int count = 0;
    for (int i = 0; i < NRINGS; i++) {
        if (!ringTaken[i]) {
            count++;
        }
    }
    return count;
}

bool allReplied()
{
    for (int i = 0; i < size; i++) {
        if (i != rank && !replied[i]) {
            return false;
        }
    }
    return true;
}

void clearReplied()
{
    for (int i = 0; i < size; i++) {
        replied[i] = false;
    }
}

void acquire()
{
    printf("Boxer %d wants to acquire a ring\n", rank);
    request();

    int nReplies = 0;
    clearReplied();
    while ( !(allReplied() &&
              lamport.isFirst(rank) &&
              lamport.isSecondBoxer() &&
              countRings() > 0 &&
              nAvailableReferees > 0) ) {

        int count = countRings();
        printf("-----\n");
        lamport.printQueue(rank);
        printf("   %d: nAvailableReferees = %d\n", rank, nAvailableReferees);
        // printf("   %d: nEmptyRings = %d, countRings() = %d\n", rank, nEmptyRings, count);
        printf("   %d: nReplies = %d/%d\n", rank, nReplies, size-1);
        // if (nEmptyRings != count) {
        //     dprintf(2, "nie zgadza sie %d != %d\n", nEmptyRings, count);
        // }

        // wait
        // receive msgs etc
        int messageTag = receive();
        if (messageTag == MSG_REPLY) {
            nReplies++;
        } else if (messageTag == MSG_OPPONENT) {
            return;
        }
        //printf("Boxer %d queue front: %d, timestamp: %d\n",
        //        rank, lamport.front().id, lamport.front().timestamp);
    }

    myRing = takeRing();
    if (myRing < 0) dprintf(2, "takeRing returned -1\n");
    //nEmptyRings--;
    if (myRing < 0) {
        printf("===== Oops, no empty ring after all?!\n");
    }
    nAvailableReferees--;
    opponent = lamport.second().id;
    notifyOpponent();

    lamport.remove(rank);
    lamport.remove(opponent);
    notifyOthers();

    printf("Boxer %d acquired ring %d\n", rank, myRing);
}

void cleanerAcquire()
{
    printf("Cleaner %d wants to acquire a ring\n", rank);

    request();

    int nReplies = 0;
    clearReplied();
    while ( !(allReplied() &&
              (lamport.isFirst(rank) || lamport.isSecond(rank)) &&
              countRings() > 0) ) {
        // wait
        // receive msgs etc
        int messageTag = receive();
        if (messageTag == MSG_REPLY) {
            nReplies++;
        }
        //printf("Cleaner %d queue front: %d, timestamp: %d\n", rank, lamport.front().id, lamport.front().timestamp);
    }

    myRing = takeRing();
    //nEmptyRings--;
    if (myRing < 0) {
        printf("===== Oops, no empty ring after all?!\n");
    }
    lamport.remove(rank);
    notifyOthers();

    //printf("-----\n");
    //lamport.printQueue(rank);
    //printf("   %d: timestamp = %d\n", rank, lamport.getTimestamp());
    //printf("   %d: nAvailableReferees = %d\n", rank, nAvailableReferees);

    printf("Cleaner %d acquired ring\n", rank);
}

void release()
{
    printf("Process %d releasing ring\n", rank);

    ringTaken[myRing] = false;
    //nEmptyRings++;
    if (type == BOXER) {
        nAvailableReferees++;
    }

    // send release msg to every node
    lamport.increment();
    //printf("Process %d timestamp: %d\n", rank, lamport.getTimestamp());
    for (int i = 0; i < size; i++) {
        if (i != rank) {
            MessageStruct message;
            message.timestamp = lamport.getTimestamp();
            message.ringId = myRing;
            message.type = type;
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
    usleep(100);

    MessageStruct message;
    MPI_Status status;
    MPI_Recv(&message, sizeof(message), MPI_BYTE,
            MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    lamport.update(message.timestamp);
    int processId = status.MPI_SOURCE;

    if (status.MPI_TAG == MSG_REQUEST) {
        //printf("Boxer %d received request from boxer %d, timestamp %d\n", rank, processId, message.timestamp);
        QueueElement request(message.timestamp, processId, message.type);
        lamport.enqueue(request);

        MessageStruct reply;
        reply.timestamp = lamport.getTimestamp();
        MPI_Send(&reply, sizeof(reply), MPI_BYTE,
                processId, MSG_REPLY, MPI_COMM_WORLD);
    }

    if (status.MPI_TAG == MSG_REPLY) {
        //printf("Boxer %d received reply from %d\n", rank, status.MPI_SOURCE);
        if (!replied[status.MPI_SOURCE]) {
            replied[status.MPI_SOURCE] = true;
            return MSG_REPLY;
        }
        return -1;
    }

    if (status.MPI_TAG == MSG_RELEASE) {
        //printf("Boxer %d received release from boxer %d, timestamp %d\n",
        //       rank, processId, message.timestamp);
        if (ringTaken[message.ringId]) {
            ringTaken[message.ringId] = false;
            //nEmptyRings++;
            if (message.type == BOXER) {
                nAvailableReferees++;
            }
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
            //nEmptyRings--;
            if (message.type == BOXER) {
                nAvailableReferees--;
            }
        }
        lamport.remove(processId);
        //if (rank == 1) {
        //    printf("removed %d\n", processId);
        //}
        if (message.type == BOXER) {
            lamport.remove(message.opponent);
            //if (rank == 1) {
            //    printf("removed %d\n", opponent);
            //}
        }
    }

    return status.MPI_TAG;
}

int ireceive()
{
    // non-blocking receive
    // update timestamp
    // if request -> enqueue and reply with timestamp
    // if release -> remove request from queue
    // if reply -> just return message type (calling function can count replies)
    usleep(100);

    MessageStruct message;
    MPI_Request req;
    MPI_Status status;
    int flag = 0;
    MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);
    if (!flag) {
        return -1;
    }
    MPI_Recv(&message, sizeof(message), MPI_BYTE,
            MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    //MPI_Test(&req, &flag, &status);
    if (flag == 0)		// nothing received
      return -1;

    lamport.update(message.timestamp);
    int processId = status.MPI_SOURCE;

    if (status.MPI_TAG == MSG_REQUEST) {
        //printf("Boxer %d received request from boxer %d, timestamp %d\n", rank, processId, message.timestamp);
        QueueElement request(message.timestamp, processId, message.type);
        lamport.enqueue(request);

        MessageStruct reply;
        reply.timestamp = lamport.getTimestamp();
        MPI_Send(&reply, sizeof(reply), MPI_BYTE,
                processId, MSG_REPLY, MPI_COMM_WORLD);
    }

    if (status.MPI_TAG == MSG_REPLY) {
        //printf("Boxer %d received reply from %d\n", rank, status.MPI_SOURCE);
        return MSG_REPLY;
    }

    if (status.MPI_TAG == MSG_RELEASE) {
        //printf("Boxer %d received release from boxer %d, timestamp %d\n",
        //       rank, processId, message.timestamp);
        if (ringTaken[message.ringId]) {
            ringTaken[message.ringId] = false;
            //nEmptyRings++;
            if (message.type == BOXER) {
                nAvailableReferees++;
            }
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
            //nEmptyRings--;
            if (message.type == BOXER) {
                nAvailableReferees--;
            }
        }
        lamport.remove(processId);
        //if (rank == 1) {
        //    printf("removed %d\n", processId);
        //}
        if (message.type == BOXER) {
            lamport.remove(message.opponent);
            //if (rank == 1) {
            //    printf("removed %d\n", opponent);
            //}
        }
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

    if (rank % 8 == 0) {
        type = CLEANER;
        cleanerLoop();
    } else {
        type = BOXER;
        boxerLoop();
    }

    MPI_Finalize();
    return 0;
}
