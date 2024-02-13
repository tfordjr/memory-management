// CS4760-001SS - Terry Ford Jr. - Project 2 Process Tables - 02/12/2024
// https://github.com/tfordjr/process-tables

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <csignal>
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

void launch_child(PCB[], int, int, Clock*);
int generate_random_number(int, int);
bool launch_interval_satisfied(int, Clock*);
void help();
void timeout_handler(int);
void ctrl_c_handler(int);

volatile sig_atomic_t term = 0;  // signal handling global
struct PCB processTable[20]; // Init Process Table Array of PCB structs (not shm)

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

    std::signal(SIGALRM, timeout_handler);  // signal handlers setup
    std::signal(SIGINT, ctrl_c_handler);
    alarm(60);   // timeout timer
          
    init_process_table(processTable); // init local process table
    Clock* clock;                             // declare clock locally
    key_t key = ftok("/tmp", 35);             // init shm clock
    int shmtid = shmget(key, sizeof(Clock), IPC_CREAT | 0666);
    clock = (Clock*)shmat(shmtid, NULL, 0);
    clock->secs = 0;   // init clock to 00:00
    clock->nanos = 0;         
                        //  ---------  MAIN LOOP  ---------   
    while(numChildren > 0){ // incrs clock, checks for dead processes and launches new ones
        increment(clock);
        print_process_table(processTable, simultaneous, clock->secs, clock->nanos);        

        pid_t pid = waitpid(-1, nullptr, WNOHANG);  // non-blocking wait call for terminated child process
        if(pid != 0){     // if child has been terminated
            update_process_table_of_terminated_child(processTable, pid);  // clear spot in pcb
            pid = 0;
        }

        if(launch_interval_satisfied(launch_interval, clock)   // child process launch check
        && process_table_vacancy(processTable, simultaneous)){
            cout << "Launching Child Process..." << endl;
            numChildren--;
            launch_child(processTable, time_limit, simultaneous, clock);
        }               
    }                   // --------- END OF MAIN LOOP --------- 


    for (int i = 0; i < simultaneous; i++) 
        wait(NULL);	// Parent Waiting for children 

	printf("Child processes have completed.\n");
    printf("Parent is now ending.\n");
    shmdt(clock);
    kill_all_processes(processTable);
    return 0;
}

void launch_child(PCB processTable[], int time_limit, int simultaneous, Clock* clock){
    string rand_secs = std::to_string(generate_random_number(1, (time_limit - 1)));
    string rand_nanos = std::to_string(generate_random_number(0, 999999999));
    // string user_parameters = std::to_string(rand_secs) + " " + std::to_string(rand_nanos); 

    pid_t childPid = fork(); // This is where the child process splits from the parent        
    if (childPid == 0) {            // Each child uses exec to run ./user	
        // execl("./user", "user", user_parameters.c_str(), NULL);    
        char* args[] = {const_cast<char*>("./user"), const_cast<char*>(rand_secs.c_str()), const_cast<char*>(rand_nanos.c_str()), nullptr};
        execvp(args[0], args);
        perror("Error: Failed to execute user program");
        exit(EXIT_FAILURE);
    } else if (childPid == -1) {  // Fork failed
        perror("Error: Fork has failed");
        exit(EXIT_FAILURE);
    } else {            // Parent updates Process Table with child info after fork()
        int i = (process_table_vacancy(processTable, simultaneous) - 1);
        processTable[i].occupied = 1;
        processTable[i].pid = childPid;
        processTable[i].startSecs = clock->secs;
        processTable[i].startNanos = clock->nanos;
    }
}

int generate_random_number(int min, int max) {  // pseudo rng for random child workload
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::default_random_engine generator(seed);
    std::uniform_int_distribution<int> distribution(min, max);
    int random_number = distribution(generator);
    return random_number;
}

bool launch_interval_satisfied(int launch_interval, Clock* clock){
    static int last_launch_secs = 0;  // static ints used to keep track of 
    static int last_launch_nanos = 0;   // most recent process launch

    int elapsed_secs = clock->secs - last_launch_secs; 
    int elapsed_nanos = clock->nanos - last_launch_nanos;

    while (elapsed_nanos < 0) {   // fix if subtracted time is too low
        elapsed_secs--;
        elapsed_nanos += 1000000000;
    }

    if (elapsed_secs > 0 || (elapsed_secs == 0 && elapsed_nanos >= launch_interval)) {        
        last_launch_secs = clock->secs;  // Update the last launch time
        last_launch_nanos = clock->nanos;        
        return true;
    } else {
        return false;
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

void timeout_handler(int signum) {
    std::cout << "Timeout occurred. Cleaning up before exiting..." << std::endl;
    term = 1;
    kill_all_processes(processTable);
    // shmdt(clock);  // detatch from shared memory
    std::exit(EXIT_SUCCESS);
}

// Signal handler for Ctrl+C (SIGINT)
void ctrl_c_handler(int signum) {
    std::cout << "Ctrl+C detected. Cleaning up before exiting..." << std::endl;
    kill_all_processes(processTable);
    // shmdt(clock);
    std::exit(EXIT_SUCCESS);
}