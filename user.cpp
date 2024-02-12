// CS4760-001SS - Terry Ford Jr. - Project 2 Process Tables - 02/12/2024
// https://github.com/tfordjr/process-tables

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <cstring>
#include <stdbool.h>
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

int main(int argc, char** argv) {

    Clock* clock;           // init shm clock
	key_t key = ftok("/tmp", 35);
	int shmtid = shmget(key, sizeof(Clock), 0666);
	clock = (Clock*)shmat(shmtid, NULL, 0);
  
    int secs = atoi(argv[1]);     // Arg 1 will be seconds
    int nanos = atoi(argv[2]);   // Arg 2 will be nanoseconds

    int start_secs = clock->secs;
    int start_nanos = clock->nanos;

    int recent_secs = clock->secs;
    int recent_nanos = clock->nanos;

    int end_secs = clock->secs + secs;   
    int end_nanos = clock->nanos + nanos;  
    if (end_nanos > 1000000000){   // if over 1 billion nanos, add 1 second, sub 1 bil nanos
        end_nanos = end_nanos - 1000000000;
        end_secs++;
    }          

    printf("USER PID: %d  PPID: %d  SysClockS: %d  SysClockNano: %d  TermTimeS: %d  TermTimeNano: %d\n--Just Starting\n", getpid(), getppid(), clock->secs, clock->nanos, end_secs, end_nanos); 

    bool done = false;
    while(!done){
        if (recent_secs != clock->secs || recent_nanos != clock->nanos){
            if(clock->secs > end_secs || clock->secs == end_secs && clock->nanos > end_nanos){
                printf("USER PID: %d  PPID: %d  SysClockS: %d  SysClockNano: %d  TermTimeS: %d  TermTimeNano: %d\n--Terminating\n", getpid(), getppid(), clock->secs, clock->nanos, end_secs, end_nanos);
                done = true;
            } else {
                printf("USER PID: %d  PPID: %d  SysClockS: %d  SysClockNano: %d  TermTimeS: %d  TermTimeNano: %d\n--%d seconds have passed since starting\n", getpid(), getppid(), clock->secs, clock->nanos, end_secs, end_nanos, (clock->secs - start_secs));
                int recent_secs = clock->secs;
                int recent_nanos = clock->nanos;
            }            
        }
    }
    return EXIT_SUCCESS;     
}
