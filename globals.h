// CS4760-001SS - Terry Ford Jr. - Project 5 Resource Management - 03/29/2024
// https://github.com/tfordjr/resource-management.git

volatile sig_atomic_t term = 0;  // signal handling global
struct PCB processTable[20]; // Init Process Table Array of PCB structs (not shm)
  // RESOURCE TABLE DECLARED IN RESOURCES_H

// Declaring globals needed for signal handlers to clean up at anytime
Clock* shm_clock;  // Declare global shm clock
key_t clock_key = ftok("/tmp", 35);             
int shmtid = shmget(clock_key, sizeof(Clock), IPC_CREAT | 0666);    // init shm clock
std::ofstream outputFile;   // init file object
int msgqid;           // MSGQID GLOBAL FOR MSGQ CLEANUP
int simultaneous = 1;  // simultaneous global so that sighandlers know PCB table size to avoid segfaults when killing all procs on PCB