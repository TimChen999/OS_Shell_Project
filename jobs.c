#include <stdio.h>
#include <stdbool.h>
#include <readline/readline.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include "jobs.h"

bool debugJob = true;

struct job jobList[20];
pid_t foreground;
int numJobs;

//Tcsetpgup set fore/background

int initJobs(){
    numJobs = 0;

    return 0;
}

int addJob(int numChild, pid_t groupPID, bool background, bool stopped){
    if(debugJob){printf("\nAdd Job GPID: %d\n", groupPID);}
    struct job curr = {numChild, groupPID, background, stopped};
    jobList[numJobs] = curr;
    numJobs++;

    //Test, get groupid of terminal, set to foreground
    pid_t foregroundProcess = tcgetpgrp(STDIN_FILENO);
    if(debugJob){printf("Foreground Job GPID: %d\n", foregroundProcess);}
    if(tcsetpgrp(STDOUT_FILENO, groupPID) != 0){
        if(debugJob){printf("Set GPID fail\n");}
    }
    foregroundProcess = tcgetpgrp(STDIN_FILENO);
    if(debugJob){printf("New foreground Job GPID: %d\n", foregroundProcess);}

    return 0;
}

//Send forground job to background
int sendToBack(){
    //Get process in foreground
    pid_t foregroundProcess = tcgetpgrp(STDIN_FILENO);
    


    return 0;
}

//Sends most recent background/stopped process to foreground


//Make a groupd id, setpgid(0, 0) in first child (same as child's ID) then setpgid(0, pgid) in second child

//ctrlz to make foreground command go to background and stop, fg will bring any background or stopped command to run in foreground, bg will bring command to background