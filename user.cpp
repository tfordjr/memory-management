// CS4760-001SS - Terry Ford Jr. - Project 3 Message Queues - 02/29/2024
// https://github.com/tfordjr/message-queues.git

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
using namespace std;

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

    msgbuffer buf, rcvbuf;   // init msg buffer
	buf.address = getpid();
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
	printf("Child %d has access to the queue\n",getpid());   // starting messages    
    printf("USER PID: %d  PPID: %d  SysClockS: %d  SysClockNano: %d  TermTimeS: %d  TermTimeNano: %d\n--Just Starting\n", getpid(), getppid(), shm_clock->secs, shm_clock->nanos, end_secs, end_nanos); 

    int iter = 0;
    bool done = false;
    while(!done){                 // Blocking msgrcv waiting for parent message
        iter++;
        if ( msgrcv(msgqid, &rcvbuf, sizeof(msgbuffer), getpid(), 0) == -1) {  
            perror("failed to receive message from parent\n");
            exit(1);
        } // output message from parent	
        printf("Child %d received message code: %d msg: %s\n",getpid(), rcvbuf.msgCode, rcvbuf.message);

            // check if end time has elapsed, if so, terminate     
        if(shm_clock->secs > end_secs || shm_clock->secs == end_secs && shm_clock->nanos > end_nanos){ 
            printf("USER PID: %d  PPID: %d  SysClockS: %d  SysClockNano: %d  TermTimeS: %d  TermTimeNano: %d\n--Terminating after sending message back to oss after %d iterations.\n", getpid(), getppid(), shm_clock->secs, shm_clock->nanos, end_secs, end_nanos, iter);
            done = true;
            buf.msgCode = MSG_TYPE_SUCCESS;    
            strcpy(buf.message,"Completed Successfully, now terminating...\n");
        } else {    // else program continues running
            printf("USER PID: %d  PPID: %d  SysClockS: %d  SysClockNano: %d  TermTimeS: %d  TermTimeNano: %d\n--%d iteration(s) have passed since starting\n", getpid(), getppid(), shm_clock->secs, shm_clock->nanos, end_secs, end_nanos, iter);
            buf.msgCode = MSG_TYPE_RUNNING;
            strcpy(buf.message,"Still Running...\n");
        }      
            // msgsnd(to parent saying if we are done or not);      
        if (msgsnd(msgqid, &buf, sizeof(msgbuffer), 0) == -1) {
            perror("msgsnd to parent failed\n");
            exit(1);
        }
    }    
    shmdt(shm_clock);
    printf("Child %d is ending\n",getpid());
    return EXIT_SUCCESS; 
}
