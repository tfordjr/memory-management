// CS4760-001SS - Terry Ford Jr. - Project 5 Resource Management - 03/29/2024
// https://github.com/tfordjr/resource-management.git

#ifndef RESOURCES_H
#define RESOURCES_H

#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <fstream>
#include <string>
#include <queue>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "pcb.h"
#include "msgq.h"
#include "rng.h"

struct Resource{
    int allocated;
    int available;
};

void send_unblock_msg(msgbuffer);

struct Resource resourceTable[NUM_RESOURCES];     // resource table
std::queue<pid_t> resourceQueues[NUM_RESOURCES];  // Queues for each resource
int ddAlgoKills = 0;
int ddAlgoRuns = 0;
int numDeadlocks = 0;
int requestsImmediatelyGranted = 0;
int requestsEventuallyGranted = 0;

void init_resource_table(Resource resourceTable[]){
    for(int i = 0; i < NUM_RESOURCES; i++){
        resourceTable[i].available = NUM_INSTANCES;
        resourceTable[i].allocated = 0;      
    }
}

void print_resource_table(Resource resourceTable[], int secs, int nanos, std::ostream& outputFile){
    static int next_print_secs = 0;  // static ints used to keep track of each 
    static int next_print_nanos = 0;   // process table print to be done

    if(secs > next_print_secs || secs == next_print_secs && nanos > next_print_nanos){
        std::cout << "OSS PID: " << getpid() << "  SysClockS: " << secs << "  SysClockNano " << nanos << "\nResource Table:\nResource  Allocated  Available  Resource Queue Size\n";
        outputFile << "OSS PID: " << getpid() << "  SysClockS: " << secs << "  SysClockNano " << nanos << "\nResource Table:\nResource  Allocated  Available  Resource Queue Size\n";
        for(int i = 0; i < NUM_RESOURCES; i++){
            std::cout << static_cast<char>(65 + i) << "\t  " << std::to_string(resourceTable[i].allocated) << "\t     " << std::to_string(resourceTable[i].available) << "         " << std::to_string(resourceQueues[i].size()) <<std::endl;
            outputFile << static_cast<char>(65 + i) << "\t  " << std::to_string(resourceTable[i].allocated) << "\t     " << std::to_string(resourceTable[i].available) << "         " << std::to_string(resourceQueues[i].size()) <<std::endl;
        }
        std::cout << "Resource Queues (number of resources in each):" << std::endl;
        for(int i = 0; i < NUM_RESOURCES; i++){
            std::cout << static_cast<char>(65 + i) << ": " << std::to_string(resourceQueues[i].size()) << "   ";
        }
        std::cout << "\n";
        next_print_nanos = next_print_nanos + 500000000;
        if (next_print_nanos >= 1000000000){   // if over 1 billion nanos, add 1 second, sub 1 bil nanos
            next_print_nanos = next_print_nanos - 1000000000;
            next_print_secs++;
        }    
    }
}

int return_PCB_index_of_pid(PCB processTable[], int simultaneous, pid_t pid){
    for (int i = 0; i < simultaneous; i++){  
        if (processTable[i].pid == pid){
            return i;
        }
    }
    perror("resources.h: Error: given pid not found on process table");
    exit(1);
    return -1;
}
    //allocate_resources() ALLOCATES UNCONDITIONALLY, MUST BE CAREFUL WHEN WE CALL IT!!!
void allocate_resources(PCB processTable[], int simultaneous, int resource_index, pid_t pid){
    resourceTable[resource_index].available -= 1;
    resourceTable[resource_index].allocated += 1;
        // LOG ALLOCATION OF RESOURCES ON PCB
    int i = return_PCB_index_of_pid(processTable, simultaneous, pid);
    processTable[i].resourcesHeld[resource_index]++;
            // Notify the process that it has been allocated resources
    msgbuffer buf;
    buf.mtype = pid;
    buf.msgCode = MSG_TYPE_GRANTED;
    buf.resource = resource_index;
    send_unblock_msg(buf);
}

void request_resources(PCB processTable[], int simultaneous, int resource_index, pid_t pid){
    if (resourceTable[resource_index].available > 0){
        allocate_resources(processTable, simultaneous, resource_index, pid);
        requestsImmediatelyGranted++;
        return;        
    } 
    std::cout << "Insufficient resources available for request." << std::endl;
    resourceQueues[resource_index].push(pid);
}

