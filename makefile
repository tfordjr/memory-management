# CS4760-001SS - Terry Ford Jr. - Project 2 Process Tables - 02/12/2024
# https://github.com/tfordjr/process-tables

CC	= g++ -g3
CFLAGS  = -g3 -g
TARGET1 = user
TARGET2 = oss 

OBJS1	= user.o
OBJS2	= oss.o

all:	$(TARGET1) $(TARGET2)

$(TARGET1):	$(OBJS1)
	$(CC) -o $(TARGET1) $(OBJS1)

$(TARGET2):	$(OBJS2)
	$(CC) -o $(TARGET2) $(OBJS2)

child.o:	child.cpp
	$(CC) $(CFLAGS) -c user.cpp 

parent.o:	parent.cpp
	$(CC) $(CFLAGS) -c oss.cpp

clean:
	/bin/rm -f *.o $(TARGET1) $(TARGET2)
