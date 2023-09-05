#include <stdio.h>
#include <stdbool.h>
#include <readline/readline.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

void sigintHandler(int signal);

void sigstopHandler(int signal);

int resetProcess();

int addForegroundProcess(pid_t processNumber);

int addBackgroundProcess(pid_t processNumber);

int setParentProcess(pid_t processNumber);

int sigInit();

