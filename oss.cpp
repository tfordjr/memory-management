// CS4760-001SS - Terry Ford Jr. - Project 4 OSS Scheduler - 03/08/2024
// https://github.com/tfordjr/oss-scheduler.git

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <csignal>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include <sys/mman.h>
#include <fstream>
#include "pcb.h"
#include "clock.h"
#include "msgq.h"
#include "scheduler.h"
#include "rng.h"
using namespace std;

void launch_child(PCB[], int, int);
bool launch_interval_satisfied(int);
void help();
void timeout_handler(int);
void ctrl_c_handler(int);
void cleanup(std::string);
void output_statistics(int, double, double, double, double);

volatile sig_atomic_t term = 0;  // signal handling global
struct PCB processTable[20]; // Init Process Table Array of PCB structs (not shm)

// Declaring globals needed for signal handlers to clean up at anytime
Clock* shm_clock;  // Declare global shm clock
key_t clock_key = ftok("/tmp", 35);             
int shmtid = shmget(clock_key, sizeof(Clock), IPC_CREAT | 0666);    // init shm clock
std::ofstream outputFile;   // init file object
int msgqid;           // MSGQID GLOBAL FOR MSGQ CLEANUP
int simultaneous = 1;  // simultaneous global so that sighandlers know PCB table size to avoid segfaults when killing all procs on PCB
 
