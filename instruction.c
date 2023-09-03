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
        if(debugIns){printf("PARENT PROCESS: [%d] parent of [%d]\n", myPid, curPid);}

        //Status of child process
        int status; 

        //-----------------------------------------------------------------------------------
        //Fork second instruction for pipelining (if it exists)
        //-----------------------------------------------------------------------------------
        if(exeIns.num > 1){
            //Fork second instruction
            pid_t curPid2 = fork();
            pid_t myPid2;

            //-----------------------------------------------------------------------------------
            //Second parent process (fork returns child pid value in curPid)
            //-----------------------------------------------------------------------------------
            if (curPid2 > 0) { 
                myPid2 = getpid();
                if(debugIns){printf("PARENT PROCESS 2: [%d] parent of [%d]\n", myPid2, curPid2);}

                //Status of 2nd child process
                int status2; 

                //Close pipes (Parent doesn't need pipes)
                close(pipes[0]);
                close(pipes[1]);

                //Child process running
                if(!exeIns.background){
                    //Wait for child process, set status WUNTRACED is set flag to wait
                    if(debugIns){printf("Wait for child2\n");}
                    waitpid(curPid2, &status2, WUNTRACED); 
                } else {
                    //Don't wait
                    if(debugIns){printf("No wait for child2\n");}
                    waitpid(curPid2, &status2, WNOHANG); 
                }
                //Child process done 
                if(debugIns){printf("[%d] Child2 done\n", curPid2);}
            //-----------------------------------------------------------------------------------
            //Second Child process
            //-----------------------------------------------------------------------------------
            } else if (curPid2 == 0) {
                myPid2 = getpid();
                if(debugIns){printf("START CHILD PROCESS2: [%d] INFO: pipeBool %d, exeIns.num %d, command %s\n", myPid2, pipeBool, 1, exeIns.insList[1].args[0]);}

                //Pipe stdin of first command from read end of pipe
                if(pipeBool == 1){
                    if(debugIns){printf("read pipe %d ", pipes[0]);}
                    close(pipes[1]); //Write not needed
                    dup2(pipes[0], 0); //For stdin of second command, read from pipe[0]
                    close(pipes[0]); //Read done
                    if(debugIns){printf("-pipe done\n ");}
                }
                //Set stdin
                else if(exeIns.insList[1].stdin.type == TOFILE){
                    //Try to open file
                    if(open(exeIns.insList[1].stdin.stdinFileName, O_RDONLY) == -1){
                    //Return error, stdin file doesn't exist
                    if(debugIns){printf("stdin file not found");}
                    return -1;
                    }

                    //Set file desciptor
                    int fileDescriptor2 = open(exeIns.insList[1].stdout.stdoutFileName, O_WRONLY);

                    //If successful (file exists), set stdin
                    dup2(fileDescriptor2, 0);
                }
                //Set stdout
                if(exeIns.insList[1].stdout.type == TOFILE){
                    if(debugIns){printf("stdout file: %s\n", exeIns.insList[1].stdout.stdoutFileName);}
                    //Try to open file
                    if(open(exeIns.insList[1].stdout.stdoutFileName, O_RDONLY) == -1){
                        //stdout file doesn't exist, create file
                        open(exeIns.insList[1].stdout.stdoutFileName, O_CREAT | O_WRONLY);
                    }

                    //Set file desciptor
                    int fileDescriptor2 = open(exeIns.insList[1].stdout.stdoutFileName, O_WRONLY);

                    //Set stdout
                    dup2(fileDescriptor2, 1);
                }
                //Set stderr
                if(exeIns.insList[1].stderr.type == TOFILE){
                    //Try to open file
                    if(open(exeIns.insList[1].stderr.stderrFileName, O_RDONLY) == -1){
                        //stdout file doesn't exist, create file
                        open(exeIns.insList[1].stderr.stderrFileName, O_CREAT | O_WRONLY);
                    }

                    //Set file desciptor
                    int fileDescriptor2 = open(exeIns.insList[1].stderr.stderrFileName, O_WRONLY);

                    //Set stderr
                    dup2(fileDescriptor2, 2);
                }

                //Execute child process (fork returns 0 in curPid)
                if(debugIns){printf("Child2: [%d] calls execvp with cmd: %s\n", myPid, exeIns.insList[1].args[0]);} 
                execvp(exeIns.insList[1].args[0], exeIns.insList[1].args);
                if(debugIns){printf("\ninstruction failed to execute\n");} //Fix (implemented): args needs to be null terminated
            }  
            //Failed fork
            else {
                if(debugIns){printf("Fork 2 failed");}
                return -1;
            }
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


        //Child process done 
        if(debugIns){printf("[%d] Child done\n", curPid);}
    }
    //-----------------------------------------------------------------------------------
    //Child process
    //-----------------------------------------------------------------------------------
    else if (curPid == 0) {
    //Data for child process
        myPid = getpid();
        if(debugIns){printf("START CHILD PROCESS: [%d] INFO: pipeBool %d, exeIns.num %d, command %s\n", myPid, pipeBool, exeIns.num, exeIns.insList[0].args[0]);}

        //Set dup/dup2 to change stdin/stdout/stderr, take piping into consideration
        
        //Set stdin
        if(exeIns.insList[0].stdin.type == TOFILE){
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

        //Pipe stdout of first command into write end of pipe (if pipelining)
        if(pipeBool == 1){
            if(debugIns){printf("write pipe %d ", pipes[1]);}
            close(pipes[0]); //Read not needed
            dup2(pipes[1], 1); //For stdout of first command, write to pipe[1]
            close(pipes[1]); //Write done
            if(debugIns){printf("-pipe done\n ");}
        }
        //Set stdout
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
        //Set stderr
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
        if(debugIns){printf("Child: [%d] calls execvp with cmd: %s\n", myPid, exeIns.insList[0].args[0]);} 
        execvp(exeIns.insList[0].args[0], exeIns.insList[0].args);
        if(debugIns){printf("\ninstruction failed to execute\n");} //Fix (implemented): args needs to be null terminated
        
    } else {
        //Failed fork
        if(debugIns){printf("Fork failed");}
        return -1;
    }
    
    return 0;
}

//NOTE: ISSUE: second process executes before the first process. lets say for ls | cat, cat executes first and hangs, but ls executes and finishes, pipelining not successful since cat tries to read before ls writes
//FIX FOUND: Closing pipes isn't transferred between parent and child, for each active thread, all pipes need to be closed at the end. Child1 closes read pipe (it writes stdout to pipe) and then write pipe after it writes, child2 closes write pipe (it reads from pipe into stdin) and then read pipe after it reads. After forking both children, parent closes both pipes to prevent hanging