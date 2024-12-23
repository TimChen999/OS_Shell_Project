#include <stdio.h>
#include <readline/readline.h>
#include <stdbool.h>
#include "parse.h"
#include "signalHeader.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include "signal.h"

bool debugIns = false;

//Store parent PID
pid_t parentPID;
pid_t child2PID;
pid_t child1PID;

//Executes Instructions, max of 2 processes (first two entered)
int executeInstructions(struct execution exeIns, bool pipeBool, int pipes[2]){
    pid_t curPid = getpid(); 
    parentPID = getpid();
    setpgid(parentPID, parentPID);
    pid_t myPid;
    if(debugIns){printf("Execute %d Instructions, Current pid: %d\n", exeIns.num, curPid);}

    //use pipeline if needed (number of process > 1)
    if(exeIns.num > 1){
        pipe(pipes);
        //if(debugIns){printf("Make pipeline: pipes[0]: %d, pipes[1]: %d\n", pipes[0], pipes[1]);} //Only need to be done once, not done on second call of function
    }

    //Fork command (If parent, curPid becomes child's pid, if child, curPid becomes zero)
    curPid = fork();

    //-----------------------------------------------------------------------------------
    //Parent process (fork returns child pid value in curPid)
    //-----------------------------------------------------------------------------------
    if (curPid > 0) { 
        myPid = getpid();
        if(debugIns){printf("PARENT PROCESS: [%d] parent of [%d]\n", myPid, curPid);}

        //Store value of child1's pid for future use
        child1PID = curPid; 

        //Add job to list (One child)
        if(exeIns.num == 1){
            addJob(exeIns.num, child1PID, 0, exeIns.insList[0].args, NULL, exeIns.background, false);  
        }

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
                if(debugIns){printf("PARENT PROCESS: [%d] parent of [%d]\n", myPid2, curPid2);}

                //Set child2 pid
                child2PID = curPid2;

                //Add job to list (2 children) has to be done in parent or values will not transfer
                addJob(exeIns.num, child1PID, child2PID, exeIns.insList[0].args, exeIns.insList[1].args, exeIns.background, false); 

                //group pgid, give terminal control
                if(true){
                    setpgid(parentPID, parentPID);
                    setpgid(child1PID, child1PID);
                    if(exeIns.num > 1){
                        setpgid(child2PID, child1PID);}
                    tcsetpgrp(STDIN_FILENO, child1PID);
                }

                //Close pipes (Parent doesn't need pipes)
                close(pipes[0]);
                close(pipes[1]);
            //-----------------------------------------------------------------------------------
            //Second Child process
            //-----------------------------------------------------------------------------------
            } else if (curPid2 == 0) {
                //Reset sigint to default in child
                signal(SIGINT, SIG_DFL); 
                signal(SIGTSTP, SIG_DFL);

                //set second child PID
                child2PID = getpid(); 

                //group pgid, give terminal control
                setpgid(parentPID, parentPID);
                setpgid(child1PID, child1PID);
                setpgid(child2PID, child1PID);
                if(!exeIns.background){
                    tcsetpgrp(STDIN_FILENO, child1PID);
                }

                myPid2 = getpid();
                if(debugIns){printf("CHILD PROCESS2: [%d] INFO: pipeBool %d, exeIns.num %d, command %s\n", myPid2, pipeBool, 1, exeIns.insList[1].args[0]);}

                //Pipe stdin of first command from read end of pipe
                if(pipeBool == 1){
                    //if(debugIns){printf("read pipe %d ", pipes[0]);}
                    close(pipes[1]); //Write not needed
                    dup2(pipes[0], 0); //For stdin of second command, read from pipe[0]
                    close(pipes[0]); //Read done
                    //if(debugIns){printf("-pipe done\n ");}
                }
                //Set stdin
                else if(exeIns.insList[1].stdin.type == TOFILE){
                    //Set file desciptor
                    int fileDescriptor2 = open(exeIns.insList[1].stdin.stdinFileName, O_RDONLY);

                    //File not found
                    if(fileDescriptor2 == -1){
                        //Return error, stdin file doesn't exist
                        if(debugIns){printf("CHILD PROCESS: stdin file not found %s \n", exeIns.insList[1].stdin.stdinFileName);}
                        return -1;
                    }

                    //If successful (file exists), set stdin
                    dup2(fileDescriptor2, 0);
                }
                //Set stdout
                if(exeIns.insList[1].stdout.type == TOFILE){
                    //if(debugIns){printf("stdout file: %s\n", exeIns.insList[1].stdout.stdoutFileName);}
                    //Try to open file
                    if(open(exeIns.insList[1].stdout.stdoutFileName, O_RDONLY) == -1){
                        //stdout file doesn't exist, create file
                        if(debugIns){printf("CHILD PROCESS: stdout file not found, create file %s \n", exeIns.insList[1].stdout.stdoutFileName);}
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
                        //stderr file doesn't exist, create file
                        if(debugIns){printf("CHILD PROCESS: stderr file not found, create file %s \n", exeIns.insList[1].stderr.stderrFileName);}
                        open(exeIns.insList[1].stderr.stderrFileName, O_CREAT | O_WRONLY);
                    }

                    //Set file desciptor
                    int fileDescriptor2 = open(exeIns.insList[1].stderr.stderrFileName, O_WRONLY);

                    //Set stderr
                    dup2(fileDescriptor2, 2);
                }

                //Execute child process (fork returns 0 in curPid)
                if(debugIns){printf("CHILD PROCESS2: [%d] calls execvp with cmd: %s\n", myPid, exeIns.insList[1].args[0]);} 
                execvp(exeIns.insList[1].args[0], exeIns.insList[1].args);
                if(debugIns){printf("\ninstruction failed to execute\n");} //Fix (implemented): args needs to be null terminated
            }  
            //Failed fork
            else {
                if(debugIns){printf("Fork 2 failed");}
                return -1;
            }
        }

        //-----------------------------------------------------------------------------------
        //End of second child, back to parent
        //-----------------------------------------------------------------------------------
        //Status of child process
        int status;

        //Status of 2nd child process
        int status2; 
        
        if(exeIns.num > 1){
            //Child process running
            if(!exeIns.background){
                //Wait for child process, set status WUNTRACED is set flag to wait
                if(debugIns){printf("PARENT PROCESS: Wait for child2\n");}
                waitpid(child2PID, &status2, WUNTRACED); 
            } else {
                //Don't wait
                if(debugIns){printf("PARENT PROCESS: No wait for child2\n");} //Instead of this, how to actually set process to background
                waitpid(child2PID, &status2, WNOHANG); 
            }
            //Child process done 
            if(debugIns){printf("PARENT PROCESS: [%d] Child2 done\n", child2PID);}
        }

        if(!exeIns.background){
            //Wait for child process, set status WUNTRACED is set flag to wait
            if(debugIns){printf("PARENT PROCESS: Wait for child\n");}
            waitpid(child1PID, &status, WUNTRACED); 
        } else {
            //Don't wait
            if(debugIns){printf("PARENT PROCESS: No wait for child\n");}
            waitpid(child1PID, &status, WNOHANG); 
        } 

        //Get status of child
        int sig = WSTOPSIG(status);

        //For pipelining, check if second instruction is also interrupted
        if(sig != SIGTSTP && exeIns.num > 1){
            sig = WSTOPSIG(status2);
        }
        
        //Child process done 
        if(debugIns){printf("PARENT PROCESS: [%d] Child done, return to parent, signal: %d\n", child1PID, sig);}

        if(!exeIns.background){
            //Set child to done list (Onlt if not stopped with SIGSTOP)
            finishJob(child1PID, sig);
        }

        //Move terminal control back to parent
        setpgid(parentPID, parentPID);
        if(!exeIns.background){
            tcsetpgrp(STDIN_FILENO, parentPID);
        }
        
    }
    //-----------------------------------------------------------------------------------
    //Child process
    //-----------------------------------------------------------------------------------
    else if (curPid == 0) {
        //Reset sigint to default in child
        signal(SIGINT, SIG_DFL); 
        signal(SIGTSTP, SIG_DFL);

        //Data for child process
        myPid = getpid();
        child1PID = myPid;
        if(debugIns){printf("CHILD PROCESS: [%d] INFO: pipeBool %d, exeIns.num %d, command %s\n", myPid, pipeBool, exeIns.num, exeIns.insList[0].args[0]);}

        //group pgid, give terminal control
        setpgid(parentPID, parentPID);
        setpgid(child1PID, child1PID);
        if(exeIns.num == 1 && !exeIns.background){
            tcsetpgrp(STDIN_FILENO, child1PID);
        }
        
        //Set stdin
        if(exeIns.insList[0].stdin.type == TOFILE){
            //Set file desciptor
            int fileDescriptor = open(exeIns.insList[0].stdin.stdinFileName, O_RDONLY);

            //File not found
            if(fileDescriptor == -1){
                //Return error, stdin file doesn't exist
                if(debugIns){printf("CHILD PROCESS: stdin file not found %s \n", exeIns.insList[0].stdin.stdinFileName);}
                return -1;
            }

            //Read file
            if(debugIns){printf("CHILD PROCESS: Read file %d, %s\n", fileDescriptor, exeIns.insList[0].stdin.stdinFileName);}

            //If successful (file exists), set stdin
            dup2(fileDescriptor, STDIN_FILENO);
        }   

        //Pipe stdout of first command into write end of pipe (if pipelining)
        if(pipeBool == 1){
            //if(debugIns){printf("write pipe %d ", pipes[1]);}
            close(pipes[0]); //Read not needed
            dup2(pipes[1], 1); //For stdout of first command, write to pipe[1]
            close(pipes[1]); //Write done
            //if(debugIns){printf("-pipe done\n ");}
        }
        //Set stdout
        else if(exeIns.insList[0].stdout.type == TOFILE){
            //if(debugIns){printf("stdout file: %s\n", exeIns.insList[0].stdout.stdoutFileName);}
            //Try to open file
            if(open(exeIns.insList[0].stdout.stdoutFileName, O_RDONLY) == -1){
                //stdout file doesn't exist, create file
                if(debugIns){printf("CHILD PROCESS: stdout file not found, create file %s \n", exeIns.insList[0].stdout.stdoutFileName);}
                open(exeIns.insList[0].stdout.stdoutFileName, O_CREAT | O_WRONLY);
            }

            //Set file desciptor
            int fileDescriptor = open(exeIns.insList[0].stdout.stdoutFileName, O_WRONLY);

            //Read file
            if(debugIns){printf("CHILD PROCESS: write file %d, %s\n", fileDescriptor, exeIns.insList[0].stdout.stdoutFileName);}

            //Set stdout
            dup2(fileDescriptor, 1);
        }
        //Set stderr
        if(exeIns.insList[0].stderr.type == TOFILE){
            //Try to open file
            if(open(exeIns.insList[0].stderr.stderrFileName, O_RDONLY) == -1){
                //stdout file doesn't exist, create file
                if(debugIns){printf("CHILD PROCESS: stderr file not found, create file %s \n", exeIns.insList[0].stderr.stderrFileName);}
                open(exeIns.insList[0].stderr.stderrFileName, O_CREAT | O_WRONLY);
            }

            //Set file desciptor
            int fileDescriptor = open(exeIns.insList[0].stderr.stderrFileName, O_WRONLY);

            //Set stderr
            dup2(fileDescriptor, 2);
        }

        //Execute child process (fork returns 0 in curPid)
        if(debugIns){printf("CHILD PROCESS: Child: [%d] calls execvp with cmd: %s\n", myPid, exeIns.insList[0].args[0]);} 
        execvp(exeIns.insList[0].args[0], exeIns.insList[0].args);
        if(debugIns){printf("\ninstruction failed to execute\n");} //Fix (implemented): args needs to be null terminated
        
    } else {
        //Failed fork
        if(debugIns){printf("Fork failed");}
        return -1;
    }
    
    return 0;
}

//Issues: if you only give foreground to children if they are foreground, it makes it so if they start at background and then go to foreground, they can't be interrupted
//Issue: if you give children foreground regardless and they start at background, the code for giving foreground to parent might trigger before the code for giving foreground to children might trigger, ending the code