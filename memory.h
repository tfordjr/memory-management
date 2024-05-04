// CS4760-001SS - Terry Ford Jr. - Project 6 Memory Management - 04/21/2024
// https://github.com/tfordjr/memory-management.git

#ifndef MEMORY_H
#define MEMORY_H

#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <fstream>
#include <string>
#include <queue>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

struct Page{    // OSS PAGE TABLE - 256K memory, 256 1k pages
    pid_t owner;
    int pageNumber;
    bool secondChanceBit;
    bool dirtyBit;
};

const int PAGE_TABLE_SIZE = 256;
struct Page pageTable[PAGE_TABLE_SIZE];
std::queue<pid_t> pageQueue; // only 18 procs in system at a time

// init page table

void page_fault(){
    static int victimPage = 0;
    bool victimFound = false;

    while(!victimFound){
        if(pageTable[victimPage].secondChanceBit = 1){   // victim not found
            pageTable[victimPage].secondChanceBit = 0;            
        } else {    // second chance bit == 0, victim is found, swap out page
            victimFound = true;
            // pageTable[victimPage].owner = 
            // pageTable[victimPage].pageNumber =
            // pageTable[victimPage].secondChanceBit = 1;
            // pageTable[victimPage].dirtyBit = 
                // SEND MSG TO CHILD
        }        
        victimPage++;  // we increment victimPage whether it's found or not
        if(victimPage == PAGE_TABLE_SIZE){
            victimPage = 0;        
        }
    }
    // run second chance algorithm, determine which page will be replaced
    // victimPage = replaced page + 1
}

void page_request(pid_t pid, int memoryAddress, int msgCode){

    int pageNumber = memoryAddress/1024;
    int offset = memoryAddress % 1024;

    for (int i = 0; i < PAGE_TABLE_SIZE; i++){
        if(pageTable[i].owner == pid && pageTable[i].pageNumber == pageNumber){
            // if(Write)
            // pageTable[i].dirtyBit = 1;
            // pageTable[i].secondChanceBit = 1;
            // increment() 100 ns
            // send msg to child to let them know page was in memory
            return;  
        }
    }
    
    // page_fault(),     
    // MSG CHILD THEY'RE BLOCKED
}

// void attempt_process_unblock(){   // attempt unblock from queue waiting for page unblock
    
// }   

#endif