// CS4760-001SS - Terry Ford Jr. - Project 3 Message Queues - 02/29/2024
// https://github.com/tfordjr/message-queues.git

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
#define MSGQ_PROJ_ID 1
#define PERMS 0644

typedef struct msgbuffer {   // Had to change names, I was getting confused
	pid_t address;           // type pid_t again so that I avoid confusion
	char message[100];
	int msgCode;
} msgbuffer;

#define MSG_TYPE_SUCCESS 1  // I'm getting confused, so I'm implementing these 
#define MSG_TYPE_RUNNING 0  

#endif