#include <stdio.h>
#include <readline/readline.h>
#include <stdbool.h>
#include "parse.h"

//This file contains code to parse instauctions

bool debug = false;
struct instruction* parseInput(char* input, int* size, bool* validity, bool* backg){
//Determine if instruction should be run in foreground or background
    bool background; 
    
    //Parse the user input
    struct instruction* insList = (struct instruction*)malloc(sizeof(struct instruction) * 20); //List of instructions, each separated by piping ('|'), can't parse more than 20 instructions
    char* element = strtok(input, " "); //Parse instruction, stores current element
    bool valid = true; //Check if instruction valid 
    int indexIns = 0; //index of current instruction on list

    //Initialize current instruction, add instruction to list
    struct instruction currentInstruction = {NULL, {"NA", NOTDEFINED}, {"NA", NOTDEFINED}, {"NA", NOTDEFINED}, NULL, 0}; 
    insList[indexIns] = currentInstruction; //Put instruction on stack to heap
    insList[indexIns].args = (char**)malloc((currentInstruction.numArgs + 2) * sizeof(char*));
    insList[indexIns].args[insList[indexIns].numArgs] = NULL; //Insert null at end of array

    //Loop through each element of input
    while(element != NULL && valid == 1){  
        //Parse character element
        if(debug){printf("PARSE.C: start: ");}
        
        //Special commands
        if(strcmp(element, "jobs") == 0){
            if(insList[indexIns].command == NULL){
                if(debug){printf(" set command to jobs ");}
                insList[indexIns].command = (char*)malloc(sizeof("jobs") + 1);
                strcpy(insList[indexIns].command, "jobs");
            }
            else{
                if(debug){printf(" Invalid: duplicate command ");}
                valid = false;
            }
        }
        else if(strcmp(element, "fg") == 0){
            if(insList[indexIns].command == NULL){
                if(debug){printf(" set command to fg ");}
                insList[indexIns].command = (char*)malloc(sizeof("fg") + 1);
                strcpy(insList[indexIns].command, "fg");
            }
            else{
                if(debug){printf(" Invalid: duplicate command ");}
                valid = false;
            }
        }
        else if(strcmp(element, "bg") == 0){
            if(insList[indexIns].command == NULL){
                if(debug){printf(" set command to bg ");}
                insList[indexIns].command = (char*)malloc(sizeof("bg") + 1);
                strcpy(insList[indexIns].command, "bg");
            }
            else{
                if(debug){printf(" Invalid: duplicate command ");}
                valid = false;
            }
        }
        
        //Check for &
        else if(strcmp(element, "&") == 0){
            //Check next element (should be null)
            element = strtok(NULL, " ");
            if(debug){printf(" send command to background, ");}
            if(element == NULL){
                if(debug){printf(" valid command ");}
                background = true;
                break;
            }
            else{
                if(debug){printf(" invalid command ");}
                valid = false;
                break;
            }
        }

        //Check for redirect
        else if(strcmp(element, "<") == 0){
            if(insList[indexIns].command == NULL){
                if(debug){printf(" Invalid: command must come before redirect ");}
                valid = false;
            }
            else{
                //Parse next, put into stdin
                element = strtok(NULL, " "); 
                if(element == NULL){
                    if(debug){printf(" Invalid: need input for stdin ");}
                    valid = false;
                }
                else{
                    //Change stdin of command to filename
                    if(debug){printf("<, %s into stdin", element);}
                    char* fileName = (char*)malloc(sizeof(element) + 1);
                    strcpy(fileName, element);
                    insList[indexIns].stdin.stdinFileName = fileName;
                    insList[indexIns].stdin.type = TOFILE;
                }
            }
        }
        else if(strcmp(element, ">") == 0){
            if(insList[indexIns].command == NULL){
                if(debug){printf(" Invalid: command must come before redirect ");}
                valid = false;
            }
            else{
                //Parse next, put into stdout
                element = strtok(NULL, " "); 
                if(element == NULL){
                    if(debug){printf(" Invalid: need input for stdin ");}
                    valid = false;
                }
                else{
                    //Change stdin of command to filename
                    if(debug){printf(" >, %s into stdout ", element);}
                    char* fileName = (char*)malloc(sizeof(element) + 1);
                    strcpy(fileName, element);
                    insList[indexIns].stdout.stdoutFileName = fileName;
                    insList[indexIns].stdout.type = TOFILE;
                }
            }
        }
        else if(strcmp(element, "2>") == 0){
            if(insList[indexIns].command == NULL){
                if(debug){printf(" Invalid: command must come before redirect ");}
                valid = false;
            }
            else{
                //Parse next, put into stderr
                element = strtok(NULL, " "); 
                if(element == NULL){
                    if(debug){printf(" Invalid: need input for stderr ");}
                    valid = false;
                }
                else{
                    //Change stdin of command to filename
                    if(debug){printf(" >, %s into stderr ", element);}
                    char* fileName = (char*)malloc(sizeof(element) + 1);
                    strcpy(fileName, element);
                    insList[indexIns].stderr.stderrFileName = fileName;
                    insList[indexIns].stderr.type = TOFILE;
                }
            }
        }

        //Check for pipe
        else if(strcmp(element, "|") == 0){
            if(debug){printf(" |, pipe new instruction ");}
            indexIns++;
            //Add new instruction (insList[indexIns] will point to the new instruction)
            struct instruction newInstruction = {NULL, {"NA", NOTDEFINED}, {"NA", NOTDEFINED}, {"NA", NOTDEFINED}, NULL, 0}; 
            insList[indexIns] = newInstruction;
            //Set stdout of previous instruction to stdin of current, malloc new space and strcpy
            insList[indexIns].stdin.stdinFileName = (char*)malloc(sizeof(insList[indexIns - 1].stdout.stdoutFileName) + 1);
            strcpy(insList[indexIns].stdin.stdinFileName, insList[indexIns - 1].stdout.stdoutFileName);
            insList[indexIns].stdin.type = insList[indexIns - 1].stdout.type;
            insList[indexIns].numArgs = 0;
        }

        //Check for other command/args
        else if(insList[indexIns].command == NULL){
            //Store command
            char* cmdName = (char*)malloc(sizeof(element) + 1);
            strcpy(cmdName, element);
            insList[indexIns].command = cmdName;
            if(debug){printf(" Command: %s ", cmdName);}

            //Add command as arg
            insList[indexIns].args = (char**)realloc(insList[indexIns].args, (insList[indexIns].numArgs + 2) * sizeof(char*));
            char* argName = (char*)malloc(sizeof(element) + 1);
            strcpy(argName, element);
            insList[indexIns].args[insList[indexIns].numArgs] = argName;
            insList[indexIns].numArgs++;
            insList[indexIns].args[insList[indexIns].numArgs] = NULL; //Insert null at end of array
        }
        else{
            //Add arg
            insList[indexIns].args = (char**)realloc(insList[indexIns].args, (insList[indexIns].numArgs + 2) * sizeof(char*));
            char* argName = (char*)malloc(sizeof(element) + 1);
            strcpy(argName, element);
            insList[indexIns].args[insList[indexIns].numArgs] = argName;
            insList[indexIns].numArgs++;
            insList[indexIns].args[insList[indexIns].numArgs] = NULL; //Insert null at end of array
            if(debug){printf(" Arg: %s ", argName);}
        }
        element = strtok(NULL, " "); //call it again with null means you want to look at remaining string
    }

    free(input);

    //Set size 
    *size = indexIns + 1;

    //Set validity
    *validity = valid;

    //set background
    *backg = background;

    if(debug){printf("\n");}

    return insList;
}


