/**
 * A sample program for parsing a command line. If you find it useful,
 * feel free to adapt this code for Assignment 4.
 * Do fix memory leaks and any additional issues you find.
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>


#define INPUT_LENGTH 2048
#define MAX_ARGS 512


struct command_line
{
	char *argv[MAX_ARGS + 1];
	int argc;
	char *input_file;
	char *output_file;
	bool is_bg;
};


struct command_line *parse_input()
{
	char input[INPUT_LENGTH];
	struct command_line *curr_command = (struct command_line *) calloc(1, sizeof(struct command_line));

	// Get input
	printf(": ");
	fflush(stdout);
	fgets(input, INPUT_LENGTH, stdin);
    //int test = strlen(input);

    
	// Do I need to free any memory here?
	// Tokenize the input
	char* savePtr;
	char* checkPtr;
	char *token = strtok_r(input, " \n", &savePtr);
	while(token){
		if(!strcmp(token,"<")){
			curr_command->input_file = strdup(strtok_r(NULL," \n", &savePtr));
		} else if(!strcmp(token,">")){
			curr_command->output_file = strdup(strtok_r(NULL," \n", &savePtr));
		} else if(!strcmp(token,"&")){
			checkPtr = savePtr;
            char *checkToken = strtok_r(NULL, " \n", &checkPtr);
            if(checkPtr == NULL){
                curr_command->is_bg = true;
            }
			else{
				curr_command->argv[curr_command->argc++] = strdup(token);
			}
		} else if(!strcmp(token,"#")){
			break;
		}else{
			curr_command->argv[curr_command->argc++] = strdup(token);
		}
		token=strtok_r(NULL," \n", &savePtr);
	}
	return curr_command;
}

int main()
{
	struct command_line *curr_command;

	while(true)
	{
		curr_command = parse_input();

		for(int i = 0; i <curr_command->argc; i++){
			// Exit command
			// https://stackoverflow.com/questions/14558068/c-kill-all-processes
			if(curr_command->argv[i] == "exit"){
				kill(0, SIGKILL);
			}
		}

	}
	return EXIT_SUCCESS;
}