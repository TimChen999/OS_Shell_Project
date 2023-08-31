#include <stdio.h>
#include <readline/readline.h>
#include <stdbool.h>
#include "parse.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

bool debugIns = true;
//Execute instructions
//Executes max of 2 processes (first two entered)
int executeInstructions(struct execution exeIns){
    pid_t curPid = getpid(); 
    pid_t myPid;
    if(debugIns){printf("Execute %d Instructions, Current pid: %d\n", exeIns.num, curPid);}

    //Fork command (If parent, curPid becomes child's pid, if child, curPid becomes zero)
    curPid = fork();

    //If else condition for whether each thread is parent or child
    if (curPid > 0) { 
        //Parent process (fork returns child pid value in curPid)
        myPid = getpid();
        if(debugIns){printf("[%d] parent of [%d]\n", myPid, curPid);}

        //Status of child process
        int status; 

        //Fork second instruction for pipelining (if it exists)
        if(exeIns.num > 1){
            //Set and execute second instruction
            struct instruction secondInstruction[1] = {exeIns.insList[1]};
            struct execution exeSecond = {secondInstruction, exeIns.status, 1, exeIns.background};
            
            //Go back to executeInstructions with second instruction
            executeInstructions(exeSecond);
        }

        //Child process running
        if(!exeIns.background){
            //Wait for child process, set status WUNTRACED is set flag to wait
            if(debugIns){printf("Wait for child\n");}
            waitpid(curPid, &status, WUNTRACED); 
        } else {
            //Don't wait
            if(debugIns){printf("No wait for child\n");}
            waitpid(curPid, &status, WNOHANG); 
        }

        //Child process done (Note, if first child finishes before second, executeInstructions(exeSecond) will not be done before second child finishes execution, statement will always print second child as done first)
        if(debugIns){printf("%d Child done\n", curPid);}

    //Child process
    } else if (curPid == 0) { 
        //Set dup/dup2 to change stdin/stdout/stderr
        if(exeIns.insList[0].stdin.type == TOFILE){
            //Try to open file
            if(open(exeIns.insList[0].stdin.stdinFileName, O_RDONLY) == -1){
                //Return error, stdin file doesn't exist
                if(debugIns){printf("Child process not found");}
                return -1;
            }

            //If successful (file exists), set stdin
            dup2(exeIns.insList[0].stdin.stdinFileName, 0);
        }   
        if(exeIns.insList[0].stdout.type == TOFILE){
            //Try to open file
            if(open(exeIns.insList[0].stdout.stdoutFileName, O_RDONLY) == -1){
                //stdout file doesn't exist, create file
                open(exeIns.insList[0].stdout.stdoutFileName, O_CREAT | O_WRONLY);
            }

            //Set stdout
            dup2(exeIns.insList[0].stdout.stdoutFileName, 1);
        }
        if(exeIns.insList[0].stderr.type == TOFILE){
            //Try to open file
            if(open(exeIns.insList[0].stderr.stderrFileName, O_RDONLY) == -1){
                //stdout file doesn't exist, create file
                open(exeIns.insList[0].stderr.stderrFileName, O_CREAT | O_WRONLY);
            }

            //Set stdout
            dup2(exeIns.insList[0].stderr.stderrFileName, 1);
        }

        //Execute child process (fork returns 0 in curPid)
        myPid = getpid();
        if(debugIns){printf("[%d] child\n", myPid);}
        execvp(exeIns.insList[0].args[0], exeIns.insList[0].args);
        if(debugIns){printf("\ninstruction failed to execute\n");} //Fix (implemented): args needs to be null terminated
        
    } else {
        //Failed fork
        if(debugIns){printf("Fork failed");}
        return -1;
    }
    
    return 0;
}

