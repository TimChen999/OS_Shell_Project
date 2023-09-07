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
pid_t foregroundProcess; 

//Whether signal was stopped before finishing
bool stopped;

int resetProcess(){
    stopped = false;
    parentProcess = 0;
    foregroundProcess= 0;
}

int addForegroundProcess(pid_t processNumber){
    foregroundProcess = processNumber;
}

int setParentProcess(pid_t processNumber){
    parentProcess = processNumber;
}

//SIGINT handler (Stops currently running process by interrupting execvp)
void sigintHandler(int signal){
    kill(foregroundProcess, SIGINT);
}

//Getter/Setter for stop
bool getStopped(){
    return stopped;
}

void setStopped(bool stop){
    stopped = stop;
}

//SIGSTOP handler (stops foreground command)
void sigstopHandler(int signal){
    //Explicitly set foreground to parent (child processes should be under separate gpid)
    kill(foregroundProcess, SIGSTOP);
    setStopped(foregroundProcess);
    //write(STDIN_FILENO, "SIGSTOP", sizeof("SIGSTOP"));
    stopped = true;
}

//Handler when something from background tries to get terminal control
void sigttouHandler(int signal){
    //write(STDIN_FILENO, "SIGTTOU", sizeof("SIGTTOU"));
    tcsetpgrp(STDOUT_FILENO, parentProcess);
}

//Init signals
int initInt(){
    signal(SIGINT, sigintHandler);
}

int initStop(){
    signal(SIGSTOP, sigstopHandler);
}

int initTtou(){
    signal(SIGTTOU, sigttouHandler);
}
