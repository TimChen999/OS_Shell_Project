#include <stdio.h>
#include <stdbool.h>
#include <readline/readline.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include "jobs.h"
#include "parse.h" //Dont call headers in headers

bool debugJob = true;

//Global vars
struct job jobList[20]; //active
struct job finished[200]; //done
pid_t foreground;
int numJobs;
int numFinished;
bool jobsInitialized = false;

//Array of PIDs from SIGCHILD handler, whenever two pids within the list match the two pids of a job, remove the job
pid_t reapedPIDs[500];
int numReaped;

//Adds PID to reaped PID list and test if any job can be removed
int reapChild(pid_t pid){
    if(debugJob){printf("JOBS.C: Reap child called: PID: %d numJobs:", pid);}
    if(debugJob){printf(" %d numReaped: %d\n", numJobs, numReaped);}

    reapedPIDs[numReaped] = pid;
    numReaped++;

    //check if any job PIDs can be reaped (1 child)
    for(int i = 0; i < numReaped; i++){
        for(int x = 0; x < numJobs; x++){
            //Matching PID found
            if(reapedPIDs[i] == jobList[x].process1 && jobList[x].process2 == 0){
                if(debugJob){printf("JOBS.C: Reap job p1 [%d] p2 [0]\n", jobList[x].process1);}
                finishJob(jobList[x].process1, 99);
            }
        }       
    }

    //check if any job PIDs can be reaped (test all combinations of 2 PIDs)
    for(int i = 0; i < numReaped; i++){
        for(int j = i + 1; j < numReaped; j++){
            //For combination of PIDs, check thru list of active jobs
            for(int x = 0; x < numJobs; x++){
                //Matching PIDs found (2 children)
                if(reapedPIDs[i] == jobList[x].process1 && reapedPIDs[j] == jobList[x].process2){
                    if(debugJob){printf("JOBS.C: Reap job p1 [%s] p2 [%s]\n", jobList[x].process1, jobList[x].process2);}
                    finishJob(jobList[x].process1, 99);
                }
                else if(reapedPIDs[i] == jobList[x].process2 && reapedPIDs[j] == jobList[x].process1){
                    if(debugJob){printf("JOBS.C: Reap job p1 [%s] p2 [%s]\n", jobList[x].process1, jobList[x].process2);}
                    finishJob(jobList[x].process1, 99);
                }
            }
        }
    }
    return 0;
}


//Initialize jobs
int initJobs(){
    if(!jobsInitialized){
        if(debugJob){printf("JOBS.C: Init\n");}
        numJobs = 0;
        numFinished = 0;
        numReaped = 0;
        jobsInitialized = true;
    }
    return 0;
}

//Add job to active job list
int addJob(int numChild, pid_t pid1, pid_t pid2, char** args1, char** args2, bool background, bool stopped){
    if(debugJob){printf("JOBS.C: Add Job PID: [%d][%d]\n", pid1, pid2);}
    struct job curr = {numChild, pid1, pid2, args1, args2, background, stopped};
    jobList[numJobs] = curr;
    numJobs++;

    return 0;
}

//Job done, remove from active job list
int finishJob(pid_t pid1, int signal){
    if(debugJob){printf("JOBS.C: Child finished signal: %d\n", signal);}

    //Recieved SIGTSTP, don't terminate process
    if(signal == SIGTSTP){
        if(debugJob){printf("JOBS.C: Stop Job GPID: %d\n", pid1);}
        setStopped(pid1);
        return -1;
    }

    //Process not stopped
    if(debugJob){printf("JOBS.C: Finish Job GPID: %d\n", pid1);}
    int index = 0; //Index of removed job
    for(int i = 0; i < numJobs; i++){
        if(jobList[i].process1 == pid1){
            if(debugJob){printf("JOBS.C: Remove: %d\n", pid1);}
            //Finish job, add to finished list
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

//Sets a new pid as foreground, returns previous PID
pid_t setForeground(pid_t pid){
    //Test, get groupid of terminal, set to foreground (Plan: do this in a separate function which takes in a groupID as parameter)
    pid_t foregroundProcess = tcgetpgrp(STDIN_FILENO);
    if(debugJob){printf("JOBS.C: Foreground Job GPID: %d\n", foregroundProcess);}
    if(tcsetpgrp(STDOUT_FILENO, pid) != 0){
        if(debugJob){printf("Set GPID fail\n");}
    }
    foregroundProcess = tcgetpgrp(STDIN_FILENO);
    if(debugJob){printf("JOBS.C: New foreground Job GPID: %d\n", foregroundProcess);}
}

//Find index of most recent background or stopped process
int mostRecentStopped(){
    if(debugJob){printf("JOBS.C: Find most recent stopped: ");}

    //Find most recent
    for(int i = numJobs; i >= 0; i--){
        if(jobList[i].stopped == true && jobList[i].background == true){
            if(debugJob){printf("%d\n", i);}
            return i;
        }
    }

    //None found
    return -1;
}

//Set job to stopped, keeps track of stopped jobs
int setStopped(pid_t pid){
    for(int i = 0; i < numJobs; i++){
        if(pid == jobList[i].process1){
            if(debugJob){printf("JOBS.C: Set stopped: [%d] at index %d", pid, i);}
            jobList[i].stopped = true;
        }
    }
}

//Execute the "bg" instruction 
int exeBg(){
    int recent = mostRecentStopped();
    jobList[recent].stopped = false;

    //Continue the process
    kill(jobList[recent].process1, SIGCONT);
    kill(jobList[recent].process2, SIGCONT);
}

//Execute fg/bg/jobs (for these, the command itself is not added as an arg)
int exeSpecialJob(char* cmd){
    //FG
    if(strcmp(cmd, "fg") == 0){
        //Foreground
        if(debugJob){printf("Job: fg\n");}
        //Implement
    }

    //BG
    if(strcmp(cmd, "bg") == 0){
        //Background
        if(debugJob){printf("Job: bg\n");}
        //Implement
        exeBg();
    }

    //Jobs
    if(strcmp(cmd, "jobs") == 0){
        //Print jobs
        if(debugJob){printf("Job: jobs\n");}
        for(int i = 0; i < numJobs; i++){
            //Print each job
            printf("Active Job: %s Stopped: %d\n",jobList[i].c1Args[0], jobList[i].stopped);
        }

        //Print finished jobs
        for(int i = 0; i < numFinished; i++){
            //Print each job
            printf("Finished Job: %s\n",finished[i].c1Args[0]);
        }
        numFinished = 0;

    }

    return 1;
}