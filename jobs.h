#include <stdio.h>
#include <stdbool.h>
#include <readline/readline.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

//Keeps track of jobs while signal keeps track of processes

//Whether a job is foreground or background. inctruction should call jobs, which tracks the job. Jobs should also call signal.c to move jobs to foreground and background

//Stores data about a job (can have one or two processes)
struct job{
    int numChild;
    pid_t process; //Child process
    bool background;
    bool stopped;
};

pid_t setForeground(pid_t pid);

int initJobs();

int addJob(int numChild, pid_t groupPID, bool background, bool stopped);

int finishJob(pid_t groupPID);

int setStopped(pid_t groupPID);


