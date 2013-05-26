#include <list>
#include <algorithm>

#include "lamport.h"

using namespace std;

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

void Lamport::remove(int id)
{
    list<QueueElement>::iterator it;
    for (it = processQueue.begin(); it != processQueue.end(); it++) {
        if (it->id == id) {
            processQueue.erase(it);
        }
        break;
    }
}

int Lamport::getTimestamp()
{
    return timestamp;
}
