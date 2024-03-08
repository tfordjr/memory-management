// CS4760-001SS - Terry Ford Jr. - Project 4 OSS Scheduler - 03/08/2024
// https://github.com/tfordjr/oss-scheduler.git

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "pcb.h"

// DEFINE THREE QUEUES    as arrays of size simultaneous?  
// Q0 10 ms or 10000000 ns
// Q1 20 ms or 20000000 ns
// Q2 40 ms or 40000000 ns

// WHO MOVES PROCESSES FROM QUEUE TO QUEUE????
// child proc determines run result, messages this to oss
// 

        // scheduler() determines i (next process location) and associated time slice
void scheduler(PCB processTable[], int simultaneous, int *i, int *time_slice){  
    if (process_table_empty(processTable, simultaneous)){
        *i = -1;
        return;
    }
    
    


    // CHECK Q0
    // CHECK Q1
    // CHECK Q2
    // SCHEDULE HIGHEST PRIORITY ELIGIBLE PROCESS FOUND!
    // RETURN i WHERE i is the processes' spot on the PCB

    // PROCESS COMPLETED SUCCESSFULLY, DESCEND QUEUE (ADD TO BACK OF QUEUE)
    // PROCESS INTERRUPTED, BLOCK IT!
    // PROCESS IS NO LONGER BLOCKED, PUT ASCEND QUEUE (ADD TO BACK OF QUEUE)
    // PROCESS TERMINATES!


    return; // returning pcb slot of next process
}    

#endif