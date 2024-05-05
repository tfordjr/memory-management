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
#include "msgq.h"
#include "clock.h"

struct Page{    // OSS PAGE TABLE - 0-255k
    pid_t pid;
    int pageNumber;  // 0-63
    bool secondChanceBit;
    bool dirtyBit;
};

struct ProcInfo{
    pid_t pid;
    int secs;
    int nanos;
    int pageNumber;
    int msgCode;
        // Constructor
    ProcInfo(pid_t p, int s, int n, int a, int m) : pid(p), secs(s), nanos(n), pageNumber(a), msgCode(m) {}
};

std::queue<ProcInfo> blockedQueue; // queue of blocked procs with their unblock time

void send_msg_to_child(msgbuffer);

const int FRAME_TABLE_SIZE = 256;
int memoryAccesses = 0;
int pageFaults = 0;
// std::queue<pid_t> pageQueue; // Omitting queue for now

void init_frame_table(Page frameTable[]){
    for(int i = 0; i < FRAME_TABLE_SIZE; i++){
        frameTable[i].pid = 0;
        frameTable[i].pageNumber = 0;
        frameTable[i].secondChanceBit = 0;
        frameTable[i].dirtyBit = 0; 
    }
}

void print_frame_table(Page frameTable[], int secs, int nanos, std::ostream& outputFile){
    static int next_print_secs = 0;  // static ints used to keep track of each 
    static int next_print_nanos = 0;   // process table print to be done

    if(secs > next_print_secs || (secs == next_print_secs && nanos > next_print_nanos)){
        std::cout << "OSS PID: " << getpid() << "  SysClockS: " << secs << "  SysClockNano " << nanos << "  \nPage Table:\n\tOwner PID\tPage Number\t2nd Chance Bit\tDirty Bit\n";
        outputFile << "OSS PID: " << getpid() << "  SysClockS: " << secs << "  SysClockNano " << nanos << "  \nPage Table:\n\tOwner PID\tPage Number\t2nd Chance Bit\tDirty Bit\n";
        for(int i = 0; i < FRAME_TABLE_SIZE; i++){
            std::cout << "Frame " << std::to_string(i + 1) << ":\t" << std::to_string(frameTable[i].pid) << "\t" << std::to_string(frameTable[i].pageNumber) << "\t" << std::to_string(frameTable[i].secondChanceBit) << "\t" << std::to_string(frameTable[i].dirtyBit) << std::endl;
            outputFile << std::to_string(i + 1) << "\t" << std::to_string(frameTable[i].pid) << "\t" << std::to_string(frameTable[i].pageNumber) << "\t" << std::to_string(frameTable[i].secondChanceBit) << "\t" << std::to_string(frameTable[i].dirtyBit) << std::endl;
        }
        next_print_nanos = next_print_nanos + 500000000;
        if (next_print_nanos >= 1000000000){   // if over 1 billion nanos, add 1 second, sub 1 bil nanos
            next_print_nanos = next_print_nanos - 1000000000;
            next_print_secs++;
        }    
    }
}

void page_fault(Page frameTable[], std::ofstream* outputFile, pid_t pid, int pageNumber, int msgCode){
    static int victimFrame = 0;
    bool victimFound = false;

    while(!victimFound){
        if(frameTable[victimFrame].secondChanceBit == 1){   // victim not found
            frameTable[victimFrame].secondChanceBit = 0;            
        } else {    // second chance bit == 0, victim is found, swap out page
            victimFound = true;
            if(frameTable[victimFrame].dirtyBit){    // SAVE OLD PAGE BEFORE SWAPPING OUT
                std::cout << "OSS: Swapping out dirty frame, saving to secondary storage..." << std::endl;
                *outputFile << "OSS: Swapping out dirty frame, saving to secondary storage..." << std::endl;
            }

            frameTable[victimFrame].pid = pid;   // WRITING NEW PAGE TO MAIN MEMORY
            frameTable[victimFrame].pageNumber = pageNumber;
            frameTable[victimFrame].secondChanceBit = 1;
            frameTable[victimFrame].dirtyBit = (msgCode == MSG_TYPE_WRITE) ? 1 : 0;
                
            msgbuffer buf;         // SEND GRANTED MSG TO CHILD
            buf.mtype = pid;
            buf.sender = getpid();
            buf.memoryAddress = pageNumber;
            buf.msgCode = MSG_TYPE_GRANTED;
            memoryAccesses++;
            send_msg_to_child(buf);
        }
        victimFrame++;  // we increment victimFrame whether it's found or not
        if(victimFrame == FRAME_TABLE_SIZE){
            victimFrame = 0;        
        }
    }
}

void page_request(Page frameTable[], std::ofstream* outputFile, Clock* c, pid_t pid, int memoryAddress, int msgCode){

    msgbuffer buf;          // msgbuffer setup
    buf.mtype = pid;
    buf.sender = getpid();
    buf.memoryAddress = memoryAddress;
    int pageNumber = memoryAddress/1024;  // translation of memory address
    // int offset = memoryAddress % 1024;        

    for (int i = 0; i < FRAME_TABLE_SIZE; i++){  // init scan of frameTable
        if(frameTable[i].pid == pid && frameTable[i].pageNumber == pageNumber){  
            
            if(msgCode == MSG_TYPE_WRITE)
                frameTable[i].dirtyBit = 1;     
            frameTable[i].secondChanceBit = 1;       
            increment(c, 100);            

            buf.msgCode = MSG_TYPE_GRANTED;
            memoryAccesses++;
            send_msg_to_child(buf); 
            return;
        }
    }
        // page not in main memory! Page Fault! 
    buf.msgCode = MSG_TYPE_BLOCKED;  
    send_msg_to_child(buf); 

    // init time values
    int unblockSecs = c->secs;
    int unblockNanos = c->nanos;
    // add_time()
    add_time(&unblockSecs, &unblockNanos, 1.4e7);

    blockedQueue.push(ProcInfo(pid, unblockSecs, unblockNanos, pageNumber, msgCode));

        // can't run page_fault() to replace until secondary storage retrieves
        // desired frame which is simulated by blocked queue and 14ms wait
    // page_fault(frameTable, outputFile, pid, pageNumber, msgCode);    
    // pageFaults++; 
}

void attempt_process_unblock(Page frameTable[], std::ofstream* outputFile, Clock* c){   // attempt unblock from queue waiting for page unblock
        // if unblock time has elapsed
    while(compare_time(c->secs, c->nanos, blockedQueue.front().secs, blockedQueue.front().nanos)){
        page_fault(frameTable, outputFile, blockedQueue.front().pid, blockedQueue.front().pageNumber, blockedQueue.front().msgCode);    
        pageFaults++; 
        blockedQueue.pop();
    }
}   

#endif