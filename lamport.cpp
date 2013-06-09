#include <list>
#include <algorithm>
#include <stdexcept>
#include <stdio.h>

#include "lamport.h"

using namespace std;

Lamport::Lamport()
{
    timestamp = 1;
}

QueueElement::QueueElement(int timestamp, int id, ProcessType type)
{
    this->timestamp = timestamp;
    this->id = id;
    this->type = type;
}

bool QueueElement::operator<(QueueElement &e)
{
    if (timestamp != e.timestamp) {
        return timestamp < e.timestamp;
    } else {
        return id < e.id;
    }
}

void Lamport::increment()
{
    timestamp++;
}

void Lamport::update(int received)
{
    timestamp = max(timestamp, received) + 1;
}

void Lamport::enqueue(QueueElement &e)
{
    processQueue.push_back(e);
    processQueue.sort();
}

QueueElement &Lamport::front()
{
    return processQueue.front();
}

QueueElement &Lamport::second()
{
    if (processQueue.size() < 2) {
        throw runtime_error("Not enough requests in queue.");
    }
    return *(++processQueue.begin());
}

void Lamport::remove(int id)
{
    list<QueueElement>::iterator it;
    for (it = processQueue.begin(); it != processQueue.end(); it++) {
        if (it->id == id) {
            processQueue.erase(it);
            break;
        }
    }
}

int Lamport::size()
{
    return processQueue.size();
}

bool Lamport::isFirst(int id)
{
    if (size() >= 1) {
        return front().id == id;
    }
    return false;
}

bool Lamport::isSecond(int id)
{
    if (size() >= 2) {
        return second().id == id;
    }
    return false;
}

bool Lamport::isSecondBoxer()
{
    if (size() >= 2) {
        return second().type == BOXER;
    }
    return false;
}

int Lamport::getTimestamp()
{
    return timestamp;
}

void Lamport::printQueue(int rank)
{
    char token = (random() % 26) + 'a';
    int i = 0;
    list<QueueElement>::iterator it;
    for (it = processQueue.begin(); it != processQueue.end(); it++) {
        printf("   %d[%c]: %dth process %d, timestamp %d\n", rank, token, i, it->id, it->timestamp);
        i++;
    }
}
