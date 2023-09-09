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

//Stores data about a job (can have one or two processes) (ADD: keep track of args)
struct job{
    int numChild;
    pid_t process1; //Child process
    pid_t process2;
    char** c1Args;
    char** c2Args;
    bool background;
    bool stopped;
};

pid_t setForeground(pid_t pid);

int initJobs();

int addJob(int numChild, pid_t pid1, pid_t pid2, char** args1, char** args2, bool background, bool stopped);

int finishJob(pid_t pid1, int signal);

int exeSpecialJob(char* cmd);

int mostRecentStopped();

int setStopped();

int reapChild(pid_t pid);

