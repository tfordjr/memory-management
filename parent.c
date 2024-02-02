#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>

void help();
void sigCatch(int);
void timeout(int);
void logfile();
int forkandwait(int);

int main(int argc, char** argv){
    int option;
    int numChildren;
    int simultaneous;
    int iterations;

    while ( (option = getopt(argc, argv, "hn:s:t:")) != -1) {  // getopt h no args, i.o req args
        switch(option) {
            case 'h':
                help();
                return 0;     // terminates if -h is present
            case 'n':                    
                for (int i = 1; i < argc; i++) {    // cycles through args 
                    if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
                        numChildren = argv[i + 1];  // assigns arg follwing -i to string variable infile
                            break;
                        }
                }	
                break;
            case 's':
                for (int i = 1; i < argc; i++) {  // cycles through args 
                    if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
                        simultaneous = argv[i + 1];
                        break;
                    } 
                }
            case 't':
                for (int i = 1; i < argc; i++) {  // cycles through args 
                    if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
                        iterations = argv[i + 1];
                        break;
                    } 
                }
        }
	}

    forkandwait(numChildren);
    printf("Number of Children: %d\n", numChildren);
    printf("Number of Simultaneous: %d\n", simultaneous);
    printf("Number of Iterations: %d\n", iterations);
}

int forkandwait(int numChildren) {    
    pid_t childPid = fork(); // This is where the child process splits from the parent

    if (childPid == 0) {
        printf("I am a child but a copy of parent! My parent's PID is %d, and my PID is %d\n",
            getppid(), getpid());
        char* args[] = {"./child", "Hello", "there", "exec", "is", "neat", 0};
            //execvp(args[0], args);
        execlp(args[0],args[0],args[1],args[2],args[3],args[4],args[5],args[6]);
        fprintf(stderr,"Exec failed, terminating\n");
        exit(1);
    } else {
        printf("I'm a parent! My pid is %d, and my child's pid is %d \n",
        getpid(), childPid);    
        wait(0);  //sleep(1);
    }

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
