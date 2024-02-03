CS4760-001SS - Terry Ford Jr. - Project 1 Processes - 01/28/2024
https://github.com/tfordjr/multiple-processes

Compile and run instructions:
simply run make command and oss command in the following format:
oss -n 4 -s 2 -t 3
where -n is total processes, -s simultaneous processes, and -t iterations done
use the -h arg to learn more about how to use these commands.

Features: 
oss.cpp: processes args and handles child forking with fork_and_wait funciton.
user.cpp: user executable takes one arg for number of iterations. prints and sleeps. 
makefile: creates oss and user executables, oss executes user executable to perform work.

Problem areas: 
Defaults assigned as 1 if none are found. Unsure what the expectation is. 