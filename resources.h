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

struct resource{
    int allocated;
    int available;
};

struct resource resourceTable[NUM_RESOURCES];

void init_resource_table(resource resourceTable[]){
    for(int i = 0; i < NUM_RESOURCES; i++){
        resourceTable[i].available = NUM_INSTANCES;
        resourceTable[i].allocated = 0;      
    }
}

//    RESOURCE  ALLOCATED   AVAILABLE
//    A            10           10
//    B            5            15
//    C            20           0 
//    D            etc... 

void print_resource_table(resource resourceTable[], int secs, int nanos, std::ostream& outputFile){
    static int next_print_secs = 0;  // static ints used to keep track of each 
    static int next_print_nanos = 0;   // process table print to be done

    if(secs > next_print_secs || secs == next_print_secs && nanos > next_print_nanos){
        std::cout << "OSS PID: " << getpid() << "  SysClockS: " << secs << "  SysClockNano " << nanos << "\nResource Table:\nResource  Allocated  Available\n";
        outputFile << "OSS PID: " << getpid() << "  SysClockS: " << secs << "  SysClockNano " << nanos << "\nResource Table:\nResource\tAllocated\tAvailable\n";
        for(int i = 0; i < NUM_RESOURCES; i++){
            std::cout << static_cast<char>(65 + i) << "\t\t" << std::to_string(resourceTable[i].allocated) << "\t\t" << std::to_string(resourceTable[i].available) << std::endl;
            outputFile << static_cast<char>(65 + i) << "\t" << std::to_string(resourceTable[i].allocated) << "\t" << std::to_string(resourceTable[i].available) << std::endl;
        }
        next_print_nanos = next_print_nanos + 500000000;
        if (next_print_nanos >= 1000000000){   // if over 1 billion nanos, add 1 second, sub 1 bil nanos
            next_print_nanos = next_print_nanos - 1000000000;
            next_print_secs++;
        }    
    }
}

void allocate_resources(int index, int instances){
    resourceTable[index].available -= instances;
    resourceTable[index].allocated += instances;
}

bool request_resources(int index, int instances){
    if (resourceTable[index].available >= instances){
        allocate_resources(index, instances);
        return true;        
    } 
    std::cout << "Insufficient resources available for request." << std::endl;
    return false;
}

void release_resources(int index, int instances){
    resourceTable[index].available += instances;
    resourceTable[index].allocated -= instances;
}

#endif