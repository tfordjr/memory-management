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
#include <random>
#include <chrono>
#include "pcb.h"
#include "clock.h"
using namespace std;

void help();
void increment(Clock*);
void print_process_table(PCB[], int, int, int);
void update_process_table_of_terminated_child(PCB[], pid_t);
bool launch_interval_satisfied(int, int, int);
bool process_table_vacancy(PCB[], int);
void launch_child(int);
int generate_random_number(int, int);
void init_process_table(PCB[]);

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
                launch_interval = (1000000 * atoi(optarg));  // converting ms to nanos
                break;            
        }
	}   // getopt loop completed here



    Clock* clock;       // init shm clock
    key_t key = ftok("/tmp", 35);
    int shmtid = shmget(key, sizeof(Clock), IPC_CREAT | 0666);
    clock = (Clock*)shmat(shmtid, NULL, 0);
    clock->secs = 0;   // init clock to 00:00
    clock->nanos = 0; 

    struct PCB processTable[20]; // Init Process Table Array of PCB structs (not shm)
    init_process_table(processTable);


    while(numChildren > 0){ // Main loop moves clock, checks for dead processes and launches new ones
        increment(clock);
        print_process_table(processTable, simultaneous, clock->secs, clock->nanos);        

        pid_t pid = waitpid(-1, nullptr, WNOHANG);  // non-blocking wait call for terminated child process
        if(pid != 0){     // if child has been terminated
            update_process_table_of_terminated_child(processTable, pid);
            pid = 0;
        }

        if(launch_interval_satisfied(launch_interval, clock->secs, clock->nanos) 
        && process_table_vacancy(processTable, simultaneous)){
            launch_child(time_limit);
        }               
    }       

    printf("\n\nLEFT MAIN LOOP\n\n");

    for (int i = 0; i < simultaneous; i++) 
        wait(NULL);	// Parent Waiting for children 

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

bool process_table_vacancy(PCB processTable[], int simultaneous){
    for(int i = 0; i < simultaneous; i++){
        if ((processTable + (i * sizeof(PCB)))->occupied == 0){
            return true;
        }
    }
    return false;
}

void init_process_table(PCB processTable[]){
    for(int i; i < 20; i++){
        (processTable + (i * sizeof(PCB)))->occupied = 0;
        (processTable + (i * sizeof(PCB)))->pid = 0;
        (processTable + (i * sizeof(PCB)))->startSecs = 0;
        (processTable + (i * sizeof(PCB)))->startNanos = 0;
    }
}

void print_process_table(PCB processTable[], int simultaneous, int secs, int nanos){

    static int next_print_secs = 0;  // static ints used to keep track of each 
    static int next_print_nanos = 0;   // process table print to be done

    if(secs > next_print_secs || secs == next_print_secs && nanos > next_print_nanos){
        printf("OSS PID: %d  SysClockS: %d  SysClockNano: %d  \nProcess Table:\nEntry\tOccupied\tPID\tStartS\t\tStartN\n", getpid(), secs, nanos);
        for(int i = 0; i < simultaneous; i++){
            printf("%d\t%d\t%d\t%d\t%d\n", i, (processTable + (i * sizeof(PCB)))->occupied, (processTable + (i * sizeof(PCB)))->pid, (processTable + (i * sizeof(PCB)))->startSecs, (processTable + (i * sizeof(PCB)))->startNanos);
        }
        next_print_nanos = next_print_nanos + 500000000;
        if (next_print_nanos >= 1000000000){   // if over 1 billion nanos, add 1 second, sub 1 bil nanos
            next_print_nanos = next_print_nanos - 1000000000;
            next_print_secs++;
        }    
    }
}


bool launch_interval_satisfied(int launch_interval, int secs, int nanos){
    static int last_launch_secs = 0;  // static ints used to keep track of 
    static int last_launch_nanos = 0;   // most recent process launch

    int elapsed_secs = secs - last_launch_secs; 
    int elapsed_nanos = nanos - last_launch_nanos;

    while (elapsed_nanos < 0) {   // fix if subtracted time is too low
        elapsed_secs--;
        elapsed_nanos += 1000000000;
    }

    if (elapsed_secs > 0 || (elapsed_secs == 0 && elapsed_nanos >= launch_interval)) {        
        last_launch_secs = secs;  // Update the last launch time
        last_launch_nanos = nanos;        
        return true;
    } else {
        return false;
    }
}


void launch_child(int time_limit){
    int rand_secs = generate_random_number(1, (time_limit - 1));
    int rand_nanos = generate_random_number(0, 999999999);
    string user_parameters = std::to_string(rand_secs) + " " + std::to_string(rand_nanos); 

    pid_t childPid = fork(); // This is where the child process splits from the parent        
    if (childPid == 0 ) {            // Each child uses exec to run ./user	
        execl("./user", "user", user_parameters.c_str(), NULL);            
        fprintf(stderr, "Failed to execute \n");      // IF child makes it 
        exit(EXIT_FAILURE);                          // this far exec did not work				
    } else 	if (childPid == -1) {  // Error message for failed fork (child has PID -1)
        perror("master: Error: Fork has failed!");
        exit(0);
    }          
}

int generate_random_number(int min, int max) {
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::default_random_engine generator(seed);
    std::uniform_int_distribution<int> distribution(min, max);
    int random_number = distribution(generator);
    return random_number;
}

void update_process_table_of_terminated_child(PCB processTable[], pid_t pid){
    for(int i = 0; i < 20; i++){
        if((processTable + (i * sizeof(PCB)))->pid == pid){  // if PCB pid equal to killed pid
            (processTable + (i * sizeof(PCB)))->occupied = 0;
            (processTable + (i * sizeof(PCB)))->pid = 0;
            (processTable + (i * sizeof(PCB)))->startSecs = 0;
            (processTable + (i * sizeof(PCB)))->startNanos = 0;
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
    printf("\t-i The argument following -i will be lauch interval between process launch in milliseconds.\n");
    printf("\t args will default to appropriate values if not provided.\n");
}