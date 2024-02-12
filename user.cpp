// CS4760-001SS - Terry Ford Jr. - Project 2 Process Tables - 02/12/2024
// https://github.com/tfordjr/process-tables

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <cstring>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include "pcb.h"
#include "shm.h"
using namespace std;

int main(int argc, char** argv) {

    Clock* clock;           // init shm clock
	key_t key = ftok("/tmp", 35);
	int shmtid = shmget(key, sizeof(Clock), 0666);
	clock = shmat(shmtid, NULL, 0);



    int iterations = atoi(argv[1]);   // only arg provided will be number of iterations
    for(int i = 1; i < iterations + 1; i++){
        printf("USER PID: %d  PPID: %d  Iteration: %d before sleeping\n", getpid(), getppid(), i);  
        sleep(1);
        printf("USER PID: %d  PPID: %d  Iteration: %d after sleeping\n" , getpid(), getppid(), i);  
    }        
    return EXIT_SUCCESS; 
}
