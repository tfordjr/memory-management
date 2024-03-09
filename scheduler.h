// CS4760-001SS - Terry Ford Jr. - Project 4 OSS Scheduler - 03/08/2024
// https://github.com/tfordjr/oss-scheduler.git

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <queue>
#include "pcb.h"

std::queue<pid_t> Q0;  // Q0 10 ms or 10000000 ns
std::queue<pid_t> Q1;  // Q1 20 ms or 20000000 ns
std::queue<pid_t> Q2;  // Q2 40 ms or 40000000 ns

// WHO MOVES PROCESSES FROM QUEUE TO QUEUE????
// child proc determines run result, messages this to oss
// oss checks child run result, updates process_table
// oss passes process_table to us, we handle queue movements

int check_blocked_processes(PCB[], int, int, int);

        // scheduler() determines i (next process location) and associated time slice
void scheduler(PCB processTable[], int simultaneous, int *i, int *time_slice, int *unblocks, int secs, int nanos){  
    if (process_table_empty(processTable, simultaneous)){
        *i = -1;
        *time_slice = 0;
        return;
    }

        // FOR EACH BLOCKED PROCESS, CHECK IF NO LONGER BLOCKED, SEND TO Q0
    unblocks = check_blocked_processes(processTable, simultaneous, secs, nanos);

    pid_t pid;    
    if(!Q0.empty()){
        pid = Q0.front();
        *time_slice = 10000000;  // 10ms 
    } else if(!Q1.empty()){
        pid = Q1.front();
        *time_slice = 20000000;  // 20ms 
    } else if(!Q2.empty()){
        pid = Q2.front();
        *time_slice = 40000000;  // 40ms 
    } 

    *i = return_position_of_given_pid(processTable, simultaneous, pid);
    return; 
}    

// DURING OSS MAIN LOOP, I LEAVE PID BEING WORKED ON AT THE FRONT OF LINE IT WAS IN
// AT THE END OF MAIN LOOP, WE REMOVE PROCESS FROM THE QUEUE IT WAS IN, AND DETERMINE 
// WHERE IT SHOULD GO AS A RESULT OF ITS RUNTIME RESULT

void descend_queues(pid_t pid){  // If full time quantum is used, move down ONE Queue
    if (Q0.front() == pid){
        Q0.pop();
        Q1.push(pid);
    } else if (Q1.front() == pid){
        Q1.pop();
        Q2.push(pid);
    } else if (Q2.front() == pid){  // If in bottom queue, move to back of the line
        Q2.pop();
        Q2.push(pid);
    }
}   

void remove_process_from_scheduling_queues(pid_t pid){ // when proc is terminated or blocked
    if (Q2.front() == pid){
        Q2.pop();
    } else if (Q1.front() == pid){
        Q1.pop();
    } else if (Q0.front() == pid){
        Q0.pop();
    }
}
    
void schedule_unblocked_process(pid_t pid){  // Move recently unblocked proc to back of Q0
    Q0.push(pid);
}

    // This says if blocked process is no longer io blocked, schedule it
int check_blocked_processes(PCB processTable[], int simultaneous, int secs, int nanos){
    int unblocks = 0;
    for(int i = 0; i < simultaneous; i++){
        if(processTable[i].blocked && (secs > processTable[i].eventBlockedUntilSec || 
        secs == processTable[i].eventBlockedUntilSec && nanos > processTable[i].eventBlockedUntilNano)){          
            schedule_unblocked_process(processTable[i].pid);
            unblocks++;            
        } 
    }
    return unblocks;
}

#endif