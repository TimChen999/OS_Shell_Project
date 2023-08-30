#include <stdio.h>
#include <readline/readline.h>
#include <stdbool.h>

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

bool debug = true;

//See if the stdin, stdout, or stderr should be a file, the console, or user input
enum sourceType{
    NOTDEFINED, //stdin, stdout, or stderr not defined
    TOFILE, //input or output in file
    CONSOLE, //input is entered by user in console, output should be output to console
};

//Stores info about the stdin of a function, if it should go into the console or a file and if so, which file to put data in
struct stdinData{
    char* stdinFileName; //On heap
    enum sourceType type; //NOTE: if stdin type is console, stdinFileName should directly have input, if stdin type is file, parse input from file
};

//Stores info about the stdout of a function, if it should go to the console or a file and if so, which file to put data in
struct stdoutData{
    char* stdoutFileName; //On heap
    enum sourceType type;
};

//Stores information about a certain instruction
//Instructions include: file redirect ls, sleep
struct instruction {
    //Command of instruction
    char* command; //Malloc data
    //Where to direct in/out/err
    struct stdinData stdin; 
    struct stdoutData stdout;
    struct stdoutData stderr;
    //Info of instruction
    char** args; //Malloc data, array of char arrays (Strings) terminated by NULL
    int numArgs; //Tracks number of args
};

//Main includes process for parsing
//Parse commands to execvp, dont have to recognize all commands, fg bg are special case
int main() {
    if(debug){printf("Polling for input:\n");}
    while(1){
        //New input var for new line of input
        char* input = (char *)malloc(sizeof(char) * 2000); //last byte of string is null

        //User Enters input
        input = readline("#"); //readline returns pointer

        //Determine if instruction should be run in foreground or background
        bool background; 
        
        //Parse the user input
        struct instruction* insList = (struct instruction*)malloc(sizeof(struct instruction) * 20); //List of instructions, each separated by piping ('|'), can't parse more than 20 instructions
        char* element = strtok(input, " "); //Parse instruction, stores current element
        int valid = 1; //Check if instruction valid 
        int indexIns = 0; //index of current instruction on list

        //Initialize current instruction, add instruction to list
        struct instruction currentInstruction = {NULL, {"NA", NOTDEFINED}, {"NA", NOTDEFINED}, {"NA", NOTDEFINED}, NULL, 0}; 
        insList[indexIns] = currentInstruction; //Put instruction on stack to heap
        insList[indexIns].args = (char**)malloc((currentInstruction.numArgs + 2) * sizeof(char*));
        insList[indexIns].args[insList[indexIns].numArgs] = NULL; //Insert null at end of array

        //Loop through each element of input
        while(element != NULL && valid == 1){  
            //Parse character element
            if(debug){printf("\nparse: ");}
            
            //Special commands
            if(strcmp(element, "jobs") == 0){
                if(insList[indexIns].command == NULL){
                    if(debug){printf("set command to jobs");}
                    insList[indexIns].command = (char*)malloc(sizeof("jobs") + 1);
                    strcpy(insList[indexIns].command, "jobs");
                }
                else{
                    if(debug){printf("Invalid: duplicate command");}
                    valid = 0;
                }
            }
            else if(strcmp(element, "fg") == 0){
                if(insList[indexIns].command == NULL){
                    if(debug){printf("set command to fg");}
                    insList[indexIns].command = (char*)malloc(sizeof("fg") + 1);
                    strcpy(insList[indexIns].command, "fg");
                }
                else{
                    if(debug){printf("Invalid: duplicate command");}
                    valid = 0;
                }
            }
            else if(strcmp(element, "bg") == 0){
                if(insList[indexIns].command == NULL){
                    if(debug){printf("set command to bg");}
                    insList[indexIns].command = (char*)malloc(sizeof("bg") + 1);
                    strcpy(insList[indexIns].command, "bg");
                }
                else{
                    if(debug){printf("Invalid: duplicate command");}
                    valid = 0;
                }
            }
            
            //Check for &
            else if(strcmp(element, "&") == 0){
                //Check next element (should be null)
                element = strtok(NULL, " ");
                if(debug){printf("send command to background, ");}
                if(element == NULL){
                    if(debug){printf("valid command");}
                    background = true;
                    break;
                }
                else{
                    if(debug){printf("invalid command");}
                    valid = 0;
                    break;
                }
            }

            //Check for redirect
            else if(strcmp(element, "<") == 0){
                if(insList[indexIns].command == NULL){
                    if(debug){printf("Invalid: command must come before redirect");}
                    valid = 0;
                }
                else{
                    //Parse next, put into stdin
                    element = strtok(NULL, " "); 
                    if(element == NULL){
                        if(debug){printf("Invalid: need input for stdin");}
                        valid = 0;
                    }
                    else{
                        //Change stdin of command to filename
                        if(debug){printf("<, %s into stdin", element);}
                        char* fileName = (char*)malloc(sizeof(element) + 1);
                        insList[indexIns].stdin.stdinFileName = fileName;
                        insList[indexIns].stdin.type = TOFILE;
                    }
                }
            }
            else if(strcmp(element, ">") == 0){
                if(insList[indexIns].command == NULL){
                    if(debug){printf("Invalid: command must come before redirect");}
                    valid = 0;
                }
                else{
                    //Parse next, put into stdin
                    element = strtok(NULL, " "); 
                    if(element == NULL){
                        if(debug){printf("Invalid: need input for stdin");}
                        valid = 0;
                    }
                    else{
                        //Change stdin of command to filename
                        if(debug){printf(">, %s into stdout", element);}
                        char* fileName = (char*)malloc(sizeof(element) + 1);
                        insList[indexIns].stdout.stdoutFileName = fileName;
                        insList[indexIns].stdout.type = TOFILE;
                    }
                }
            }
            else if(strcmp(element, "2>") == 0){
                if(insList[indexIns].command == NULL){
                    if(debug){printf("Invalid: command must come before redirect");}
                    valid = 0;
                }
                else{
                    //Parse next, put into stdin
                    element = strtok(NULL, " "); 
                    if(element == NULL){
                        if(debug){printf("Invalid: need input for stderr");}
                        valid = 0;
                    }
                    else{
                        //Change stdin of command to filename
                        if(debug){printf(">, %s into stderr", element);}
                        char* fileName = (char*)malloc(sizeof(element) + 1);
                        insList[indexIns].stderr.stdoutFileName = fileName;
                        insList[indexIns].stderr.type = TOFILE;
                    }
                }
            }

            //Check for pipe
            else if(strcmp(element, "|") == 0){
                if(debug){printf("|, pipe new instruction");}
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
                if(debug){printf("Command: %s", cmdName);}

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
                if(debug){printf("Arg: %s", argName);}
            }
            element = strtok(NULL, " "); //call it again with null means you want to look at remaining string
        }
        //Debug at end of parse
        if(debug){printf("\nParse finished\n");}
        for(int i = 0; i < indexIns + 1 && debug; i++){
            printf("Index %d: Command: %s stdin: %s stdout: %s Args: ", (i), insList[i].command, insList[i].stdin.stdinFileName, insList[i].stdout.stdoutFileName);
            for(int j = 0; j < insList[i].numArgs; j++){
               printf("%d: %s ", j, insList[i].args[j]);
            }
            printf("\n");
        }
        if(debug){printf("\n");}

        free(input);
    }

    //One task in foreground, rest in background
    
    return 0;
}

