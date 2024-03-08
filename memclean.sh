#!/bin/bash

#Script to clean up your own shared memory
ME=`whoami`

IPCS_S=`ipcs -s | egrep "0x[0-9a-f]+ [0-9]+" | grep $ME | cut -f2 -d" "`
IPCS_M=`ipcs -m | egrep "0x[0-9a-f]+ [0-9]+" | grep $ME | cut -f2 -d" "`
IPCS_Q=`ipcs -q | egrep "0x[0-9a-f]+ [0-9]+" | grep $ME | cut -f2 -d" "`

echo $IPCS_S

echo $IPCS_M

echo $IPCS_Q

for id in $IPCS_M; do
        ipcrm -m $id;
done

for id in $IPCS_S; do
        ipcrm -s $id;
done

for id in $IPCS_Q; do
        ipcrm -q $id;
done