void release_all_resources(PCB processTable[], int simultaneous, Resource resourceTable[], pid_t killed_pid){ // needs process table to find out
    // find held resources by killed_pid
    int i = return_PCB_index_of_pid(processTable, simultaneous, killed_pid);

    for (int j = 0; j < NUM_RESOURCES; j++){
        resourceTable[j].available += processTable[i].resourcesHeld[j];
        resourceTable[j].allocated -= processTable[i].resourcesHeld[j];
        processTable[i].resourcesHeld[j] = 0;
    }
}

void release_single_resource(PCB processTable[], int simultaneous, Resource resourceTable[], pid_t pid){
    int i = return_PCB_index_of_pid(processTable, simultaneous, pid);

    for (int j = 0; j < 10; j++){  // 10 tries to release a random resource index
        int randomIndex = generate_random_number(0, (NUM_RESOURCES - 1), getpid());
        if (processTable[i].resourcesHeld[randomIndex] > 0){
            resourceTable[randomIndex].available++;
            resourceTable[randomIndex].allocated--;
            processTable[i].resourcesHeld[randomIndex]--;
            std::cout << "OSS: Decided to release Resource " << static_cast<char>(65 + randomIndex) << " from pid " << pid << std::endl;
            return;
        }
    }

    for (int j = 0; j < NUM_RESOURCES; j++){  // if that fails, we sequentially check
        if (processTable[i].resourcesHeld[j] > 0){ // to release a single resource (not random)
            resourceTable[j].available++;
            resourceTable[j].allocated--;
            processTable[i].resourcesHeld[j]--;
            std::cout << "OSS: Decided to release Resource " << static_cast<char>(65 + j) << " from pid " << pid << std::endl;
            return;
        }
    }
    std::cout << "OSS: determined there were no resources to release from pid " << pid << ", release action resulted in no release..." << std::endl;
}  // If both fail, we conclude the child has no resources to release, move on with no action

int dd_algorithm(PCB processTable[], int simultaneous){   // if deadlock, return resource number, else return 0
    ddAlgoRuns++;
    for(int i = 0; i < NUM_RESOURCES; i++){ // for each resource
        int sum = 0;  // sum of instances of a particular resource held by blocked procs
        for(int j = 0; j < simultaneous; j++){  // go through each process 
            if(processTable[j].blocked){  // if process is blocked
                sum += processTable[j].resourcesHeld[i]; 
            }            
        }
        if(sum == NUM_INSTANCES){ // if sum of resource instances held by blocked processes is equal to max number
            return i;  // return resource index number 
        }
    }
    return 0;
}

void deadlock_detection(PCB processTable[], int simultaneous, Resource resourceTable[], int secs, int nanos){
    static int next_dd_secs = 0;  // used to keep track of next deadlock detection    
    if(secs >= next_dd_secs){
        int deadlocked_resource_index = dd_algorithm(processTable, simultaneous); // returns 0 if no deadlock        
        if (deadlocked_resource_index){
            numDeadlocks++;
        }
        while(deadlocked_resource_index){  // While deadlock
            // kills random pid that is allocated a resource that is fully allocated
            kill(resourceQueues[deadlocked_resource_index].front(), SIGKILL);            
            release_all_resources(processTable, simultaneous, resourceTable, resourceQueues[deadlocked_resource_index].front()); // release resources held by PID!            
            resourceQueues[deadlocked_resource_index].pop();
            deadlocked_resource_index = dd_algorithm(processTable, simultaneous);
            ddAlgoKills++;
        }        
        next_dd_secs++;
    }
}

    // dd_algo   for each process in blocked queue, if all other instances of a given Resource
    // are held by blocked processes, then a deadlock exists

    // We will solve it by killing a random blocked process that holds at least one instance 
    // of a resource that whose instances are all allocated
    // then run dd_algo again

    // SHOULD BE ALMOST THE SAME AS 2nd half of release_resources() but more comprehensive
    // need to check all 
void attempt_process_unblock(PCB processTable[], int simultaneous, Resource resourceTable[]){   
    for (int j = 0; j < NUM_RESOURCES; j++){
        while (!resourceQueues[j].empty() && resourceTable[j].available > 0){                      
            allocate_resources(processTable, simultaneous, j, resourceQueues[j].front()); // Allocate one instance to the waiting process 
            resourceQueues[j].pop();
            requestsEventuallyGranted++;
        }
    }  // This implementation only checks each resource queue once, so if there is 
}      // a large buildup of blocked procs in a given queue, that could potentially be 
       // challenging to unblock even if there are large amounts of available resources

#endif