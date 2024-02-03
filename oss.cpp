#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <cstring>
using namespace std;


void help();
int forkandwait(int, int);

int main(int argc, char** argv){
    int option;
    int numChildren;
    int simultaneous;
    int iterations;

    while ( (option = getopt(argc, argv, "hn:s:t:")) != -1) {  
        switch(option) {
            case 'h':
                help();
                return 0;     // terminates if -h is present
            case 'n':                    
                for (int i = 1; i < argc; i++) {    // cycles through args 
                    if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
                        numChildren = atoi(argv[i + 1]);  
                        break;
                    }
                }	
                break;
            case 's':
                for (int i = 1; i < argc; i++) {  // cycles through args 
                    if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
                        simultaneous = atoi(argv[i + 1]);
                        break;
                    } 
                }
            case 't':
                for (int i = 1; i < argc; i++) {  // cycles through args 
                    if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
                        iterations = atoi(argv[i + 1]);
                        break;
                    } 
                }
        }
	}   

    printf("Number of Children: %d\n", numChildren);
    printf("Number of Simultaneous: %d\n", simultaneous);
    printf("Number of Iterations: %d\n", iterations);

    forkandwait(numChildren, iterations);  

    // std::cout << "Number of Children: " + numChildren << std::endl;
    // std::cout << "Number of Simultaneous: " + simultaneous << std::endl;
    // std::cout << "Number of Iterations: " + iterations << std::endl;
}

int forkandwait(int numChildren, int iterations) {    
    for (int i = 0; i < numChildren; i++) {

        char iterations_as_string[20];
        sprintf(iterations_as_string, "%d", iterations);

        pid_t childPid = fork(); // This is where the child process splits from the parent

        // if (childPid == 0) {        
        //     static char *args[] = { "./user", (char *)iterations, NULL };
        //     execv(args[0], args);
        //     fprintf(stderr, "Failed to execute %s\n", args[0]);
        //     exit(EXIT_FAILURE);
        // } else {
        //     printf("I'm a parent! My pid is %d, and my child's pid is %d \n",
        //     getpid(), childPid);    
        //     sleep(1); // wait(0);  
        // }

        if (childPid == 0 ) {             // Each child uses exec to run ./user	
		 	// if(execl("./user", "user", (char *)iterations, (char *)NULL) == -1) {   
			// 	perror("Exec failed.\n");				
			// }	
			// exit(0);
            execl("./user", "user", iterations.cstr(), NULL);
				
		} else 	if (childPid == -1) {  // Error message for failed fork (child has PID -1)
            perror("master: Error: Fork has failed!");
            exit(0);
        }       
		wait(NULL);  // Parent waits to assure children perform in order
    }

    for (int i = 0; i < numChildren; i++) { 
        wait(NULL);	// Parent Waiting for children
    }

	printf("Child processes have completed.\n");
    printf("Parent is now ending.\n");
    return EXIT_SUCCESS;
}

void help(){   // Help message here
    printf("-h detecteed. Printing Help Message...\n");
    printf("The options for this program are: \n");
    printf("\t-h Help feature. This takes no arguments.\n");
    printf("\t-i The argument following -i will be the input file. Optional - defaults to input.dat.\n");
    printf("\t-o The argument following -o will be the output file. Optional - defaults to output.dat.\n");
}
