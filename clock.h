// CS4760-001SS - Terry Ford Jr. - Project 3 Message Queues - 02/29/2024
// https://github.com/tfordjr/message-queues.git

#ifndef CLOCK_H
#define CLOCK_H

typedef struct Clock {
    int secs;
    int nanos;
} Clock;

void increment(Clock* c, int numProcesses){
    // c->nanos = c->nanos + (250000000/numProcesses); // 1/4 second (250ms) / num processes
    c->nanos = c->nanos + 250;
    if (c->nanos >= 1000000000){   // if over 1 billion nanos, add 1 second, sub 1 bil nanos
        c->nanos = c->nanos - 1000000000;
        c->secs++;
    }    
}

#endif