// CS4760-001SS - Terry Ford Jr. - Project 5 Resource Management - 03/29/2024
// https://github.com/tfordjr/resource-management.git

#ifndef CLOCK_H
#define CLOCK_H

#define DISPATCH_AMOUNT 1e9      // 1 ms dispatch time
#define CHILD_LAUNCH_AMOUNT 1000  // .001 ms child launch time
#define UNBLOCK_AMOUNT 1000       // .001 ms process unblock/reschedule time

typedef struct Clock {
    int secs;
    int nanos;
} Clock;

void increment(Clock* c, int increment_amount){
    c->nanos = c->nanos + increment_amount; 
    if (c->nanos >= 1e9){   // if over 1 billion nanos, add 1 second, sub 1 bil nanos
        c->nanos -= 1e9;
        c->secs++;
    }    
}

void add_time(int *addend1Secs, int *addend1Nanos, int addend2Nanos){
    *addend1Nanos += addend2Nanos;
    if (*addend1Nanos >= 1e9){   // if over 1 billion nanos, add 1 second, sub 1 bil nanos
        *addend1Nanos -= 1e9;
        *addend1Secs++;
    }
}

#endif