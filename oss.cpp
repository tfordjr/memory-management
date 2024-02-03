// CS4760-001SS - Terry Ford Jr. - Project 1 Processes - 01/28/2024
// https://github.com/tfordjr/multiple-processes

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
using namespace std;

void help();
int fork_and_wait(int, int, int);

int main(int argc, char** argv){
    int option, numChildren, simultaneous, iterations;    
    while ( (option = getopt(argc, argv, "hn:s:t:")) != -1) {   // getopt implementation
        switch(option) {
            case 'h':
                help();
                return 0;     // terminates if -h is present
            case 'n':                                
                numChildren = (optarg == NULL || atoi(optarg) > 20) ? 1 : atoi(optarg);
                if (numChildren > 20 || numChildren < 0 || !numChildren)
                    numChildren = 1;
                break;
            case 's':          
                simultaneous = (optarg == NULL || atoi(optarg) > 20) ? 1 : atoi(optarg);
                if (simultaneous > 20 || simultaneous < 0 || !simultaneous)
                    simultaneous = 1;
                break;
            case 't':
                iterations = (optarg == NULL || atoi(optarg) > 20) ? 1 : atoi(optarg);
                if (iterations > 20 || iterations < 0 || !iterations)
                    iterations = 1;
                break;            
        }
	}   // getopt loop completed here

    printf("Number of Children: %d\nNumber of Simultaneous: %d\nNumber of Iterations: %d\n", numChildren, simultaneous, iterations);
    fork_and_wait(numChildren, simultaneous, iterations);  
    return 0;
}

int fork_and_wait(int numChildren, int simultaneous, int iterations) {    
    int running = 0;  // count of current number of running processes
    for (int i = 0; i < numChildren; i++) {
        pid_t childPid = fork(); // This is where the child process splits from the parent
        
        if (childPid == 0 ) {             // Each child uses exec to run ./user	
		 	// static char *args[] = { "./user", (char *)iterations, NULL };
            // execv(args[0], args);
            execl("./user", "user", (std::to_string(iterations)).c_str(), NULL);            
            fprintf(stderr, "Failed to execute \n");      // IF child makes it 
            exit(EXIT_FAILURE);                          // this far exec did not work				
		} else 	if (childPid == -1) {  // Error message for failed fork (child has PID -1)
            perror("master: Error: Fork has failed!");
            exit(0);
        }       
        running++;  

        if(running >= simultaneous){ //If number of currently running processes at max number
            wait(NULL);  // Parent waits to assure children perform in order
            running--;
        } 
    }

    for (int i = 0; i < numChildren; i++) 
        wait(NULL);	// Parent Waiting for children    

	printf("Child processes have completed.\n");
    printf("Parent is now ending.\n");
    return EXIT_SUCCESS;
}

void help(){   // Help message here
    printf("-h detected. Printing Help Message...\n");
    printf("The options for this program are: \n");
    printf("\t-h Help will halt execution, print help message, and take no arguments.\n");
    printf("\t-n The argument following -n will be number of total processes to be run.\n");
    printf("\t-s The argument following -s will be max number of processes to be run simultaneously\n");
    printf("\t-t The argument following -t will be number of iterations each process will perform.\n");
}