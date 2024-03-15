// CS4760-001SS - Terry Ford Jr. - Project 4 OSS Scheduler - 03/08/2024
// https://github.com/tfordjr/oss-scheduler.git

#ifndef CLOCK_H
#define CLOCK_H

#define DISPATCH_AMOUNT 10000      // .01 ms dispatch time
#define CHILD_LAUNCH_AMOUNT 10000  // .01 ms child launch time
#define UNBLOCK_AMOUNT 10000       // .01 ms process unblock/reschedule time

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

double add_time(int addend1Secs, int addend1Nanos, int addend2Secs, int addend2Nanos){
    addend1Nanos += addend2Nanos;
    if (addend1Nanos >= 1e9){   // if over 1 billion nanos, add 1 second, sub 1 bil nanos
        addend1Nanos -= 1e9;
        addend1Secs++;
    } 
    addend1Secs += addend2Secs;

    return (addend1Secs + (addend1Nanos/1e9));
}

double subtract_time(int minuendSecs, int minuendNanos, int subtrahendSecs, int subtrahendNanos){
    minuendNanos -= subtrahendNanos;
    if (minuendNanos < 0){   // if negative nanos, add a second to nanos, take that second from
        minuendNanos += 1e9;      // number to be subtracted
        minuendSecs--;
    } 
    minuendSecs -= subtrahendSecs;

    return (minuendSecs + (minuendNanos/1e9));  
}

#endif