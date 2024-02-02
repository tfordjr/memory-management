#include<unistd.h>
#include<sys/types.h>
#include<stdio.h>
#include<stdlib.h>

int main(int argc, char** argv) {

    printf("%d\n", argv[0]);

    printf("Hello from Child.c, a new executable!\n");
    printf("My process id is: %d\n",getpid());
    printf(" I got %d arguments: \n", argc);
    int i;
    for (i = 0; i < argc; i++)
        printf("|%s| ", argv[i]);
    printf("\nChild is now ending.\n");
    sleep(3);
    return EXIT_SUCCESS; 
}
