#include <stdio.h>`
#include <stdbool.h>
#include <readline/readline.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

void sigintHandler(int signal);

void sigtstpHandler(int signal);

void sigttouHandler(int signal);

void sigChildHandler(int signal);

int resetProcess();

int setForegroundProcess(pid_t processNumber, pid_t processNumber2);

int setParentProcess(pid_t processNumber);

int setForegroundProcess1(pid_t processNumber);

int setForegroundProcess2(pid_t processNumber);

void setHandlers();

