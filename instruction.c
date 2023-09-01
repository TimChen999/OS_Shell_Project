#include <stdio.h>
#include <readline/readline.h>
#include <stdbool.h>
#include "parse.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

//ISSUE: further explore piping with cat, ls | cat appears to hang (maybe some sort of deadlock?)
bool debugIns = true;

//Execute instructions
//Executes max of 2 processes (first two entered)
int executeInstructions(struct execution exeIns, bool pipeBool, int pipes[2]){
    pid_t curPid = getpid(); 
    pid_t myPid;
    if(debugIns){printf("Execute %d Instructions, Current pid: %d\n", exeIns.num, curPid);}

    //use pipeline if needed (number of process > 1)
    if(exeIns.num > 1){
        pipe(pipes);
        if(debugIns){printf("Make pipeline: pipes[0]: %d, pipes[1]: %d\n", pipes[0], pipes[1]);} //Only need to be done once, not done on second call of function
    }

    //Fork command (If parent, curPid becomes child's pid, if child, curPid becomes zero)
    curPid = fork();

    //If else condition for whether each thread is parent or child
    //-----------------------------------------------------------------------------------
    //Parent process (fork returns child pid value in curPid)
    //-----------------------------------------------------------------------------------
    if (curPid > 0) { 
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
            executeInstructions(exeSecond, true, pipes);
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
        close(pipes);

    //-----------------------------------------------------------------------------------
    //Child process
    //-----------------------------------------------------------------------------------
    } else if (curPid == 0) { 
        //Data for child process
        myPid = getpid();
        if(debugIns){printf("[%d] child, pipeBool %d, exeIns.num %d\n", myPid, pipeBool, exeIns.num);}

        //Set dup/dup2 to change stdin/stdout/stderr, take piping into consideration
        //Pipe stdin of first command from read end of pipe
        if(pipeBool == 1 && exeIns.num == 1){
            if(debugIns){printf("Pipe stdin of [%d], command: %s into pipes[0]: %d\n", myPid, exeIns.insList[1], pipes[0]);}
            dup2(pipes[0], 0); //For stdin of second command, read from pipe[0]
            if(debugIns){printf("Finished pipe stdin of [%d]\n", myPid);}
        }
        //Normally set stdin
        else if(exeIns.insList[0].stdin.type == TOFILE){
            //Try to open file
            if(open(exeIns.insList[0].stdin.stdinFileName, O_RDONLY) == -1){
                //Return error, stdin file doesn't exist
                if(debugIns){printf("stdin file not found");}
                return -1;
            }

            //Set file desciptor
            int fileDescriptor = open(exeIns.insList[0].stdout.stdoutFileName, O_WRONLY);

            //If successful (file exists), set stdin
            dup2(fileDescriptor, 0);
        }   
        //Pipe stdout of first command into write end of pipe
        if(pipeBool == 1 && exeIns.num > 1){
            if(debugIns){printf("Pipe stdout of [%d], command: %s into pipes[1]: %d\n", myPid, exeIns.insList[0], pipes[1]);}
            dup2(pipes[1], 1); //For stdout of first command, write to pipe[1]
            if(debugIns){printf("Finished pipe stout of [%d]\n", myPid);}
        }
        else if(exeIns.insList[0].stdout.type == TOFILE){
            if(debugIns){printf("stdout file: %s\n", exeIns.insList[0].stdout.stdoutFileName);}
            //Try to open file
            if(open(exeIns.insList[0].stdout.stdoutFileName, O_RDONLY) == -1){
                //stdout file doesn't exist, create file
                open(exeIns.insList[0].stdout.stdoutFileName, O_CREAT | O_WRONLY);
            }

            //Set file desciptor
            int fileDescriptor = open(exeIns.insList[0].stdout.stdoutFileName, O_WRONLY);

            //Set stdout
            dup2(fileDescriptor, 1);
        }
        if(exeIns.insList[0].stderr.type == TOFILE){
            //Try to open file
            if(open(exeIns.insList[0].stderr.stderrFileName, O_RDONLY) == -1){
                //stdout file doesn't exist, create file
                open(exeIns.insList[0].stderr.stderrFileName, O_CREAT | O_WRONLY);
            }

            //Set file desciptor
            int fileDescriptor = open(exeIns.insList[0].stderr.stderrFileName, O_WRONLY);

            //Set stderr
            dup2(fileDescriptor, 2);
        }

        //Execute child process (fork returns 0 in curPid)
        if(debugIns){printf("Child: [%d] calls execvp\n", myPid);} 
        execvp(exeIns.insList[0].args[0], exeIns.insList[0].args);
        if(debugIns){printf("\ninstruction failed to execute\n");} //Fix (implemented): args needs to be null terminated
        
    } else {
        //Failed fork
        if(debugIns){printf("Fork failed");}
        return -1;
    }
    
    return 0;
}

