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

void send_msg_to_child(msgbuffer);

struct Resource resourceTable[NUM_RESOURCES];     // resource table
std::queue<pid_t> resourceQueues[NUM_RESOURCES];  // Queues for each resource
int ddAlgoKills = 0;           // globals to keep track for statistics output after execution
int ddAlgoRuns = 0;
int numDeadlocks = 0;
int requestsImmediatelyGranted = 0;
int requestsEventuallyGranted = 0;

std::ofstream outputFile;   // init file object

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

    // print_process_table(processTable, simultaneous, 999, 999, outputFile);
    // print_resource_table(resourceTable, 999, 999, outputFile);
    
    // std::cout << "pid " << pid << " not found. return_PCB_index_of_pid() failed. Cleaning up before exiting..." << std::endl;    
    // kill_all_processes(processTable, simultaneous);   
    // std::exit(EXIT_SUCCESS);
    return -1;
}

    //allocate_resources() ALLOCATES UNCONDITIONALLY, MUST BE CAREFUL WHEN WE CALL IT!!!
void allocate_resources(PCB processTable[], int simultaneous, int resource_index, pid_t pid){
    resourceTable[resource_index].available -= 1;
    resourceTable[resource_index].allocated += 1;
        // LOG ALLOCATION OF RESOURCES ON PCB
    std::cout << "Calling return_PCB...() from allocate_resources() with pid " << pid << std::endl;
    int i = return_PCB_index_of_pid(processTable, simultaneous, pid);
    processTable[i].resourcesHeld[resource_index]++;
            // Notify the process that it has been allocated resources
    msgbuffer buf;
    buf.mtype = pid;
    buf.msgCode = MSG_TYPE_GRANTED;
    buf.resource = resource_index;
    send_msg_to_child(buf);
}

void request_resources(PCB processTable[], int simultaneous, int resource_index, pid_t pid){
    if (resourceTable[resource_index].available > 0){
        allocate_resources(processTable, simultaneous, resource_index, pid);
        requestsImmediatelyGranted++;
        std::cout << "OSS: Granted " << pid << " resource " << static_cast<char>(65 + resource_index) << std::endl;
        return;        
    } 
    std::cout << "OSS: Insufficent resources " << pid << " added to resource queue " << static_cast<char>(65 + resource_index) << std::endl;
    std::cout << "Calling return_PCB...() from request_resources() with pid " << pid << std::endl;
    int i = return_PCB_index_of_pid(processTable, simultaneous, pid);

    // SEND MESSAGE LETTING PROC KNOW HE IS BLOCKED
    msgbuffer buf;
    buf.mtype = pid;
    buf.msgCode = MSG_TYPE_BLOCKED;
    buf.resource = resource_index;
    send_msg_to_child(buf);

    processTable[i].blocked = 1;
    resourceQueues[resource_index].push(pid);
}

void release_all_resources(PCB pTable[], int simultaneous, Resource rTable[], pid_t killed_pid){ // needs process table to find out
    // find held resources by killed_pid
    std::cout << "Calling return_PCB...() from release_all_resources() with pid " << killed_pid << std::endl;
    int i = return_PCB_index_of_pid(pTable, simultaneous, killed_pid);

    for (int j = 0; j < NUM_RESOURCES; j++){
        rTable[j].available += pTable[i].resourcesHeld[j];
        rTable[j].allocated -= pTable[i].resourcesHeld[j];
        pTable[i].resourcesHeld[j] = 0;
    }

    std::cout << "release_all_resources() complete" << std::endl;
}

