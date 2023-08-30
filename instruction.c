#include <stdio.h>
#include <readline/readline.h>
#include <stdbool.h>
#include "parse.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

bool debugIns = true;
//Execute instructions
//TODO: IMPLEMENT PIPELINING WITH FORK, Note: fork for every separate process (instruction struct) before calling execv
int executeInstructions(struct instruction* insList){
    pid_t curPid = getpid(); 
    pid_t myPid;
    printf("Current pid: %d\n", curPid);

    //Fork command
    curPid = fork();

    //If else for process (code from class)
    if (curPid > 0) { 
        //Parent process (fork returns child pid value in curPid)
        myPid = getpid();
        if(debugIns){printf("[%d] parent of [%d]\n", myPid, curPid);}
        int status; //Status of child process
        waitpid(curPid, &status, WUNTRACED); //Wait for child process, set status WUNTRACED is set flag to wait

    } else if (curPid == 0) { 
        //Child process (fork returns 0 in curPid)
        myPid = getpid();
        printf("[%d] child\n", myPid);
        execvp(insList[0].args[0], insList[0].args);
        if(debugIns){printf("\ninstruction failed to execute\n");} //Fix (implemented): args needs to be null terminated
        
    } else {
        //Fail
        perror("Fork failed");
    }
    
    return 0;
}

