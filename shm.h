// CS4760-001SS - Terry Ford Jr. - Project 2 Process Tables - 02/12/2024
// https://github.com/tfordjr/process-tables

#ifndef SHM_H
#define SHM_H

typedef struct Clock {
    int secs;
    int nanos;
} Clock;

Clock wind(Clock c){
    c.secs = 0;
    c.nanos = 0;
    return c;
}

#endif