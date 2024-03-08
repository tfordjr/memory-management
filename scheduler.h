// CS4760-001SS - Terry Ford Jr. - Project 4 OSS Scheduler - 03/08/2024
// https://github.com/tfordjr/oss-scheduler.git

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "pcb.h"

// DEFINE THREE QUEUES    as arrays of size simultaneous?  

int scheduler(PCB processTable[], int simultaneous){
    if (process_table_empty(processTable, simultaneous)){
        return -1;
    }
    // I don't think we care about i, which process ran last. may delete parameter
    int i;


    // CHECK Q0
    // CHECK Q1
    // CHECK Q2
    // SCHEDULE HIGHEST PRIORITY ELIGIBLE PROCESS FOUND!
    // RETURN i WHERE i is the processes' spot on the PCB

    // PROCESS COMPLETED SUCCESSFULLY, DESCEND QUEUE (ADD TO BACK OF QUEUE)
    // PROCESS INTERRUPTED, BLOCK IT!
    // PROCESS IS NO LONGER BLOCKED, PUT ASCEND QUEUE (ADD TO BACK OF QUEUE)
    // PROCESS TERMINATES!


    return i; // this case we're returning to comm with same process as last time, 
}    

#endif