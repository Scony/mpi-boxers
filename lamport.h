#ifndef LAMPORT_H
#define LAMPORT_H

#include <list>

using namespace std;

enum ProcessType { BOXER, WORKER };

class QueueElement
{
    public:
    int timestamp;
    int id;
    ProcessType type;
    QueueElement(int timestamp, int id, ProcessType type);
    bool operator<(QueueElement &e);
};

class Lamport
{
    // processes waiting for ring (and referee) reservation
    list<QueueElement> processQueue;

    int timestamp;

    public:
    void increment();
    void update(int received);
    void enqueue(QueueElement &e);
    QueueElement &front();
    void remove(int id);
    int getTimestamp();
};

#endif