// CS4760-001SS - Terry Ford Jr. - Project 4 OSS Scheduler - 03/08/2024
// https://github.com/tfordjr/oss-scheduler.git

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
	long mtype;
    char message[100]; // WANT TO CHANGE TO INT FOR ACTUAL TIME SLICE USED
	int time_slice_used;  // int sufficient, holds -2 to 2 bil, only need -40 to 40 mil (40 ms converted to ns)
	int msgCode;
} msgbuffer;

#define MSG_TYPE_BLOCKED 2   // BLOCKED
#define MSG_TYPE_RUNNING 1   // STILL RUNNING
#define MSG_TYPE_SUCCESS 0   // TERMINATING

#endif