#define _XOPEN_SOURCE
#include <mpi.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <queue>
#include <algorithm>

using namespace std;

enum ProcessType { BOXER, WORKER };

class QueueElement
{
    int timestamp;
    int id;
    ProcessType type;

    public:
    bool operator<(QueueElement &e)
    {
        return this.timestamp < e.timestamp;
    }
}

class Lamport
{
    // processes waiting for ring (and referee) reservation
    vector<queue<QueueElement>> processQueues;

    int timestamp;

    public:
    void increment()
    {
        timestamp++;
    }

    void update(int received)
    {
        timestamp = max(timestamp, received) + 1;
    }

    void enqueue(int ringId, QueueElement &e)
    {
        processQueues[ringId].push(e);
    }
}
