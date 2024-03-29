// CS4760-001SS - Terry Ford Jr. - Project 5 Resource Management - 03/29/2024
// https://github.com/tfordjr/resource-management.git

#ifndef MSGQ_H
#define MSGQ_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>

#define MSGQ_FILE_PATH "msgq.txt"
#define MSGQ_PROJ_ID 65
#define PERMS 0644

typedef struct msgbuffer {
	long mtype;			 	 // pid
    char message[100];   	 // string message
	int time_slice; 	 // OSS SENDS TO CHILD: TIME SLICE ALLOTTED TO CHILD!
						// CHILD SENDS TO OSS: TIME SLICE ACTUALLY USED!
	int blocked_until_secs;	 // time when process becomes unblocked, MSGSEND CHILD TO PARENT ONLY,
	int blocked_until_nanos; // PARENT ONLY SENDS 0s FOR THESE ARGS TO CHILD
	int msgCode;			  // process state code
} msgbuffer;

#define MSG_TYPE_BLOCKED 2   // BLOCKED
#define MSG_TYPE_RUNNING 1   // STILL RUNNING
#define MSG_TYPE_SUCCESS 0   // TERMINATING
 
#endif