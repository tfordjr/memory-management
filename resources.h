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

struct Resource{
    int allocated;
    int available;
};

struct Resource resourceTable[NUM_RESOURCES]; // resourceTable declaration
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

void allocate_resources(int index, int instances, pid_t pid){
    resourceTable[index].available -= instances;
    resourceTable[index].allocated += instances;
}

bool request_resources(int index, int instances, pid_t pid){
    if (resourceTable[index].available >= instances){
        allocate_resources(index, instances, pid);
        return true;        
    } 
    std::cout << "Insufficient resources available for request." << std::endl;
    resourceQueues[index].push(pid);
    return false;
}

void release_resources(int index, int instances){
    resourceTable[index].available += instances;
    resourceTable[index].allocated -= instances;

    // If there are processes waiting in the queue for this resource, allocate resources to them
    while (!resourceQueues[index].empty() && resourceTable[index].available > 0) {
        // This assumes pid in queue wants just ONE of this resource and nothing else! Make SURE!
        pid_t waitingPID = resourceQueues[index].front();
        resourceQueues[index].pop();

        // allocate_resources(index, x); // Allocate one instance to the waiting process
        
        // Notify the process that it has been allocated resources        
    }
}

#endif