// CS4760-001SS - Terry Ford Jr. - Project 3 Message Queues - 02/29/2024
// https://github.com/tfordjr/message-queues.git

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <csignal>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include <sys/mman.h>
#include <random>
#include <chrono>
#include <fstream>
#include "pcb.h"
#include "clock.h"
#include "msgq.h"
using namespace std;

void launch_child(PCB[], int, int);
int generate_random_number(int, int);
bool launch_interval_satisfied(int);
void help();
void timeout_handler(int);
void ctrl_c_handler(int);

volatile sig_atomic_t term = 0;  // signal handling global
struct PCB processTable[20]; // Init Process Table Array of PCB structs (not shm)

Clock* shm_clock;  // Declare global shm clock
key_t clock_key = ftok("/tmp", 35);             
int shmtid = shmget(clock_key, sizeof(Clock), IPC_CREAT | 0666);    // init shm clock
std::ofstream outputFile;   // init file object
int msgqid;           // MSGQID GLOBAL FOR MSGQ CLEANUP
 // Doing it up here because shmtid is needed to delete shm, needed for timeout/exit signal

int main(int argc, char** argv){
    int option, numChildren = 1, simultaneous = 1, time_limit = 2, launch_interval = 100;      
    string logfile = "logfile.txt";
    while ( (option = getopt(argc, argv, "hn:s:t:i:f:")) != -1) {   // getopt implementation
        switch(option) {
            case 'h':
                help();
                return 0;     // terminates if -h is present
            case 'n':                                
                numChildren = atoi(optarg);
                break;
            case 's':          
                simultaneous = atoi(optarg);
                break;
            case 't':
                time_limit = atoi(optarg);
                break; 
            case 'i':
                launch_interval = (1000000 * atoi(optarg));  // converting ms to nanos
                break;            
            case 'f':
                logfile = optarg;
                break;
        }
	}   // getopt loop completed here

    std::signal(SIGALRM, timeout_handler);  // init signal handlers 
    std::signal(SIGINT, ctrl_c_handler);
    alarm(60);   // timeout timer
          
    init_process_table(processTable);      // init local process table
    shm_clock = (Clock*)shmat(shmtid, NULL, 0);    // attatch to global clock
    shm_clock->secs = 0;                        // init clock to 00:00
    shm_clock->nanos = 0;         
    
    outputFile.open(logfile); // This will create or overwrite the file "example.txt"    
    if (!outputFile.is_open()) {
        std::cerr << "Error: logfile didn't open" << std::endl;
        return 1; // Exit with error
    }
    
    msgbuffer buf;     //  INITIALIZE MESSAGE QUEUE	  (MSGQID MOVED TO GLOBAL)
	key_t msgq_key;
	system("touch msgq.txt");
	if ((msgq_key = ftok(MSGQ_FILE_PATH, MSGQ_PROJ_ID)) == -1) {   // get a key for our message queue
		perror("ftok");
		exit(1);
	}	
	if ((msgqid = msgget(msgq_key, PERMS | IPC_CREAT)) == -1) {  // create our message queue
		perror("msgget in parent");
		exit(1);
	}
	printf("Message queue set up by OSS\n");

                        //  ---------  MAIN LOOP  ---------   
    while(numChildren > 0 || !process_table_empty(processTable, simultaneous)){ 
        increment(shm_clock, running_processes(processTable, simultaneous));
        print_process_table(processTable, simultaneous, shm_clock->secs, shm_clock->nanos, outputFile);        
      
            // FOR EACH PROCESS IN PCB, SEND A MESSAGE AND WAIT TO HEAR BACK!!!!!
        for (int i = 0; i < simultaneous; i++){
            if (processTable[i].occupied == 1){
                buf.address = processTable[i].pid;     // SEND MESSAGE TO CHILD
                buf.msgCode = MSG_TYPE_RUNNING;   // we will give it the pid we are sending to, so we know it received it
                strcpy(buf.message, "Message to child\n");
                if (msgsnd(msgqid, &buf, sizeof(msgbuffer), 0) == -1) {
                    perror("msgsnd to child 1 failed\n");
                    exit(1);
                }
                outputFile << "OSS: Sending message to worker " << i + 1 << " PID: " << buf.address << " at time " << shm_clock->secs << ":" << shm_clock->nanos << std::endl;

                msgbuffer rcvbuf;     // BLOCKING WAIT TO RECEIVE MESSAGE FROM CHILD
                if (msgrcv(msgqid, &rcvbuf, sizeof(msgbuffer), getpid(), 0) == -1) {
                    perror("failed to receive message in parent\n");
                    exit(1);
                }
                printf("Parent %d received message code: %d msg: %s\n",getpid(), buf.msgCode, buf.message);
                outputFile << "OSS: Receiving message from worker " << i + 1 << " PID: " << buf.address << " at time " << shm_clock->secs << ":" << shm_clock->nanos << std::endl;

                if(rcvbuf.msgCode == MSG_TYPE_SUCCESS){     // if child has been terminated
                    update_process_table_of_terminated_child(processTable, rcvbuf.address);
                    outputFile << "OSS: Worker " << i + 1 << " PID: " << buf.address << " is planning to terminate" << std::endl;
                }
            }
        }      

        if(numChildren > 0 && launch_interval_satisfied(launch_interval)  
        && process_table_vacancy(processTable, simultaneous)){ // child process launch check
            cout << "Launching Child Process..." << endl;
            numChildren--;
            launch_child(processTable, time_limit, simultaneous);
        }               
    }                   // --------- END OF MAIN LOOP ---------  

	printf("Child processes have completed.\n");
    printf("Parent is now ending.\n");
    outputFile << "Child processes have completed.\n";
    outputFile << "Parent is now ending.\n";
    outputFile.close();  // file object close

    shmdt(shm_clock);      // clock cleanup, detatch & delete shm
    if (shmctl(shmtid, IPC_RMID, NULL) == -1) 
        perror("Error: shmctl failed!!");
    kill_all_processes(processTable);

    if (msgctl(msgqid, IPC_RMID, NULL) == -1) {  // get rid of message queue
		perror("msgctl to get rid of queue in parent failed");
		exit(1);
	}

    return 0;
}

