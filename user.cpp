#include<unistd.h>
#include<sys/types.h>
#include<stdio.h>
#include<stdlib.h>
using namespace std;

int main(int argc, char** argv) {

    int iterations = atoi(argv[1]);
    for(int i = 1; i < iterations + 1; i++){
        printf("USER PID: %d ", getpid());
        printf("PPID: %d ", getppid());
        printf("Iteration: %d\n", i);
    }    

    // printf("Hello from Child.c, a new executable!\n");
    // printf("My process id is: %d\n",getpid());
    // printf(" I got %d arguments: \n", argc);
    // int i;
    // for (i = 0; i < argc; i++)
    //     printf("|%s| ", argv[i]);
    // printf("\nChild is now ending.\n");

    sleep(3);
    return EXIT_SUCCESS; 
}
