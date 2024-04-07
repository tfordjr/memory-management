# CS4760-001SS - Terry Ford Jr. - Project 5 Resource Management - 03/29/2024
# https://github.com/tfordjr/resource-management.git
# recently added -Wall and -lrt, remove if it's not working

CC	= g++ 
CFLAGS  = -g3 -std=c++11 -Wall
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
	$(CC) $(CFLAGS) -c oss.cpp -lrt

clean:
	/bin/rm -f *.o $(TARGET1) $(TARGET2) logfile.txt msgq.txt
	./memclean.sh
