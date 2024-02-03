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
    int iterations = atoi(argv[1]);
    for(int i = 1; i < iterations + 1; i++){
        printf("USER PID: %d  PPID: %d  Iteration: %d\n", getpid(), getppid(), i);       
    }    

    sleep(3);
    return EXIT_SUCCESS; 
}
