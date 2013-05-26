#include <list>
#include <algorithm>
#include <stdexcept>

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
        }
        break;
    }
}

int Lamport::size()
{
    return processQueue.size();
}

int Lamport::getTimestamp()
{
    return timestamp;
}
