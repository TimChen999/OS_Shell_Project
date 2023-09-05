#include <stdio.h>
#include <stdbool.h>
#include <readline/readline.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include "signalHeader.h"

bool sigDebug = true;

//Parent progress
pid_t parentProcess; 

//Can be a max of 2 foreground processes
pid_t foregroundProcess[2]; 
pid_t numForegroundProcess;

//Can be a max of 50 background processes
pid_t backgroundProcess[50]; 
pid_t numBackgroundProcess;

int resetProcess(){
    parentProcess = 0;
    numForegroundProcess = 0;
    numBackgroundProcess = 0;
    foregroundProcess[0] = 0;
    foregroundProcess[1] = 0;
    backgroundProcess[0] = 0;
    backgroundProcess[1] = 0;
}

int addForegroundProcess(pid_t processNumber){
    foregroundProcess[numForegroundProcess] = processNumber;
    numForegroundProcess++;
}

int addBackgroundProcess(pid_t processNumber){
    backgroundProcess[numBackgroundProcess] = processNumber;
    numBackgroundProcess++;
}

int setParentProcess(pid_t processNumber){
    parentProcess = processNumber;
}

//SIGINT handler
void sigintHandler(int signal){
    //Stop foreground processes
    if(numForegroundProcess == 1){
        kill(foregroundProcess[0], SIGINT);
        numForegroundProcess = 0;
    }
    else if(numForegroundProcess == 2){
        
        kill(foregroundProcess[0], SIGINT);
        kill(foregroundProcess[1], SIGINT);
        numForegroundProcess = 0;
    }
}

//SIGSTOP handler (stops foreground command)
void sigstopHandler(int signal){
    //Requires job table, to be implemented
}

//Init signals
int sigInit(){
    signal(SIGINT, sigintHandler);
    signal(SIGSTOP, sigstopHandler);
}