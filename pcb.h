// CS4760-001SS - Terry Ford Jr. - Project 3 Message Queues - 02/29/2024
// https://github.com/tfordjr/message-queues.git

#ifndef PCB_H
#define PCB_H

#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <fstream>

struct PCB {
    int occupied;     // either true or false
    pid_t pid;        // process id of this child
    int startSecs; // time when it was forked
    int startNanos;    // time when it was forked
};

void init_process_table(PCB processTable[]){
    for(int i = 0; i < 20; i++){
        processTable[i].occupied = 0;
        processTable[i].pid = 0;
        processTable[i].startSecs = 0;
        processTable[i].startNanos = 0;
    }
}

int process_table_vacancy(PCB processTable[], int simultaneous){
    for(int i = 0; i < simultaneous; i++){
        if (processTable[i].occupied == 0){
            return (i + 1);
        }
    }
    return 0;
}

bool process_table_empty(PCB processTable[], int simultaneous){
    for(int i = 0; i < simultaneous; i++){
        if (processTable[i].occupied){
            return 0;
        }
    }
    return 1;
}

void print_process_table(PCB processTable[], int simultaneous, int secs, int nanos, ofstream& file){
    static int next_print_secs = 0;  // static ints used to keep track of each 
    static int next_print_nanos = 0;   // process table print to be done

    if(secs > next_print_secs || secs == next_print_secs && nanos > next_print_nanos){
        printf("OSS PID: %d  SysClockS: %d  SysClockNano: %d  \nProcess Table:\nEntry\tOccupied  PID\tStartS\tStartN\n", getpid(), secs, nanos);
        for(int i = 0; i < simultaneous; i++){
            printf("%d\t%d\t%d\t%d\t%d\n", (i + 1), processTable[i].occupied, processTable[i].pid, processTable[i].startSecs, processTable[i].startNanos);
            file << to_string(i + 1) << "\t" << to_string(processTable[i].occupied) << "\t" << to_string(processTable[i].pid) << "\t" << to_string(processTable[i].startSecs) << "\t" << to_string(processTable[i].startNanos) << std::endl;
        }
        next_print_nanos = next_print_nanos + 500000000;
        if (next_print_nanos >= 1000000000){   // if over 1 billion nanos, add 1 second, sub 1 bil nanos
            next_print_nanos = next_print_nanos - 1000000000;
            next_print_secs++;
        }    
    }
}

void update_process_table_of_terminated_child(PCB processTable[], pid_t pid){
    for(int i = 0; i < 20; i++){
        if(processTable[i].pid == pid){  // if PCB pid equal to killed pid
            processTable[i].occupied = 0;
            processTable[i].pid = 0;
            processTable[i].startSecs = 0;
            processTable[i].startNanos = 0;
            return;
        } 
    }
}

void kill_all_processes(PCB processTable[]){
    for(int i = 0; i < 20; i++){
        if(processTable[i].occupied){  // if PCB pid equal to killed pid
            kill(processTable[i].pid, SIGKILL);
        } 
    }
}

# endif