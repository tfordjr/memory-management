// CS4760-001SS - Terry Ford Jr. - Project 2 Process Tables - 02/12/2024
// https://github.com/tfordjr/process-tables

#ifndef PCB_H
#define PCB_H

struct PCB {
    int occupied; // either true or false
    pid_t pid; // process id of this child
    int startSeconds; // time when it was forked
    int startNano; // time when it was forked
};

# endif