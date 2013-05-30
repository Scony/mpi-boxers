#ifndef BOXER_H
#define BOXER_H

struct MessageStruct {
    int timestamp;
    int ringId;
    int opponent;
    ProcessType type;
};

void fight();
void clean();
void rest();
void cleanerRest();
void request();
void notifyOpponent();
void notifyOthers();
int takeRing();
void acquire();
void cleanerAcquire();
void release();
int receive();
int ireceive();
void boxerLoop();
void cleanerLoop();

#endif
