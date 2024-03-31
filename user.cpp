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
#include "resources.h"
using namespace std;

#define TERMINATION_CHANCE 5     // 5% chance
#define IO_BLOCK_CHANCE 15
#define ACUTAL_IO_BLOCK_CHANCE (TERMINATION_CHANCE + IO_BLOCK_CHANCE)

// #define TERMINATION_CHANCE 5
#define R_REQUEST_CHANCE 75
#define R_RELEASE_CHANCE 20

void calculate_time_until_unblocked(int, int, int, int *, int *);
bool will_process_terminate_during_quantum(int, int, int, int, int, int, int, int*);

int main(int argc, char** argv) {
    Clock* shm_clock;           // init shm clock
	key_t clock_key = ftok("/tmp", 35);
	int shmtid = shmget(clock_key, sizeof(Clock), 0666);
	shm_clock = (Clock*)shmat(shmtid, NULL, 0);

    int start_secs = shm_clock->secs;  // start time is current time at the start
    int start_nanos = shm_clock->nanos; // Is this our issue leaving child in sys too long?
  
    int secs = atoi(argv[1]);     // Arg 1 will be secs, secs given from start to terminate
    int nanos = atoi(argv[2]);   // Arg 2 will be nanos  
    
    int end_secs = start_secs + secs;   // end time is starting time plus time told to wait
    int end_nanos = start_nanos + nanos;  
    if (end_nanos >= 1000000000){   // if over 1 billion nanos, add 1 second, sub 1 bil nanos
        end_nanos = end_nanos - 1000000000;
        end_secs++;
    } 

    msgbuffer buf, rcvbuf;   // buf for msgsnd buffer, rcvbuf for msgrcv buffer	
    buf.mtype = getppid();
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
    printf("USER PID: %d  PPID: %d  SysClockS: %d  SysClockNano: %d  TermTimeS: %d  TermTimeNano: %d\n--Just Starting\n", getpid(), getppid(), shm_clock->secs, shm_clock->nanos, end_secs, end_nanos); 

    int iter = 0;
    bool done = false;

    while(!done){      // ----------- MAIN LOOP -----------     
        iter++;       // MSGRCV BLOCKING WAIT, WAITS HERE WHILE IO BLOCKED ALSO              
        if ( msgrcv(msgqid, &rcvbuf, sizeof(msgbuffer), getpid(), 0) == -1) {
            perror("failed to receive message from parent\n");
            exit(1);
        }       // MSGRCV PRINT MSG	
        printf("%d: Child dispatched by parent and given quantum %d\n",getpid(), rcvbuf.time_slice);
          
        int random_number = generate_random_number(1, 100, getpid());

                    // IF WE RANDOMLY TERM EARLY
        if (random_number < TERMINATION_CHANCE){   
            printf("USER PID: %d  PPID: %d  SysClockS: %d  SysClockNano: %d  TermTimeS: %d  TermTimeNano: %d\n--Terminating after sending message back to oss after %d iterations.\n", getpid(), getppid(), shm_clock->secs, shm_clock->nanos, end_secs, end_nanos, iter);
            done = true;
            buf.msgCode = MSG_TYPE_SUCCESS;                
            buf.time_slice = generate_random_number(1, 100000000, getpid()) % rcvbuf.time_slice;  // use random amount of timeslice before terminating            
            strcpy(buf.message,"Completed Successfully (RANDOM TERMINATION), now terminating...\n");
                    // IF WE RANDOMLY IO BLOCK
        } else if (random_number < ACUTAL_IO_BLOCK_CHANCE){                        
            int nanos_blocked = 1000000000; // IO BLOCK ALWAYS LASTS 1 SEC
            int temp_unblock_secs, temp_unblock_nanos;
            calculate_time_until_unblocked(shm_clock->secs, shm_clock->nanos, nanos_blocked, &temp_unblock_secs, &temp_unblock_nanos);
            buf.blocked_until_secs = temp_unblock_secs;
            buf.blocked_until_nanos = temp_unblock_nanos;
            buf.msgCode = MSG_TYPE_BLOCKED;   
            buf.time_slice = generate_random_number(1, 100000000, getpid()) % rcvbuf.time_slice;  // use random amount of timeslice before IO Block
            strcpy(buf.message,"IO BLOCKED!!!...\n");
                    // IF WE NEITHER TERM EARLY OR IO BLOCK
        } else { // THEN CHECK IF TERMINATION TIME HAS PASSED
            int secs_plus_quantum, nanos_plus_quantum, time_slice_used = rcvbuf.time_slice;
            if(shm_clock->secs > end_secs || shm_clock->secs == end_secs && shm_clock->nanos > end_nanos){ 
                printf("USER PID: %d  PPID: %d  SysClockS: %d  SysClockNano: %d  TermTimeS: %d  TermTimeNano: %d\n--Terminating after sending message back to oss after %d iterations.\n", getpid(), getppid(), shm_clock->secs, shm_clock->nanos, end_secs, end_nanos, iter);
                done = true;
                buf.msgCode = MSG_TYPE_SUCCESS;
                buf.time_slice = 0;  // TIME SLICE NOT USED, ENDED BEFORE SEEN BY CPU
                strcpy(buf.message,"Completed Successfully (END TIME ELAPSED BEFORE RUNTIME), now terminating...\n");
                    // ELSE WILL IT TERMINATE DURING THIS QUANTUM
            } else if(will_process_terminate_during_quantum(shm_clock->secs, shm_clock->nanos, end_secs, end_nanos, rcvbuf.time_slice, secs_plus_quantum, nanos_plus_quantum, &time_slice_used)){
                printf("USER PID: %d  PPID: %d  SysClockS: %d  SysClockNano: %d  TermTimeS: %d  TermTimeNano: %d\n--Terminating after sending message back to oss after %d iterations.\n", getpid(), getppid(), shm_clock->secs, shm_clock->nanos, end_secs, end_nanos, iter);
                done = true;
                buf.msgCode = MSG_TYPE_SUCCESS;
                buf.time_slice = time_slice_used;
                strcpy(buf.message,"Completed Successfully (END TIME ELAPSED DURING RUNTIME), now terminating...\n");
            } else {    // ELSE TERMINATION WILL NOT OCCUR IN THIS QUANTUM 
                printf("USER PID: %d  PPID: %d  SysClockS: %d  SysClockNano: %d  TermTimeS: %d  TermTimeNano: %d\n--%d iteration(s) have passed since starting\n", getpid(), getppid(), shm_clock->secs, shm_clock->nanos, end_secs, end_nanos, iter);
                buf.msgCode = MSG_TYPE_RUNNING;
                buf.time_slice = rcvbuf.time_slice;  // used full time slice
                strcpy(buf.message,"Still Running...\n");
            }
        }        
            // msgsnd(to parent saying if we are done or not);        
            // blocking wait to prevent rare race condition bug (occurs at -n > 1000) in which oss schedules program from scheduling queue not in the process table
        if (msgsnd(msgqid, &buf, sizeof(msgbuffer), 1) == -1) {
            perror("msgsnd to parent failed\n");
            exit(1);
        }
    }   // ----------- MAIN LOOP -----------     

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
         // Try using this if the above if is too dangerous, with how short our quantums are,
         // this will suffice for all of the cases we will see.
    // if(elapsed_secs == -1){   
    //     *time_slice_used = abs(1000000000 + elapsed_nanos);
    //     return true;
    // }

        // else program will not end during next quantum
    return false;
}