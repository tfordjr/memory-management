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

void send_msg_to_child(msgbuffer);

const int FRAME_TABLE_SIZE = 256;
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
            buf.msgCode = MSG_TYPE_GRANTED;
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
    int pageNumber = memoryAddress/1024;  // translation of memory address
    // int offset = memoryAddress % 1024;        

    for (int i = 0; i < FRAME_TABLE_SIZE; i++){  // init scan of frameTable
        if(frameTable[i].pid == pid && frameTable[i].pageNumber == pageNumber){  
            
            if(msgCode == MSG_TYPE_WRITE)
                frameTable[i].dirtyBit = 1;     
            frameTable[i].secondChanceBit = 1;       
            increment(c, 100);            

            buf.msgCode = MSG_TYPE_GRANTED;
            send_msg_to_child(buf); 
            return;
        }
    }
        // page not in main memory! Page Fault! 
    buf.msgCode = MSG_TYPE_BLOCKED;  
    send_msg_to_child(buf); 
    page_fault(frameTable, outputFile, pid, pageNumber, msgCode);     
}

// void attempt_process_unblock(){   // attempt unblock from queue waiting for page unblock
    
// }   

#endif