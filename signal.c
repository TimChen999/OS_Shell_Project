#include <stdio.h>
#include <stdbool.h>
#include <readline/readline.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include "signalHeader.h"
#include "jobs.h"

bool sigDebug = false;

//Parent progress
pid_t parentProcess; 

//Can be a max of 2 foreground processes
pid_t foregroundProcess; 

//Whether process was stopped before finishing
bool stopped;

int resetProcess(){
    stopped = false;
    parentProcess = 0;
    foregroundProcess= 0;
}

int setForegroundProcess(pid_t processNumber){
    if(sigDebug){printf("SIGNAL.C: set foreground [%d]\n", processNumber);}
    foregroundProcess = processNumber;
}

int setParentProcess(pid_t processNumber){
    if(sigDebug){printf("SIGNAL.C: set parent [%d]\n", processNumber);}
    parentProcess = processNumber;
}

//SIGINT handler (Stops currently running process by interrupting execvp)
void sigintHandler(int signal){

}

//Handler when something from background tries to get terminal control
void sigttouHandler(int signal){
    
}

//SIGSTOP handler (stops foreground command)
void sigtstpHandler(int signal){
    
}

//SIGCHILD handler
void sigChildHandler(int signal){
    pid_t pidCh;
    int status;

    //Get pid of any reaped children
    pidCh = waitpid(-1, &status, WNOHANG);

    //Continue reaping children while there are children to reap
    while(pidCh > 0){
        reapChild(pidCh);
        pidCh = waitpid(-1, &status, WNOHANG);
    }
}

//Init signals
void initSignals(){
    signal(SIGINT, SIG_IGN); //Ignore sig int for parent
    signal(SIGTSTP, SIG_IGN); //Ignore sig tstp for parent
    signal(SIGTTOU, SIG_IGN); //Ignore this everywhere
    signal(SIGCHLD, sigChildHandler); //SIGCHILD handler
}

