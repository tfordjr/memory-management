// CS4760-001SS - Terry Ford Jr. - Project 2 Process Tables - 02/12/2024
// https://github.com/tfordjr/process-tables

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <cstring>
using namespace std;

int main(int argc, char** argv) {
    int iterations = atoi(argv[1]);   // only arg provided will be number of iterations
    for(int i = 1; i < iterations + 1; i++){
        printf("USER PID: %d  PPID: %d  Iteration: %d before sleeping\n", getpid(), getppid(), i);  
        sleep(1);
        printf("USER PID: %d  PPID: %d  Iteration: %d after sleeping\n" , getpid(), getppid(), i);  
    }        
    return EXIT_SUCCESS; 
}