int main(int argc, char** argv){
    int option, numChildren = 1, time_limit = 2, launch_interval = 100;      
    int totalChildren; // used for statistics metrics report
    double totalSystemTime, totalBlockedTime = 0, totalCPUTime = 0, totalWaitTime = 0; // used for statistics report
    string logfile = "logfile.txt";
    while ( (option = getopt(argc, argv, "hn:s:t:i:f:")) != -1) {   // getopt implementation
        switch(option) {
            case 'h':
                help();
                return 0;     // terminates if -h is present
            case 'n':                                
                numChildren = atoi(optarg);
                totalChildren = numChildren;
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
            case 'f':
                logfile = optarg;
                break;
        }
	}   // getopt loop completed here

    std::signal(SIGALRM, timeout_handler);  // init signal handlers 
    std::signal(SIGINT, ctrl_c_handler);
    alarm(60);   // timeout timer
          
    init_process_table(processTable);      // init local process table
    shm_clock = (Clock*)shmat(shmtid, NULL, 0);    // attatch to global clock
    shm_clock->secs = 0;                        // init clock to 00:00
    shm_clock->nanos = 0;         
    
    outputFile.open(logfile); // This will create or overwrite the file "example.txt"    
    if (!outputFile.is_open()) {
        std::cerr << "Error: logfile didn't open" << std::endl;
        return 1; // Exit with error
    }
    
    msgbuffer buf;     //  INITIALIZE MESSAGE QUEUE	  (MSGQID MOVED TO GLOBAL)
	key_t msgq_key;
	system("touch msgq.txt");
	if ((msgq_key = ftok(MSGQ_FILE_PATH, MSGQ_PROJ_ID)) == -1) {   // get a key for our message queue
		perror("ftok");
		exit(1);
	}	
	if ((msgqid = msgget(msgq_key, PERMS | IPC_CREAT)) == -1) {  // create our message queue
		perror("msgget in parent");
		exit(1);
	}
	cout << "OSS: Message queue set up\n";
    outputFile << "OSS: Message queue set up\n";

    int i;          // holds PCB location of next process
    int time_slice; // holds time slice of next process in ns, updated by scheduler()
    int unblocks;   // holds number of unblocks performed to simulate scheduling overhead
                        //  ---------  MAIN LOOP  ---------   
    while(numChildren > 0 || !process_table_empty(processTable, simultaneous)){         
        scheduler(processTable, simultaneous, &i, &time_slice, &unblocks, shm_clock->secs, shm_clock->nanos); // assigns i to next child
        increment(shm_clock, (DISPATCH_AMOUNT + (unblocks * UNBLOCK_AMOUNT)));  // dispatcher overhead and unblocked reschedule overhead
        print_process_table(processTable, simultaneous, shm_clock->secs, shm_clock->nanos, outputFile);        
                  
                // MSG SEND
        if (!process_table_empty(processTable, simultaneous) && i != -1){  // comm with next child                         
            buf.mtype = processTable[i].pid;     // SEND MESSAGE TO CHILD NONBLOCKING
            buf.time_slice = time_slice;    // set local variable to msg struct parameter!
            buf.msgCode = MSG_TYPE_RUNNING;   // we will give it the pid we are sending to, so we know it received it
            buf.blocked_until_secs = 0;
            buf.blocked_until_nanos = 0;
            strcpy(buf.message, "Message to child\n");
            printf("%d: PARENT SENDING message code: %d and given quantum %d\n",getpid(), buf.msgCode, buf.time_slice);
            if (msgsnd(msgqid, &buf, sizeof(msgbuffer), 0) == -1) {
                perror(("oss.cpp: Error: msgsnd to child " + to_string(i + 1) + " failed\n").c_str());
                cleanup("perror encountered.");
                exit(1);
            }       // LOG MSG SEND
            cout << "OSS: Sending message code "<< buf.msgCode << " to worker " << i + 1 << " PID: " << processTable[i].pid << " at time " << shm_clock->secs << ":" << shm_clock->nanos << std::endl;
            outputFile << "OSS: Sending message code "<< buf.msgCode << " to worker " << i + 1 << " PID: " << processTable[i].pid << " at time " << shm_clock->secs << ":" << shm_clock->nanos << std::endl;

                    // MSG RECEIVE
            msgbuffer rcvbuf;     // BLOCKING WAIT TO RECEIVE MESSAGE FROM CHILD
            if (msgrcv(msgqid, &rcvbuf, sizeof(msgbuffer), getpid(), 0) == -1) {
                perror("oss.cpp: Error: failed to receive message in parent\n");
                cleanup("perror encountered.");
                exit(1);
            }       // LOG MSG RECEIVE
            cout << "OSS: Receiving message code " << rcvbuf.msgCode << " from worker " << i + 1 << " PID: " << processTable[i].pid << " at time " << shm_clock->secs << ":" << shm_clock->nanos << std::endl;
            outputFile << "OSS: Receiving message code " << rcvbuf.msgCode << " from worker " << i + 1 << " PID: " << processTable[i].pid << " at time " << shm_clock->secs << ":" << shm_clock->nanos << std::endl;
            increment(shm_clock, abs(rcvbuf.time_slice)); // increment absolute value of time used, sign only indicates process state, not time used

                    // UNPACK RECEIVED MESSAGE
            if (time_slice == rcvbuf.time_slice) { // If total time slice used
                descend_queues(processTable[i].pid); 
            } else if (rcvbuf.msgCode == MSG_TYPE_BLOCKED) {  // Process blocked
                remove_process_from_scheduling_queues(processTable[i].pid, processTable, simultaneous);
                update_process_table_of_blocked_child(processTable, processTable[i].pid, simultaneous, rcvbuf.blocked_until_secs, rcvbuf.blocked_until_nanos);                
            }else if(rcvbuf.msgCode == MSG_TYPE_SUCCESS){     // if child is terminating   
                cout << "OSS: Worker " << i + 1 << " PID: " << processTable[i].pid << " is planning to terminate" << std::endl;             
                outputFile << "OSS: Worker " << i + 1 << " PID: " << processTable[i].pid << " is planning to terminate" << std::endl;
                wait(0);  // give terminating process time to clear out of system
                remove_process_from_scheduling_queues(processTable[i].pid, processTable, simultaneous);
                update_process_table_of_terminated_child(processTable, processTable[i].pid, simultaneous);

                // WHEN PROCESS ENDS WE CAN LOG START AND END TIME OF THE PROCESS(TOTAL TIME IN SYSTEM)
                // THEN WE CAN (SUM OF TIME SPENT BLOCKED)/(TOTAL TIME IN SYSTEM) FOR AVG BLOCKED TIME
                // AND (SUM AVG BLOCKED TIME)/(NUMBER OF PROCESSES) = TOTAL AVERAGE BLOCKED TIME
            }
            // NEED TO LOG: totalSystemTime, totalBlockedTime, totalCPUTime, totalWaitTime
            // LOG rcvbuf.time_slice USED HERE FOR IDLE CPU TIME, BLOCKED TIME METRICS, ETC
            // ADDING ALL rcvbuf.time_slice = (TOTAL CPU TIME USED BY PROCESSES)            
        }         
                // CHECK IF CONDITIONS ARE RIGHT TO LAUNCH ANOTHER CHILD
        if(numChildren > 0 && launch_interval_satisfied(launch_interval)  // check conditions to launch child
        && process_table_vacancy(processTable, simultaneous)){ // child process launch check
            cout << "OSS: Launching Child Process..." << endl;
            outputFile << "OSS: Launching Child Process..." << endl;
            numChildren--;
            launch_child(processTable, time_limit, simultaneous);
        }        
    }                   // --------- END OF MAIN LOOP ---------  

    // output_statistics(totalChildren, totalSystemTime, totalBlockedTime, totalCPUTime, totalWaitTime);

	cout << "OSS: Child processes have completed. (" << numChildren << " remaining)\n";
    cout << "OSS: Parent is now ending.\n";
    outputFile << "OSS: Child processes have completed. (" << numChildren << " remaining)\n";
    outputFile << "OSS: Parent is now ending.\n";
    outputFile.close();  // file object close

    cleanup("OSS Success.");  // function to cleanup shm, msgq, and processes

    return 0;
}

