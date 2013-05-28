#ifndef BOXER_H
#define BOXER_H

struct MessageStruct {
    int timestamp;
    int ringId;
    ProcessType type;
};

void fight();
void clean();
void rest();
void cleanerRest();
void request(ProcessType type);
void notifyOpponent();
void notifyOthers();
int takeRing();
void acquire();
void cleanerAcquire();
void release();
int receive();
void boxerLoop();
void cleanerLoop();

#endif
