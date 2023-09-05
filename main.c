#include <stdio.h>
#include <readline/readline.h>
#include <stdbool.h>
#include "parse.h"
#include "signalHeader.h"

// Features
// Stdin is what you type in the shell or what you redirect into it
// echo is command and string is argument. Output is stdout (whatever is on console), but > file1.txt means you tell it to go into a file
// echo string > file1.txt -put string into contents of file1.txt
// cat file1.txt > file2.txt -cat file1.txt prints out file1
// first is program name, rest is argument, but ">" or "<" means next is file name, what you want to change stdin or stdout to 
// "<" works with commands that take input and ">" work with commands that take output. less than followed by input (what is taken in as input), greater then followed by output (where the output goes)
// ex: cat without argument takes stdin (what you type to console) and prints it out (stdout)
// cat followed by string argument will print string argument
// cat < input.txt will read input.txt (instead of what is typed) and put it on console
// cat < input.txt > a.txt means you take what is in input.txt as stdin into cat. cat will now put it's stdin into stdout. This will usually print to console, but now will put it into a.txt (> a.txt)
// cat > a.txt < input.txt does the same stuff
// cat > a.txt will mean that cat is expecting input from console and whatever is typed goes into a.txt 
// However "< input.txt" means that input of cat is input.txt, which will be put into a.txt
// "2>" is similar to ">" but takes stderr instead of stdout
// Sign just tells you whether to replace stdin, stdout, or stderr which conveniently are 0, 1, 2. They basically just change what is the input or output. A command like cat has both input and output "<" and ">" just changes what is input (stdin) and output (stdout)

bool debugMain = true;

//Main includes process for parsing
//Parse commands to execvp, dont have to recognize all commands, fg bg are special case
int main() {
    if(debugMain){printf("Polling for input:\n");}
    while(1){
        //New input var for new line of input
        char* input = (char *)malloc(sizeof(char) * 2000); //last byte of string is null

        //User Enters input
        input = readline("#"); //readline returns pointer

        //Size of instruction (number of processes)
        int size = 0;

        //Validity of instruction
        bool valid = true;

        //Background
        bool background = false;

        //Parse the instruction
        struct instruction* insList = parseInput(input, &size, &valid, &background);

        //Debug at end of parse
        printf("\nNumber of processes: %d", size);
        if(debugMain){printf("\nParse finished, Valid: %d\n", valid);}
        for(int i = 0; i < size && debugMain; i++){
            printf("Index %d: Command: %s stdin: %s stdout: %s Args: ", (i), insList[i].command, insList[i].stdin.stdinFileName, insList[i].stdout.stdoutFileName);
            for(int j = 0; j < insList[i].numArgs; j++){
                printf("%d: %s ", j, insList[i].args[j]);
            }
            printf("\n");
        }
        if(debugMain){printf("\n");} 

        //List of processes to execute
        struct execution exeList = {insList, 0, size, background};

        //init signal
        sigInit();

        //Boolean determine whether to pipe (Make sure piping isnt overriden by redirection)
        bool pipeBool = false;
        if((size > 1 && exeList.insList[0].stdout.type != TOFILE) && exeList.insList[1].stdin.type != TOFILE){
            pipeBool = true;
        }

        //Create pipe to pass as parameter (doing so allows the function to call itself without overriding pipe with new definition, allowing piping to work)
        int pipes[2] = {0,0};

        //Reset Processes in signal.c
        resetProcess();

        //One task in foreground, rest in background
        if(valid){
            int a = executeInstructions(exeList, pipeBool, pipes);
        }
    }
    return 0;
}

//change: redirect takes priority over a pipe ls > text | cat results in ls outputting to text, cat ignored, only execute pipe instructions if stdin/out not set
//tcsetgrp to set foreground background
//set follow-fork-mode child

