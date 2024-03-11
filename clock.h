// CS4760-001SS - Terry Ford Jr. - Project 4 OSS Scheduler - 03/08/2024
// https://github.com/tfordjr/oss-scheduler.git

#ifndef CLOCK_H
#define CLOCK_H

#define DISPATCH_AMOUNT 1000000      // 1 ms dispatch time
#define CHILD_LAUNCH_AMOUNT 1000000  // 1 ms child launch time
#define UNBLOCK_AMOUNT 1000000       // 1 ms process unblock/reschedule time

typedef struct Clock {
    int secs;
    int nanos;
} Clock;

void increment(Clock* c, int increment_amount){
    c->nanos = c->nanos + increment_amount; 
    if (c->nanos >= 1000000000){   // if over 1 billion nanos, add 1 second, sub 1 bil nanos
        c->nanos -= 1000000000;
        c->secs++;
    }    
}

int add_time(int addend1Secs, int addend1Nanos, int addend2Secs, int addend2Nanos){
    addend1Nanos += addend2Nanos;
    if (addend1Nanos >= 1000000000){   // if over 1 billion nanos, add 1 second, sub 1 bil nanos
        addend1Nanos -= 1000000000;
        addend1Secs++;
    } 
    return (addend1Secs + addend2Secs);
}

int subtract_time(int minuendSecs, int minuendNanos, int subtrahendSecs, int subtrahendNanos){
    minuendNanos -= subtrahendNanos;
    if (minuendNanos < 0){   // if negative nanos, add a second to nanos, take that second from
        minuendNanos += 1000000000;      // number to be subtracted
        subtrahendSecs--;
    } 
    return (minuendSecs - subtrahendSecs);
}

#endif