void launch_child(PCB processTable[], int time_limit, int simultaneous){
    string rand_secs = std::to_string(generate_random_number(1, (time_limit - 1), getpid()));
    string rand_nanos = std::to_string(generate_random_number(0, 999999999, getpid()));
    // string user_parameters = std::to_string(rand_secs) + " " + std::to_string(rand_nanos); 

    pid_t childPid = fork(); // This is where the child process splits from the parent        
    if (childPid == 0) {            // Each child uses exec to run ./user	
        // execl("./user", "user", user_parameters.c_str(), NULL);    
        char* args[] = {const_cast<char*>("./user"), const_cast<char*>(rand_secs.c_str()), const_cast<char*>(rand_nanos.c_str()), nullptr};
        execvp(args[0], args);
        perror("oss.cpp: Error: Failed to execute user program");
        exit(EXIT_FAILURE);
    } else if (childPid == -1) {  // Fork failed
        perror("oss.cpp: Error: Fork has failed");
        exit(EXIT_FAILURE);
    } else {            // Parent updates Process Table with child info after fork()
        int i = (process_table_vacancy(processTable, simultaneous) - 1);
        processTable[i].occupied = 1;
        processTable[i].pid = childPid;
        processTable[i].startSecs = shm_clock->secs;
        processTable[i].startNanos = shm_clock->nanos;
        processTable[i].blocked = 0;
        processTable[i].eventBlockedUntilSec = 0;
        processTable[i].eventBlockedUntilNano = 0;
        queue_process(processTable[i].pid);
        increment(shm_clock, CHILD_LAUNCH_AMOUNT);  // child launch overhead simulated
    }
}

bool launch_interval_satisfied(int launch_interval){
    static int last_launch_secs = 0;  // static ints used to keep track of 
    static int last_launch_nanos = 0;   // most recent process launch

    int elapsed_secs = shm_clock->secs - last_launch_secs; 
    int elapsed_nanos = shm_clock->nanos - last_launch_nanos;

    while (elapsed_nanos < 0) {   // fix if subtracted time is too low
        elapsed_secs--;
        elapsed_nanos += 1000000000;
    }

    if (elapsed_secs > 0 || (elapsed_secs == 0 && elapsed_nanos >= launch_interval)) {        
        last_launch_secs = shm_clock->secs;  // Update the last launch time
        last_launch_nanos = shm_clock->nanos;        
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
    printf("\t-i The argument following -i will be launch interval between process launch in milliseconds.\n");
    printf("\t-i The argument following -f will be the logfile name (please include file extention)\n");
    printf("\t args will default to appropriate values if not provided.\n");
}

void timeout_handler(int signum) {
    cleanup("Timeout Occurred.");
}

// Signal handler for Ctrl+C (SIGINT)
void ctrl_c_handler(int signum) {    
    cleanup("Ctrl+C detected.");
}

void cleanup(string cause) {
    std::cout << cause << " Cleaning up before exiting..." << std::endl;
    outputFile << cause << " Cleaning up before exiting..." << std::endl;
    kill_all_processes(processTable, simultaneous);
    outputFile.close();  // file object close
    shmdt(shm_clock);       // clock cleanup, detatch & delete shm
    if (shmctl(shmtid, IPC_RMID, NULL) == -1) {
        perror("oss.cpp: Error: shmctl failed!!");
        exit(1);
    }            
    if (msgctl(msgqid, IPC_RMID, NULL) == -1) {  // get rid of message queue
		perror("oss.cpp: Error: msgctl to get rid of queue in parent failed");
		exit(1);
	}
    std::exit(EXIT_SUCCESS);
}

void output_statistics(int totalChildren, double totalBlockedTime, double totalCPUTime, double totalWaitTime){
    // int bufferSecs = 0;
    // int bufferNanos = 0;
    double totalSystemTime = shm_clock->secs + (shm_clock->nanos)/1000000000;

    std::cout << "Average Wait time: " << totalWaitTime/totalChildren << std::endl;      // sum of each process' total time in system - sum CPU time given - sum Blocked time
    std::cout << "Average CPU Utilization: " << totalCPUTime/totalChildren << endl;           // sum each process' CPU utilization each process got / totalChildren
    std::cout << "Average time a process waited in a blocked queue: " << totalBlockedTime/totalChildren << endl; // sum each Process' blocked time / totalChildren
    std::cout << "Total Idle CPU time: " << totalSystemTime - totalCPUTime << endl;
}

// Is wait time same as time spent 'ready' state meaning not total time in system minus blocked time minus given quantums?

// ASK ABOUT TEST FINAL GRADE