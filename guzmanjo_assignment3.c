// Assignment 3: smallsh
// CS 374
// Author: Jon G.
// 8.2.25

#include "smallsh_header.h"

int main()
{
	// Create struct to hold command line input info
	struct command_line *curr_command;
	// Create background PIDs array and count of background PIDs
	pid_t* bgPids = NULL;
	int bgPidCount = 0;
	// Create variable to hold exit status of child processes
	int status = 0;

	// Setting up SIGINT struct
	// Main shell will ignore Ctrl-C
	struct sigaction SIGINT_action = {0};
	SIGINT_action.sa_handler = SIG_IGN;
	sigfillset(&SIGINT_action.sa_mask);
	SIGINT_action.sa_flags = 0;
	sigaction(SIGINT, &SIGINT_action, NULL);

	// Setting up SIGTSTP struct
	// Main shell will use signal handler for Ctrl-Z
	struct sigaction SIGTSTP_action = {0};
	SIGTSTP_action.sa_handler = handle_SIGTSTP;
	sigfillset(&SIGTSTP_action.sa_mask);
	SIGTSTP_action.sa_flags = 0;
	sigaction(SIGTSTP, &SIGTSTP_action, NULL);

	while(true)
	{
		//Iterate through bg processes to see if any have completed or were terminated by a signal
		for(int i = 0; i < bgPidCount; i++){
			int status;
			if (bgPids[i] > 0){
				pid_t bgPid = waitpid(bgPids[i], &status, WNOHANG);
				if (bgPid > 0){
					// Process was successful
					if(WIFEXITED(status)){
						printf("background pid %d is done: exit value %d\n", bgPid, WEXITSTATUS(status));
					}
					// Process terminated by signal
					if(WIFSIGNALED(status)){
						printf("background pid %d is done: terminated by signal %d\n", bgPid, WTERMSIG(status));
					}
					bgPids[i] = -1;
				}
			}
		}

		// Parses command line input from user
		curr_command = parse_input();

		// Checks if there was no input or if input was a comment
		if(curr_command->argv[0] == NULL || curr_command->argv[0][0] == '#'){
			continue;
		}
		// Checks if user exited the program
		else if(!strcmp(curr_command->argv[0], "exit")){
			exit(EXIT_SUCCESS);
		}
		// Checks if user wants to change directory
		else if(!strcmp(curr_command->argv[0], "cd")){
			if(curr_command->argc == 1){
				char* homeDir = getenv("HOME");
				chdir(homeDir);
			}
			else{
				chdir(curr_command->argv[1]);
			}
		}
		// Returns the exit status or terminating of last completed foreground command
		else if(!strcmp(curr_command->argv[0], "status")){
			printf("exit value %d\n", status);
			fflush(stdout);
		}
		else{
			//https://stackoverflow.com/questions/31714475/how-to-pass-an-integer-pointer-to-a-function
			processCommands(curr_command, &bgPids, &bgPidCount, &status);
		}
	}
	// Free command line struct memory
	freeCommandLineStruct(curr_command);

	// Free memory allocated to store background processes
	free(bgPids);

	return EXIT_SUCCESS;
}