void release_single_resource(PCB processTable[], int simultaneous, Resource resourceTable[], pid_t pid){
    std::cout << "Calling return_PCB...() from release_single_resource() with pid " << pid << std::endl;
    
    int i = return_PCB_index_of_pid(processTable, simultaneous, pid);
    if (i == -1)  // Common bug where process termed and old release resource msg
        return;   // for some reason left in msgq, msg release req safely ignored

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

bool dd_algorithm(PCB processTable[], int simultaneous, Resource resourceTable[], pid_t deadlockedPIDs[], int* index){
    ddAlgoRuns++;
    *index = 0;

    struct PCB simProcessTable[20];    // declaration of temp copies of tables, resources
    struct Resource simResourceTable[NUM_RESOURCES];
    std::queue<pid_t> simResourceQueues[NUM_RESOURCES];
    
    for (int i = 0; i < NUM_RESOURCES; i++) {   // Create a local copies        
        simResourceQueues[i] = resourceQueues[i];
        simResourceTable[i] = resourceTable[i];
    }
    for (int i = 0; i < 20; i++) {   // Create a local copies        
        simProcessTable[i] = processTable[i];
    }
    

    for (int i = 0; i < simultaneous; i++){   // free all processes not in a blocked queue
        if (!simProcessTable[i].blocked){   // these processes are 100% not deadlocked bc they're not blocked
            std::cout << "dd_algorithm() simulating release_all_resources() SIMULATING RELEASING RUNNING PROCS" << std::endl;
            release_all_resources(simProcessTable, simultaneous, simResourceTable, simProcessTable[i].pid);
        }
    }
                    // THINK SEGFAULT IS BELOW
    int count = 0;
    while(count < 3){   // repeat attempted allocation 3 times to be generous
        for (int i = 0; i < NUM_RESOURCES; i++){ // attempt to allocate free resources
            std:: cout << "dd_algo() SIM UNBLOCK LOOP: " << i << std::endl;
            while (!simResourceQueues[i].empty() && simResourceTable[i].available > 0){ 
                std::cout << "dd_algorithm() simulating release_all_resources() SIMULATING RELEASING UNBLOCKS" << std::endl; 
                release_all_resources(simProcessTable, simultaneous, simResourceTable, simResourceQueues[i].front());
                simResourceQueues[i].pop();                     
            }        
        }
        count++;
    }
    
    for (int i = 0; i < NUM_RESOURCES; i++){ // logging stubborn (probably deadlocked) pids
        std:: cout << "dd_algo() DEADLOCKED TRACKING ATTEMPT: " << i << std::endl;
        while (!simResourceQueues[i].empty()){
            std::cout << "dd_algo() DEADLOCK TRACKING SUCCESS: " << i << std::endl;
            std::cout << "Index-" << *index << "   simResourceQueues[i].front()-" << simResourceQueues[i].front() << std::endl;
            deadlockedPIDs[(*index)++] = simResourceQueues[i].front();
            simResourceQueues[i].pop();
        }
    }
    std:: cout << "dd_algo() ENDING! " << std::endl;
    if(*index == 0) 
        return false;   // THERE ARE NO DEADLOCKS IN SYSTEM
    return true;
}

void deadlock_detection(PCB processTable[], int simultaneous, Resource resourceTable[], int secs, int nanos){
    static int next_dd_secs = 0;  // used to keep track of next deadlock detection    
    if(secs < next_dd_secs)  // bails if not time for dd() yet
        return;  
    next_dd_secs++;

    pid_t deadlockedPIDs[simultaneous];
    int index = 0;
    while(dd_algorithm(processTable, simultaneous, resourceTable, deadlockedPIDs, &index)){
            // determine the pid with the least resources (least impact on system) and kill it
        int sum = 0;
        int leastSum = (NUM_RESOURCES * NUM_INSTANCES);
        pid_t pidWithLeastSum;
        while(index >= 0){
            std::cout << "Calling return_PCB...() from deadlock_detection() with pid " << deadlockedPIDs[index] << std::endl;
            int i = return_PCB_index_of_pid(processTable, simultaneous, deadlockedPIDs[index]);
            for(int j = 0; j < NUM_RESOURCES; j++){
                sum += processTable[i].resourcesHeld[j];
            }
            if(sum < leastSum){
                leastSum = sum;
                pidWithLeastSum = deadlockedPIDs[index];
            }
            sum = 0;
            index--;
        }        
        std::cout << "deadlock_detection() running release_all_resources() REAL TERMINATIONS" << std::endl;
        release_all_resources(processTable, simultaneous, resourceTable, pidWithLeastSum); // release resources held by PID!       
        kill(pidWithLeastSum, SIGKILL);     // kill that least important pid
        ddAlgoKills++;
        index = 0;        
    }
}
    
void attempt_process_unblock(PCB processTable[], int simultaneous, Resource resourceTable[]){   
    for (int j = 0; j < NUM_RESOURCES; j++){
        while (!resourceQueues[j].empty() && resourceTable[j].available > 0){                      
            allocate_resources(processTable, simultaneous, j, resourceQueues[j].front()); // Allocate one instance to the waiting process             
            int i = return_PCB_index_of_pid(processTable, simultaneous, resourceQueues[j].front());
            resourceQueues[j].pop();
            processTable[i].blocked = 0;
            requestsEventuallyGranted++;
            std::cout << "OSS: Unblocked " << processTable[i].pid << " granted resource " << static_cast<char>(65 + j) << std::endl;
        }
    }  
}      

#endif