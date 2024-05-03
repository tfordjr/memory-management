// CS4760-001SS - Terry Ford Jr. - Project 6 Memory Management - 04/21/2024
// https://github.com/tfordjr/memory-management.git

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

#define TERMINATION_CHANCE 5  // 0.5% chance to terminate every loop
#define READ_CHANCE 70
// This means write chance is 30%

int main(int argc, char** argv) {
    Clock* shm_clock;           // init shm clock
	key_t clock_key = ftok("/tmp", 35);
	int shmtid = shmget(clock_key, sizeof(Clock), 0666);
	shm_clock = (Clock*)shmat(shmtid, NULL, 0);
        
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

    bool done = false;
    msgbuffer buf, rcvbuf;   // buf for msgsnd buffer, rcvbuf for msgrcv buffer	
    buf.mtype = getppid();              
    buf.sender = getpid();        
    // add_time(&nextSecs, &nextNanos, randomInterval); // next user request/release event

    while(!done){ // ---------- MAIN LOOP ----------             
        if (TERMINATION_CHANCE > generate_random_number(0, 1000, getpid())){
            std::cout << "Child " << getpid() << " randomly terminating..." << std::endl;
            exit(1);
        }
                // generating memoryAddress to read or write to        
        int pageNumber = generate_random_number(0, 63, getpid());
        int offset = generate_random_number(0, 1023, getpid());
        buf.memoryAddress = (pageNumber * 1024) + offset;

        if(READ_CHANCE > generate_random_number(1, 100, getpid())){  // REQUEST
            buf.msgCode = MSG_TYPE_READ; 
        } else {                                                     // WRITE
            buf.msgCode = MSG_TYPE_WRITE;
        }   // MSGSND REQUEST/RELEASE TO OSS     
        if(msgsnd(msgqid, &buf, sizeof(msgbuffer), 1) == -1) { 
            perror("msgsnd to parent failed\n");
            exit(1);
        }
            // MSGRCV BLOCKING WAIT FOR RESPONSE TO READ/WRITE REQUEST
        if(msgrcv(msgqid, &rcvbuf, sizeof(msgbuffer), getpid(), 0) == -1) {
            perror("failed to receive message from parent\n");
            exit(1);
        }   // IF BLOCKED (PAGEFAULT) MSG RECEIVED, WAIT FOR UNBLOCK MSG FROM OSS
        if(rcvbuf.msgCode == MSG_TYPE_BLOCKED){                            
            if(msgrcv(msgqid, &rcvbuf, sizeof(msgbuffer), getpid(), 0) == -1) {
                perror("failed to receive message from parent\n");
                exit(1);
            }
            if(rcvbuf.msgCode != MSG_TYPE_GRANTED){
                perror("user.cpp: child process unblocked but msgCode was not MSG_TYPE_GRANTED");
                exit(1);
            } // If resource is granted, resume execution. OSS handles Allocation
        }            
    }   // ---------------------- MAIN LOOP ----------------------
    shmdt(shm_clock);  // deallocate shm and terminate
    printf("%d: Child is terminating...\n",getpid());
    return EXIT_SUCCESS; 
}