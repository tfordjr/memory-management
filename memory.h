// CS4760-001SS - Terry Ford Jr. - Project 6 Memory Management - 04/21/2024
// https://github.com/tfordjr/memory-management.git

#ifndef MEMORY_H
#define MEMORY_H

struct frame{    // OSS FRAME TABLE - 256K memory, 256 1k frames
    bool secondChanceBit;
    bool dirtyBit;
};

void page_fault(){
    static int victimFrame = 0;
    // run second chance algorithm, determine which frame will be replaced
    // victimFrame = replaced frame + 1
}

void page_request(){
    // if pagefault
    // page_fault(), put process in blocked queue until page is brought in

    // if not pagefault
    // increment() 100 ns
    // send msg back to proc
}

void attempt_process_unblock(){   // attempt unblock from queue waiting for page unblock
    
}   

#endif