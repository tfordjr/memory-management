#include<unistd.h>
#include<sys/types.h>
#include<stdio.h>
#include<stdlib.h>

int main(int argc, char** argv) {
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
