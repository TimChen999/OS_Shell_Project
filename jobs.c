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

bool debugJob = false;

//Global vars
static struct job jobList[20]; //active
static struct job finished[200]; //done
static pid_t foreground;
static int numJobs;
static int numFinished;
static bool jobsInitialized = false;

//Keeps track of jobNumber, each job has unique number
static int jobNumber;

//Array of PIDs from SIGCHILD handler, whenever two pids within the list match the two pids of a job, remove the job
static pid_t reapedPIDs[500];
static int numReaped;

//Most recent job index
int recentJob;

//Removed pid from reaped pid list(job associated with pid has been removed)
int removeReapedPid(pid_t pid){
    //Find index of PID
    int index = -1;
    for(int i = 0; i < numReaped; i++){
        if(pid == reapedPIDs[i]){
            if(debugJob){printf("JOBS.C removeReapedPid(pid_t pid): Remove [%d] at index %d\n", pid, i);}
            index = i;
        }
    }

    //PID not found
    if(index == -1){
        return -1;
    }

    //Remove PID from list (move all elements after up)
    index++;
    for(;index < numReaped; index++){
        reapedPIDs[index - 1] = reapedPIDs[index];
    }
    numReaped--;
    return 0;
}

//Adds PID to reaped PID list and test if any job can be removed (IMPLEMENT: REMOVE PIDS THAT HAVE BEEN REAPED AND RESPECTIVE JOBS HAVE BEEN REMOVED)
int reapChild(pid_t pid){
    if(debugJob){printf("JOBS.C reapChild(pid_t pid): Reap child called: PID: %d numJobs:", pid);}

    reapedPIDs[numReaped] = pid;
    numReaped++;

    if(debugJob){printf(" %d numReaped: %d\n", numJobs, numReaped);}

    //check if any job PIDs can be reaped (1 child)
    for(int i = 0; i < numReaped; i++){
        for(int x = 0; x < numJobs; x++){
            //Matching PID found
            if(reapedPIDs[i] == jobList[x].process1 && jobList[x].process2 == 0){
                if(debugJob){printf("JOBS.C reapChild(pid_t pid): Reap job p1 [%d] p2 [0]\n", jobList[x].process1);}
                removeReapedPid(reapedPIDs[i]);
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
                    if(debugJob){printf("JOBS.C reapChild(pid_t pid): Reap job p1 [%d] p2 [%d]\n", jobList[x].process1, jobList[x].process2);}
                    removeReapedPid(reapedPIDs[i]);
                    removeReapedPid(reapedPIDs[j]);
                    finishJob(jobList[x].process1, 99);
                }
                else if(reapedPIDs[i] == jobList[x].process2 && reapedPIDs[j] == jobList[x].process1){
                    if(debugJob){printf("JOBS.C reapChild(pid_t pid): Reap job p1 [%d] p2 [%d]\n", jobList[x].process1, jobList[x].process2);}
                    removeReapedPid(reapedPIDs[i]);
                    removeReapedPid(reapedPIDs[j]);
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
        jobNumber = 1;
        recentJob = -1;
    }
    return 0;
}

//Add job to active job list
int addJob(int numChild, pid_t pid1, pid_t pid2, char** args1, char** args2, bool background, bool stopped){
    if(debugJob){printf("JOBS.C addJob(): Add Job PID: [%d][%d], numChild: %d ", pid1, pid2, numChild);}

    //Create new job with the params
    struct job curr = {numChild, pid1, pid2, args1, args2, background, stopped, jobNumber};
    jobNumber++;

    //Add created job to local jobs list
    if(debugJob){printf("Add Job done, numJobs %d ", numJobs);}
    jobList[numJobs] = curr;
    numJobs++;

    if(debugJob){printf("--> %d\n ", numJobs);}

    return 0;
}

//Job done, remove from active job list
int finishJob(pid_t pid1, int signal){
    if(debugJob){printf("JOBS.C finishJob(): Child finished signal: %d ", signal);}

    //Recieved signal, don't terminate process but set to background and stop
    if(signal == SIGTSTP){
        if(debugJob){printf(" Stop Job GPID: %d num: %d \n", pid1, numJobs);}
        setStopped(pid1);
        setBackground(pid1);
        return -1;
    }

    //Process not stopped
    if(debugJob){printf(" finishJob(): Finish Job GPID: %d ", pid1);}
    int index = 0; //Index of removed job
    for(int i = 0; i < numJobs; i++){
        if(jobList[i].process1 == pid1){
            if(debugJob){printf("call finishJob() ", pid1);}
            //Finish job, add to finished list
            finished[numFinished] = jobList[i];
            numFinished++;
            index = i;
        }
    }

    //Move all elements down one
    if(debugJob){printf("remove index: %d ", index);}
    index++;
    for(; index < numJobs; index++){
        jobList[index - 1] = jobList[index];
    }
    if(debugJob){printf("num jobs: %d --> ", numJobs);}
    numJobs--;
    if(debugJob){printf("%d\n", numJobs);}
    return 0;
}

//Sets a new pid as foreground, returns previous PID
pid_t setForeground(pid_t pid){
    //Test, get groupid of terminal, set to foreground (Plan: do this in a separate function which takes in a groupID as parameter)
    pid_t foregroundProcess = tcgetpgrp(STDIN_FILENO);
    if(debugJob){printf("JOBS.C setForeground(pid_t pid): Foreground Job GPID: %d\n", foregroundProcess);}
    if(tcsetpgrp(STDOUT_FILENO, pid) != 0){
        if(debugJob){printf("Set GPID fail\n");}
    }
    foregroundProcess = tcgetpgrp(STDIN_FILENO);
    if(debugJob){printf("JOBS.C setForeground(pid_t pid): New foreground Job GPID: %d\n", foregroundProcess);}
}

//Find index of most recent background or stopped process
int mostRecentStopped(){
    if(debugJob){printf("JOBS.C mostRecentStopped(): Find most recent stopped: ");}

    //Find most recent
    for(int i = numJobs - 1; i >= 0; i--){
        if(jobList[i].stopped == true || jobList[i].background == true){
            if(debugJob){printf("%d\n", i);}
            return i;
        }
    }

    //Newline
    if(debugJob){printf(" not found \n");}

    //None found
    return -1;
}

//Set job to stopped, keeps track of stopped jobs
int setStopped(pid_t pid){
    if(debugJob){printf("JOBS.C setStopped(pid_t pid): Set stopped: look for [%d] in size %d \n", pid, numJobs);}
    for(int i = 0; i < numJobs; i++){
        if(pid == jobList[i].process1){
            if(debugJob){printf("JOBS.C setStopped(pid_t pid): Set stopped: [%d] at index %d \n", pid, i);}
            jobList[i].stopped = true;
        }
    }
}

//Set job to background, keeps track of background jobs
int setBackground(pid_t pid){
    if(debugJob){printf("JOBS.C: Set stopped setBackground(pid_t pid): look for [%d] in size %d \n", pid, numJobs);}
    for(int i = 0; i < numJobs; i++){
        if(pid == jobList[i].process1){
            if(debugJob){printf("JOBS.C setBackground(pid_t pid): Set background: [%d] at index %d \n", pid, i);}
            jobList[i].background = true;
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

    //Print the command resumed by BG
    printf("[%d]", jobList[recent].jobNumber);

    //Print +/- for most recent
    recentJob = numJobs - 1;
    if(recent == recentJob){
        printf("+ ");
    }
    else{
        printf("- ");
    }

    //Print running
    printf("Running\t");

    //Print args
    int index = 0;
    while(true){
        char* arg = jobList[recent].c1Args[index];
        //Break if null
        if(arg == NULL){
            break;
        }
        //Print arg
        printf("%s ", arg);
        index++;
    }
    //Pipelining
    if(jobList[recent].numChild > 1){
        printf("| ");
        index = 0;
        while(true){
            char* arg = jobList[recent].c2Args[index];
            //Break if null
            if(arg == NULL){
                break;
            }
            //Print arg
            printf("%s ", arg);
            index++;
        }
    }

    //Newline
    printf("\n");
}

//Execute the "fg" instruction 
int exeFg(){
    int recent = -1;
    recent = mostRecentStopped();
    jobList[recent].stopped = false;
    jobList[recent].background = false;

    //No processes found
    if(recent == -1){return -1;}

    //Found recent command
    if(debugJob){printf("JOBS.C exeFg(): found recent command: Index: %d Children: ", recent);}

    //Get processes of recent job
    pid_t process1 = jobList[recent].process1;
    pid_t process2 = jobList[recent].process2;

    int status;
    int status2;

    //Current pid to give back terminal control
    pid_t currentPID = getpid();

    //Print process args
    //Print args
    int index = 0;
    while(true){
        char* arg = jobList[recent].c1Args[index];
        //Break if null
        if(arg == NULL){
            break;
        }
        //Print arg
        printf("%s ", arg);
        index++;
    }
    //Pipelining
    if(jobList[recent].numChild > 1){
        printf("| ");
        index = 0;
        while(true){
            char* arg = jobList[recent].c2Args[index];
            //Break if null
            if(arg == NULL){
                break;
            }
            //Print arg
            printf("%s ", arg);
            index++;
        }
    }

    //Newline
    printf("\n");

    //Continue the process in foreground
    //Give temporary terminal control
    if(jobList[recent].numChild == 1){
        if(debugJob){printf("1 child [%d]\n", process1);}
        //Ungroup process
        setpgid(currentPID, currentPID);
        setpgid(process1, process1);

        //Resume process
        kill(process1, SIGCONT);

        //Set terminal control
        tcsetpgrp(STDIN_FILENO, process1);
        waitpid(process1, &status, WUNTRACED); 

        //Give terminal control back to parent
        tcsetpgrp(STDIN_FILENO, currentPID);

        //Check process result (end process if it finishes uninterrupted)
        int sig = WSTOPSIG(status);
        if(debugJob){printf("JOBS.C exeFg(): child 1 signal: %d \n", sig);}
        finishJob(process1, sig);
    }
    //2 Children pipeline
    else{
        if(debugJob){printf("2 children [%d][%d]\n", process1, process2);}
        //Group process
        setpgid(currentPID, currentPID);
        setpgid(process1, process1);
        setpgid(process2, process1);

        //Resume process
        kill(process1, SIGCONT);
        kill(process2, SIGCONT);

        //Set terminal control
        tcsetpgrp(STDIN_FILENO, process1);
        waitpid(process1, &status, WUNTRACED); 
        waitpid(process2, &status, WUNTRACED);

        //Give terminal control back to parent
        tcsetpgrp(STDIN_FILENO, currentPID);

        //Check process result (end process if it finishes uninterrupted)
        //If any of the two children are stopped by SIGTSTP, do not finish process, even if the other is finished
        int sig = WSTOPSIG(status);
        if(debugJob){printf("JOBS.C exeFg(): child 1 signal: %d ", sig);}
        if(sig != SIGTSTP){
            if(debugJob){printf("child 2 signal: %d\n", sig);}
            sig = WSTOPSIG(status2);
        }
        finishJob(process1, sig);
    }  
}

//Execute fg/bg/jobs (for these, the command itself is not added as an arg)
int exeSpecialJob(char* cmd){
    //FG
    if(strcmp(cmd, "fg") == 0){
        //Foreground
        if(debugJob){printf("Job: fg\n");}
        //Implement
        exeFg();
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
        //Print finished jobs
        recentJob = numJobs - 1;
        for(int i = 0; i < numFinished; i++){
            //Debug print
            //printf("Num: %d Finished Job: %s PIDs:[%d][%d]\n",finished[i].jobNumber ,finished[i].c1Args[0], finished[i].process1, finished[i].process2);

            //Print finished job
            printf("[%d]- Done\t", finished[i].jobNumber);
            
            //Print args
            int index = 0;
            while(true){
                char* arg = finished[i].c1Args[index];
                //Break if null
                if(arg == NULL){
                    break;
                }
                //Print arg
                printf("%s ", arg);
                index++;
            }
            //Pipelining
            if(finished[i].numChild > 1){
                printf("| ");
                index = 0;
                while(true){
                    char* arg = finished[i].c2Args[index];
                    //Break if null
                    if(arg == NULL){
                        break;
                    }
                    //Print arg
                    printf("%s ", arg);
                    index++;
                }
            }

            //Newline
            printf("\n");
        }

        //Print active jobs
        if(debugJob){printf("Job: %d jobs\n", numJobs);}
        for(int i = 0; i < numJobs; i++){
            //Debug print
            //printf("Num: %d Active Job: %s Stopped: %d Background: %d PIDs:[%d][%d]\n",jobList[i].jobNumber ,jobList[i].c1Args[0], jobList[i].stopped, jobList[i].background, jobList[i].process1, jobList[i].process2);

            //Print active job
            printf("[%d]", jobList[i].jobNumber);

            //Print +/- for most recent
            if(i == recentJob){
                printf("+ ");
            }
            else{
                printf("- ");
            }

            //Print running/stopped
            if(jobList[i].stopped == true){
                printf("Stopped\t");
            }
            else{
                printf("Running\t");
            }

            //Print args
            int index = 0;
            while(true){
                char* arg = jobList[i].c1Args[index];
                //Break if null
                if(arg == NULL){
                    break;
                }
                //print arg
                printf("%s ", arg);
                index++;
            }
            //Pipelining
            if(jobList[i].numChild > 1){
                printf("| ");
                index = 0;
                while(true){
                    char* arg = jobList[i].c2Args[index];
                    //Break if null
                    if(arg == NULL){
                        break;
                    }
                    //Print arg
                    printf("%s ", arg);
                    index++;
                }
            }
            //Newline
            printf("\n");
        }
        numFinished = 0;

    }

    return 1;
}