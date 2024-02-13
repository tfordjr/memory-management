// CS4760-001SS - Terry Ford Jr. - Project 2 Process Tables - 02/12/2024
// https://github.com/tfordjr/process-tables

#ifndef CLOCK_H
#define CLOCK_H

typedef struct Clock {
    int secs;
    int nanos;
} Clock;

void increment(Clock* c){
    c->nanos = c->nanos + 200;
    if (c->nanos >= 1000000000){   // if over 1 billion nanos, add 1 second, sub 1 bil nanos
        c->nanos = c->nanos - 1000000000;
        c->secs++;
    }    
}

#endif