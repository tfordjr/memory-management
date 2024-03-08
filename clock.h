// CS4760-001SS - Terry Ford Jr. - Project 4 OSS Scheduler - 03/08/2024
// https://github.com/tfordjr/oss-scheduler.git

#ifndef CLOCK_H
#define CLOCK_H

#define DISPATCH_AMOUNT 1000000      // 1 ms dispatch time
#define CHILD_LAUNCH_AMOUNT 1000000  // 1 ms child launch time

typedef struct Clock {
    int secs;
    int nanos;
} Clock;

void increment(Clock* c, int increment_amount){
    c->nanos = c->nanos + increment_amount; 
    if (c->nanos >= 1000000000){   // if over 1 billion nanos, add 1 second, sub 1 bil nanos
        c->nanos = c->nanos - 1000000000;
        c->secs++;
    }    
}

#endif