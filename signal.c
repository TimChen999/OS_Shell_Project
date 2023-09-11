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

bool sigDebug = true;

//Parent progress
pid_t parentProcess; 

//Can be a max of 2 foreground processes
pid_t foregroundProcess; 
pid_t foregroundProcess2;

//Whether process was stopped before finishing
bool stopped;

int resetProcess(){
    if(sigDebug){printf("SIGNAL.C: resetProcess()\n");}
    foregroundProcess = 0;
    foregroundProcess2 = 0;
}

int setForegroundProcess(pid_t processNumber, pid_t processNumber2){
    if(sigDebug){printf("SIGNAL.C: set foreground [%d][%d]\n", processNumber, processNumber2);}
    foregroundProcess = processNumber;
    foregroundProcess2 = processNumber2;
}

int setForegroundProcess1(pid_t processNumber){
    if(sigDebug){printf("SIGNAL.C: set foreground1 [%d]\n", processNumber);}
    foregroundProcess = processNumber;
}

int setForegroundProcess2(pid_t processNumber){
    if(sigDebug){printf("SIGNAL.C: set foreground2 [%d]\n", processNumber);}
    foregroundProcess2 = processNumber;
}

int setParentProcess(pid_t processNumber){
    if(sigDebug){printf("SIGNAL.C: set parent [%d]\n", processNumber);}
    parentProcess = processNumber;
}

//SIGINT handler (Stops currently running process by interrupting execvp)
void sigintHandler(int signal){
    kill(foregroundProcess, SIGINT);
    kill(foregroundProcess2, SIGINT);
}

//Handler when something from background tries to get terminal control
void sigttouHandler(int signal){
    
}

//SIGSTOP handler (stops foreground command)
void sigtstpHandler(int signal){
    kill(foregroundProcess, SIGTSTP);
    kill(foregroundProcess2, SIGTSTP);
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

//Set signal handlers
void setHandlers(){
    signal(SIGINT, sigintHandler); //Set sig int handler
    signal(SIGTSTP, sigtstpHandler); //Set sig tstp handler
    signal(SIGTTOU, SIG_IGN); //Ignore this everywhere
    signal(SIGCHLD, sigChildHandler); //SIGCHILD handler
}