void launch_child(PCB processTable[], int time_limit, int simultaneous){
    string rand_secs = std::to_string(generate_random_number(1, (time_limit - 1)));
    string rand_nanos = std::to_string(generate_random_number(0, 999999999));
    // string user_parameters = std::to_string(rand_secs) + " " + std::to_string(rand_nanos); 

    pid_t childPid = fork(); // This is where the child process splits from the parent        
    if (childPid == 0) {            // Each child uses exec to run ./user	
        // execl("./user", "user", user_parameters.c_str(), NULL);    
        char* args[] = {const_cast<char*>("./user"), const_cast<char*>(rand_secs.c_str()), const_cast<char*>(rand_nanos.c_str()), nullptr};
        execvp(args[0], args);
        perror("Error: Failed to execute user program");
        exit(EXIT_FAILURE);
    } else if (childPid == -1) {  // Fork failed
        perror("Error: Fork has failed");
        exit(EXIT_FAILURE);
    } else {            // Parent updates Process Table with child info after fork()
        int i = (process_table_vacancy(processTable, simultaneous) - 1);
        processTable[i].occupied = 1;
        processTable[i].pid = childPid;
        processTable[i].startSecs = shm_clock->secs;
        processTable[i].startNanos = shm_clock->nanos;
    }
}

int generate_random_number(int min, int max) {  // pseudo rng for random child workload
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::default_random_engine generator(seed);
    std::uniform_int_distribution<int> distribution(min, max);
    int random_number = distribution(generator);
    return random_number;
}

bool launch_interval_satisfied(int launch_interval){
    static int last_launch_secs = 0;  // static ints used to keep track of 
    static int last_launch_nanos = 0;   // most recent process launch

    int elapsed_secs = shm_clock->secs - last_launch_secs; 
    int elapsed_nanos = shm_clock->nanos - last_launch_nanos;

    while (elapsed_nanos < 0) {   // fix if subtracted time is too low
        elapsed_secs--;
        elapsed_nanos += 1000000000;
    }

    if (elapsed_secs > 0 || (elapsed_secs == 0 && elapsed_nanos >= launch_interval)) {        
        last_launch_secs = shm_clock->secs;  // Update the last launch time
        last_launch_nanos = shm_clock->nanos;        
        return true;
    } else {
        return false;
    }
}

void help(){   // Help message here
    printf("-h detected. Printing Help Message...\n");
    printf("The options for this program are: \n");
    printf("\t-h Help will halt execution, print help message, and take no arguments.\n");
    printf("\t-n The argument following -n will be number of total processes to be run.\n");
    printf("\t-s The argument following -s will be max number of processes to be run simultaneously\n");
    printf("\t-t The argument following -t will be the max time limit for each user process created.\n");
    printf("\t-i The argument following -i will be launch interval between process launch in milliseconds.\n");
    printf("\t-i The argument following -f will be the logfile name (please include file extention)\n");
    printf("\t args will default to appropriate values if not provided.\n");
}

void timeout_handler(int signum) {
    std::cout << "Timeout occurred. Cleaning up before exiting..." << std::endl;
    term = 1;
    kill_all_processes(processTable);
    outputFile.close();  // file object close
    shmdt(shm_clock);  // clock cleanup, detatch & delete shm
    if (shmctl(shmtid, IPC_RMID, NULL) == -1) {
        perror("Error: shmctl failed!!");
        exit(1);
    }      
    if (msgctl(msgqid, IPC_RMID, NULL) == -1) {  // get rid of message queue
		perror("msgctl to get rid of queue in parent failed");
		exit(1);
	}
    std::exit(EXIT_SUCCESS);
}

// Signal handler for Ctrl+C (SIGINT)
void ctrl_c_handler(int signum) {
    std::cout << "Ctrl+C detected. Cleaning up before exiting..." << std::endl;
    kill_all_processes(processTable);
    outputFile.close();  // file object close
    shmdt(shm_clock);       // clock cleanup, detatch & delete shm
    if (shmctl(shmtid, IPC_RMID, NULL) == -1) {
        perror("Error: shmctl failed!!");
        exit(1);
    }            
    if (msgctl(msgqid, IPC_RMID, NULL) == -1) {  // get rid of message queue
		perror("msgctl to get rid of queue in parent failed");
		exit(1);
	}
    std::exit(EXIT_SUCCESS);
}