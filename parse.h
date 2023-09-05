#include <stdio.h>
#include <readline/readline.h>
#include <stdbool.h>

//Contains structs for the main file

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

//Stores info about the stderr of a function
struct stderrData{
    char* stderrFileName; //On heap
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
    struct stderrData stderr;
    //Info of instruction
    char** args; //Malloc data, array of char arrays (Strings) terminated by NULL
    int numArgs; //Tracks number of args
};

//Struct for execution of instructions (all processes)
struct execution {
    struct instruction* insList;
    int status; //Status of processes
    int num; //number of processes (project requires max 2 processes)
    bool background;
};

//Parse input
struct instruction* parseInput(char* input, int* size, bool* validity, bool* backg);

//Execute instructions
int executeInstructions(struct execution exeIns, bool pipeBool, int pipes[2]);
//Check pipe first and then file redirection

//Make struct for jobs, comprised of processes (each process is an instruction struct, add later)
