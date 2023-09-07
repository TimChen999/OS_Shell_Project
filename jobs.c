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

struct job jobList[20]; //active
struct job finished[200]; //done
pid_t foreground;
int numJobs;
int numFinished;

//Use tcsetpgup set fore/background

//Initialize jobs
int initJobs(){
    numJobs = 0;
    numFinished;
    return 0;
}

//Add job to active job list
int addJob(int numChild, pid_t groupPID, bool background, bool stopped){
    if(debugJob){printf("\nAdd Job GPID: %d\n", groupPID);}
    struct job curr = {numChild, groupPID, background, stopped};
    jobList[numJobs] = curr;
    numJobs++;

    return 0;
}

//Job done, remove from active job list
int finishJob(pid_t groupPID){
    //Process is stopped, return handler
    if(getStopped()){
        if(debugJob){printf("\nStop Job GPID: %d\n", groupPID);}
        setStopped(false);
        return 1;
    }

    //Process not stopped
    if(debugJob){printf("\nFinish Job GPID: %d\n", groupPID);}
    int index = 0; //Index of removed job
    for(int i = 0; i < numJobs; i++){
        if(jobList[i].process == groupPID){
            if(debugJob){printf("Remove: %d", groupPID);}
            finished[numFinished] = jobList[i];
            numFinished++;
            index = i;
        }
    }

    //Move all elements down one
    index++;
    for(; index < numJobs; index++){
        jobList[index - 1] = jobList[index];
    }
    numJobs--;
    return 0;
}

//Track stopped job
int setStopped(pid_t groupPID){
    for(int i = 0; i < numJobs; i++){
        if(jobList[i].process == groupPID){
            if(debugJob){printf("Stop: %d", groupPID);}
            jobList[i].stopped = true;
            jobList[i].background = true;
        }
    }
}

//Resume a stopped job, bring to foreground
int setResumeForeground(pid_t groupPID){
    for(int i = 0; i < numJobs; i++){
        if(jobList[i].process == groupPID){
            if(debugJob){printf("Stop: %d", groupPID);}
            jobList[i].stopped = false;
            jobList[i].background = false;
        }
    }
}

//Sets a new pid as foreground, returns previous PID
pid_t setForeground(pid_t pid){
    //Test, get groupid of terminal, set to foreground (Plan: do this in a separate function which takes in a groupID as parameter)
    pid_t foregroundProcess = tcgetpgrp(STDIN_FILENO);
    if(debugJob){printf("Foreground Job GPID: %d\n", foregroundProcess);}
    if(tcsetpgrp(STDOUT_FILENO, pid) != 0){
        if(debugJob){printf("Set GPID fail\n");}
    }
    foregroundProcess = tcgetpgrp(STDIN_FILENO);
    if(debugJob){printf("New foreground Job GPID: %d\n", foregroundProcess);}
}

//Sends most recent background/stopped process to foreground
int sendRecentToFore(){
    if(debugJob){printf("send most recent stopped job to foreground");}

    int mostRecentStopped = 999;
    for(int i = numJobs; i >= 0; i--){
        if(jobList[i].stopped == true){
            mostRecentStopped = i;
        }
    }
    //No stopped processes
    if(mostRecentStopped == 999){
        return 1; //Not done
    }
    //Set foreground
    else{
        jobList[mostRecentStopped].stopped = false;
        jobList[mostRecentStopped].background = false;

        //IMPLEMENT: Resume the process in foreground
        setForeground(jobList[mostRecentStopped].process);
        return 0; //Done
    }
}

//Resume stopped process
int resumeRecentInBack(){
    if(debugJob){printf("resume most recent stopped job in background");}

    int mostRecentStopped = 999;
    for(int i = numJobs; i >= 0; i--){
        if(jobList[i].stopped == true){
            mostRecentStopped = i;
        }
    }
    //No stopped processes
    if(mostRecentStopped == 999){
        return 1; //Not done
    }
    //Set background
    else{
        jobList[mostRecentStopped].stopped = false;
        jobList[mostRecentStopped].background = true;

        //IMPLEMENT: resume in background
        
        return 0; //Done
    }

    return 0;
}

//Execute fg/bg/jobs (for these, the command itself is not added as an arg)
int exeJob(struct execution exeIns){
    //FG
    if(strcmp(exeIns.insList[0].command, "fg") == 0){
        //Foreground
        if(debugJob){printf("Job: fg");}
        return sendRecentToFore();
    }

    //BG
    if(strcmp(exeIns.insList[0].command, "bg") == 0){
        //Background
        if(debugJob){printf("Job: bg");}
        return resumeRecentInBack();
    }

    //Jobs
    if(strcmp(exeIns.insList[0].command, "jobs") == 0){
        //Print jobs
    }

    return 1;
}

//Make a groupd id, setpgid(0, 0) in first child (same as child's ID) then setpgid(0, pgid) in second child

//ctrlz to make foreground command go to background and stop, fg will bring any background or stopped command to run in foreground, bg will bring command to background