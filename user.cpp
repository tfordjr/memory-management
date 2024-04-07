// CS4760-001SS - Terry Ford Jr. - Project 5 Resource Management - 03/29/2024
// https://github.com/tfordjr/resource-management.git

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <cstring>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include <sys/mman.h>
#include <errno.h>
#include "clock.h"
#include "msgq.h"
#include "rng.h"
#include "pcb.h"
using namespace std;

#define TERMINATION_CHANCE 5
#define R_REQUEST_CHANCE 75
#define ACTUAL_REQUEST_CHANCE (TERMINATION_CHANCE + R_REQUEST_CHANCE)
      // Remaining (100 - (TERM_CHANCE + R_REQUEST_CHANCE)) = R_RELEASE_CHANCE
#define R_INTERVAL_BOUND 1e7  // max bound is 10 ms (in ns)

void calculate_time_until_unblocked(int, int, int, int *, int *);
bool will_process_terminate_during_quantum(int, int, int, int, int, int, int, int*);

int main(int argc, char** argv) {
    Clock* shm_clock;           // init shm clock
	key_t clock_key = ftok("/tmp", 35);
	int shmtid = shmget(clock_key, sizeof(Clock), 0666);
	shm_clock = (Clock*)shmat(shmtid, NULL, 0);

    int start_secs = shm_clock->secs;  // start time is current time at the start
    int start_nanos = shm_clock->nanos; // Is this our issue leaving child in sys too long?
        
	int msgqid = 0;
	key_t msgq_key;	
	if ((msgq_key = ftok(MSGQ_FILE_PATH, MSGQ_PROJ_ID)) == -1) {   // get a key for our message queue
		perror("ftok");
		exit(1);
	}	
	if ((msgqid = msgget(msgq_key, PERMS)) == -1) {  // create our message queue
		perror("msgget in child");
		exit(1);
	}
	printf("%d: Child has access to the msg queue\n",getpid());   // starting messages    
    printf("USER PID: %d  PPID: %d  SysClockS: %d  SysClockNano: %d \n--Just Starting\n", getpid(), getppid(), shm_clock->secs, shm_clock->nanos); 

    int iter = 0;
    bool done = false;
    msgbuffer buf, rcvbuf;   // buf for msgsnd buffer, rcvbuf for msgrcv buffer	
    buf.mtype = getppid();              
    int randomAction = generate_random_number(1, 100, getpid());    
    int randomInterval = generate_random_number(1, R_INTERVAL_BOUND, getpid());        
    int nextSecs = shm_clock->secs;
    int nextNanos = shm_clock->nanos;
    add_time(&nextSecs, &nextNanos, randomInterval); // next user request/release event

    while(!done){ // ---------- MAIN LOOP ---------- BUSY-WAIT LOOP FOR NEXT ACTION ---------    
        if(shm_clock->secs > nextSecs || shm_clock->secs == nextSecs && shm_clock->nanos > nextNanos){
            if (randomAction < TERMINATION_CHANCE){
                done = true;
                // buf.msgCode = MSG_TYPE_SUCCESS;  // NO MSG WHEN PROC TERMINATES
            } else {  // THIS CASE COULD BE RELEASE OR REQUEST, EITHER WAY WE SEND A MSG TO OSS                
                buf.resource = generate_random_number(0, (NUM_RESOURCES - 1), getpid()); // which resource will be requested/released
                if(randomAction < ACTUAL_REQUEST_CHANCE){  // REQUEST
                    buf.msgCode = MSG_TYPE_REQUEST; 
                } else {  // RELEASE
                    buf.msgCode = MSG_TYPE_RELEASE;
                }
                    // MSGSND REQUEST/RELEASE TO OSS
                if(msgsnd(msgqid, &buf, sizeof(msgbuffer), 1) == -1) { 
                    perror("msgsnd to parent failed\n");
                    exit(1);
                }
                if(buf.msgCode == MSG_TYPE_REQUEST){  // IF REQUEST WAS SENT TO OSS
                        // MSGRCV BLOCKING WAIT FOR RESPONSE TO RESOURCE REQUEST
                    if(msgrcv(msgqid, &rcvbuf, sizeof(msgbuffer), getpid(), 0) == -1) {
                        perror("failed to receive message from parent\n");
                        exit(1);
                    }                
                    if(rcvbuf.msgCode == MSG_TYPE_BLOCKED){    // YOU ARE IN BLOCKED QUEUE WAITING FOR RESOURCE                        
                        if(msgrcv(msgqid, &rcvbuf, sizeof(msgbuffer), getpid(), 0) == -1) {
                            perror("failed to receive message from parent\n");
                            exit(1);
                        }
                        if(rcvbuf.msgCode != MSG_TYPE_GRANTED){
                            perror("user.cpp: child process unblocked but msgCode was not MSG_TYPE_GRANTED");
                            exit(1);
                        } // If resource is granted, resume execution. OSS handles Allocation
                    }
                }
            }   // DETERMINE NEXT ACTION AND TIME NEXT ACTION WILL OCCUR
            randomAction = generate_random_number(1, 100, getpid());   
            randomInterval = generate_random_number(1, R_INTERVAL_BOUND, getpid());                 
            nextSecs = shm_clock->secs;
            nextNanos = shm_clock->nanos;
            add_time(&nextSecs, &nextNanos, randomInterval); // next user request/release event
        }
    }   // ---------------------- MAIN LOOP ----------------------

    shmdt(shm_clock);  // deallocate shm and terminate
    printf("%d: Child is terminating...\n",getpid());
    return EXIT_SUCCESS; 
}

void calculate_time_until_unblocked(int secs, int nanos, int nanos_blocked, int *temp_unblock_secs, int *temp_unblock_nanos){
    nanos += nanos_blocked;
    if (nanos >= 1000000000){   // if over 1 billion nanos, add 1 second, sub 1 bil nanos
        nanos -= 1000000000;
        secs++;
    }
    *temp_unblock_secs = secs;
    *temp_unblock_nanos = nanos;
    return;
}

bool will_process_terminate_during_quantum(int secs, int nanos, int end_secs, int end_nanos, int quantum, int secs_plus_quantum, int nanos_plus_quantum, int* time_slice_used){
    secs_plus_quantum = secs;
    nanos_plus_quantum = nanos;
    
    nanos_plus_quantum += quantum;

    if (nanos >= 1000000000){   // if over 1 billion nanos, add 1 second, sub 1 bil nanos
        nanos_plus_quantum -= 1000000000;
        secs_plus_quantum++;
    }
    
    int elapsed_secs = end_secs - secs_plus_quantum;  // difference between termination time and clock time
    int elapsed_nanos = end_nanos - nanos_plus_quantum;    
   
    if (elapsed_nanos < 0) {  
        elapsed_secs--;       
        elapsed_nanos += 1000000000;
    }
        // if difference is negative, process will end during quantum
    if(elapsed_secs < 0){
        *time_slice_used = abs(elapsed_secs * 1000000000 + elapsed_nanos);
        return true;
    }
    
    return false;
}