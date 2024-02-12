// CS4760-001SS - Terry Ford Jr. - Project 2 Process Tables - 02/12/2024
// https://github.com/tfordjr/process-tables

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include "pcb.h"
#include "clock.h"
using namespace std;

void help();
void increment(Clock*);
void print_process_table(PCB[], int, int, int);
void update_process_table_of_terminated_child(PCB[], pid_t);
bool launch_interval_satisfied(int, int, int);

int main(int argc, char** argv){
    int option, numChildren = 1, simultaneous = 1, time_limit = 2, launch_interval = 100;  
    while ( (option = getopt(argc, argv, "hn:s:t:i:")) != -1) {   // getopt implementation
        switch(option) {
            case 'h':
                help();
                return 0;     // terminates if -h is present
            case 'n':                                
                numChildren = atoi(optarg);
                break;
            case 's':          
                simultaneous = atoi(optarg);
                break;
            case 't':
                time_limit = atoi(optarg);
                break; 
            case 'i':
                launch_interval = atoi(optarg);
                break;            
        }
	}   // getopt loop completed here



    Clock* clock;       // init shm clock
    key_t key = ftok("/tmp", 35);
    int shmtid = shmget(key, sizeof(Clock), IPC_CREAT | 0666);
    clock = (Clock*)shmat(shmtid, NULL, 0);
    clock->secs = 0;   // init clock to 00:00
    clock->nanos = 0; 

    struct PCB processTable[20]; // Init Process Table Array of PCB structs    
    


    while(numChildren > 0){
        increment(clock);
        print_process_table(processTable, simultaneous, clock->secs, clock->nanos);        

        pid_t pid = waitpid(-1, &status, WNOHANG);  // non-blocking wait call for terminated child process
        if(pid != 0){     // if child has been terminated
            update_process_table_of_terminated_child(processTable, pid);
            pid = 0;
        }

        if(launch_interval_satisfied(launch_interval, clock->secs, clock->nanos) 
            && process_table_vacancy()){
                launch_child();
        }        
    }





    
    // for (int i = 0; i < numChildren; i++) {  //  OLD CODE FROM P1
    //     pid_t childPid = fork(); // This is where the child process splits from the parent
        
    //     if (childPid == 0 ) {             // Each child uses exec to run ./user	
	// 	 	// static char *args[] = { "./user", (char *)iterations, NULL };
    //         // execv(args[0], args);
    //         execl("./user", "user", (std::to_string(iterations)).c_str(), NULL);            
    //         fprintf(stderr, "Failed to execute \n");      // IF child makes it 
    //         exit(EXIT_FAILURE);                          // this far exec did not work				
	// 	} else 	if (childPid == -1) {  // Error message for failed fork (child has PID -1)
    //         perror("master: Error: Fork has failed!");
    //         exit(0);
    //     }       
    //     running++;  

    //     if(running >= simultaneous){ //If number of currently running processes at max number
    //         wait(NULL);  // Parent waits to assure children perform in order
    //         running--;
    //     } 
    // }

    // for (int i = 0; i < numChildren; i++) 
    //     wait(NULL);	// Parent Waiting for children    

	printf("Child processes have completed.\n");
    printf("Parent is now ending.\n");
    return 0;
}


void increment(Clock* c){
    c->nanos = c->nanos + 100;
    if (c->nanos >= 1000000000){   // if over 1 billion nanos, add 1 second, sub 1 bil nanos
        c->nanos = c->nanos - 1000000000;
        c->secs++;
    }    
}


void print_process_table(PCB processTable[], int simultaneous, int secs, int nanos){

    static int next_print_secs = 0;  // static ints used to keep track of each 
    static int next_print_nanos = 0;   // process table print to be done

    if(secs > next_print_secs || secs == next_print_secs && nanos > next_print_nanos){
        printf("OSS PID: %d  SysClockS: %d  SysClockNano: %d  \nProcess Table:\nEntry\tOccupied\tPID\tStartS\tStartN\n", getpid(), secs, nanos);
        for(int i = 0; i < simultaneous; i++){
            printf("%d\t\t%d\t\t%d\t\t%d\t\t%d\n", i, processTable[i]->occupied, processTable[i]->pid, processTable[i]->startSecs, processTable[i]->startNanos);
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
        if(processTable[i]->pid == pid){  // if PCB pid equal to killed pid
            processTable[i]->occupied = 0;
            processTable[i]->pid = 0;
            processTable[i]->startSecs = 0;
            processTable[i]->startNanos = 0;
            return;
        } 
    }
}

void help(){   // Help message here
    printf("-h detected. Printing Help Message...\n");
    printf("The options for this program are: \n");
    printf("\t-h Help will halt execution, print help message, and take no arguments.\n");
    printf("\t-n The argument following -n will be number of total processes to be run.\n");
    printf("\t-s The argument following -s will be max number of processes to be run simultaneously\n");
    printf("\t-t The argument following -t will be the max time limit for each user process created.\n");
    printf("\t-t The argument following -i will be lauch interval between process launch in milliseconds.\n");
    printf("\t args will default to appropriate values if not provided.\n");
}