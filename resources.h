// CS4760-001SS - Terry Ford Jr. - Project 5 Resource Management - 03/29/2024
// https://github.com/tfordjr/resource-management.git

#ifndef RESOURCES_H
#define RESOURCES_H

#define NUM_RESOURCES 10
#define NUM_INSTANCES 20

#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <fstream>
#include <string>
#include <queue>
#include <set>

struct Resource{
    int allocated;
    int available;
};

struct Resource resourceTable[NUM_RESOURCES];     // resource table
std::queue<pid_t> resourceQueues[NUM_RESOURCES];  // Queues for each resource

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
        std::cout << "OSS PID: " << getpid() << "  SysClockS: " << secs << "  SysClockNano " << nanos << "\nResource Table:\nResource  Allocated  Available\n";
        outputFile << "OSS PID: " << getpid() << "  SysClockS: " << secs << "  SysClockNano " << nanos << "\nResource Table:\nResource  Allocated  Available\n";
        for(int i = 0; i < NUM_RESOURCES; i++){
            std::cout << static_cast<char>(65 + i) << "\t  " << std::to_string(resourceTable[i].allocated) << "\t     " << std::to_string(resourceTable[i].available) << std::endl;
            outputFile << static_cast<char>(65 + i) << "\t  " << std::to_string(resourceTable[i].allocated) << "\t     " << std::to_string(resourceTable[i].available) << std::endl;
        }
        next_print_nanos = next_print_nanos + 500000000;
        if (next_print_nanos >= 1000000000){   // if over 1 billion nanos, add 1 second, sub 1 bil nanos
            next_print_nanos = next_print_nanos - 1000000000;
            next_print_secs++;
        }    
    }
}

pid_t return_PCB_index_of_pid(PCB processTable[], int simultaneous, pid_t pid){
    for (int i = 0; i < simultaneous; i++){  
        if (processTable[i].pid == pid){
            return i;
        }
    }
    perror("resources.h: Error: given pid not found on process table");
    exit(1);
    return -1;
}

void allocate_resources(PCB processTable[], int index, pid_t pid){
    resourceTable[index].available -= 1;
    resourceTable[index].allocated += 1;
    // LOG ALLOCATION OF RESOURCES SOMEWHERE
    processTable[]
    // Notify the process that it has been allocated resources
}

bool request_resources(int index, int instances, pid_t pid){
    if (resourceTable[index].available >= instances){
        // allocate_resources(index, instances, pid);
        return true;        
    } 
    std::cout << "Insufficient resources available for request." << std::endl;
    resourceQueues[index].push(pid);
    return false;
}

void release_resources(PCB processTable[], int simultaneous, Resource resourceTable[], pid_t killed_pid){ // needs process table to find out
    // find held resources by killed_pid
    int i = return_PCB_index_of_pid(processTable, simultaneous, killed_pid);

    for (int j = 0; j < NUM_RESOURCES; j++){
        resourceTable[j].available += processTable[i].resources_held[j];
        resourceTable[j].allocated -= processTable[i].resources_held[j];
        processTable[i].resources_held[j] = 0;
    }

    // RESOURCES DONE RELEASING HERE


    // If there are processes waiting in the queue for this resource, allocate resources to them
    for (int j = 0; j < NUM_RESOURCES; j++){
        while (!resourceQueues[j].empty() && resourceTable[j].available > 0){                      
            allocate_resources(processTable, j, resourceQueues[j].front()); // Allocate one instance to the waiting process
            resourceQueues[j].pop();
        }
    }
}

int dd_algorithm(){   // if deadlock, return resource number, else return 0 
    for(int i = 0; i < NUM_RESOURCES; i++){
        if (resourceTable[i].available == 0){ // if all allocated
            if (){ // allocated only to blocked processes, 
                return i;
            }
        }
    }
    return 0;
}    

void deadlock_detection(PCB processTable[], int simultaneous, Resource resourceTable[], int secs, int nanos){
    static int next_dd_secs = 0;  // used to keep track of next deadlock detection    
    if(secs >= next_dd_secs){
        int deadlocked_resource_index = dd_algorithm(); // returns 0 if no deadlock
        while(deadlocked_resource_index){  // While deadlock
            // kills random pid that is allocated a resource that is fully allocated
            kill(resourceQueues[deadlocked_resource_index].front(), SIGKILL);            
            release_resources(processTable, resourceTable, resourceQueues[deadlocked_resource_index].front()); // release resources held by PID!            
            resourceQueues[deadlocked_resource_index].pop();
        }
        next_dd_secs++;
    }
}

    // dd_algo   for each process in blocked queue, if all other instances of a given Resource
    // are held by blocked processes, then a deadlock exists

    // We will solve it by killing a random blocked process that holds at least one instance 
    // of a resource that whose instances are all allocated
    // then run dd_algo again

    // KEEP STATS OF HOW MANY PROCs KILLED THIS WAY
    // KEEP STATS OF HOW MANY TIMES dd_algorithm is run!

#endif