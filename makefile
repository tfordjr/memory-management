# CS4760-001SS - Terry Ford Jr. - Project 3 Message Queues - 02/29/2024
# https://github.com/tfordjr/message-queues.git

CC	= g++ 
CFLAGS  = -g3 -std=c++11
TARGET1 = user
TARGET2 = oss 

OBJS1	= user.o
OBJS2	= oss.o

all:	$(TARGET1) $(TARGET2)

$(TARGET1):	$(OBJS1)
	$(CC) -o $(TARGET1) $(OBJS1)

$(TARGET2):	$(OBJS2)
	$(CC) -o $(TARGET2) $(OBJS2)

user.o:	user.cpp
	$(CC) $(CFLAGS) -c user.cpp 

parent.o:	parent.cpp
	$(CC) $(CFLAGS) -c oss.cpp

clean:
	/bin/rm -f *.o $(TARGET1) $(TARGET2) logfile.txt
	../memclean.sh
