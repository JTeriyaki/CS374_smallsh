#include "smallsh_header.h"

bool SIGTSTP_flag = false;

/* This function parses a users command line input to the terminal and fills
a respecting command line struct to hold the input arguments, whether the process(es)
should be run in the foreground or background and whether there is an input or output 
file for i/o redirection
Source: OSU CS374 assignment material
*/
struct command_line *parse_input()
{
	char input[INPUT_LENGTH];
	struct command_line *curr_command = (struct command_line *) calloc(1, sizeof(struct command_line));

	// Get input from user
	printf(": ");
	fflush(stdout);
	fgets(input, INPUT_LENGTH, stdin);

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
            if(checkToken == NULL){
                curr_command->is_bg = true;
            }
			else{
				curr_command->argv[curr_command->argc++] = strdup(token);
			}
		}else{
			curr_command->argv[curr_command->argc++] = strdup(token);
		}
		token=strtok_r(NULL," \n", &savePtr);
	}
	return curr_command;
}


/* Function process foreground and background commands while checking for input and output files
for appropriate i/o redirection. Tracks the number of background processes and stores their PIDs
in a background processes array which is checked in main before each command line input.
*/
void processCommands(struct command_line* command, pid_t** bgPids, int* bgPidCount, int* status){
	// if SIGTSTP is false then background processes are registered
	if(SIGTSTP_flag == false){
		// Background process check
		if(command->is_bg){
			// Background process
			processBackgroundCommand(command, bgPids, bgPidCount);
		}else{
			// Foreground process
			processForegroundCommand(command, status);
		}
	}else{
		// Foreground process
		processForegroundCommand(command, status);
	}
}

/* Function handles the SIGTSTP signal by processing commands in the foreground only
with an initial Ctrl-Z and then exiting that mode if Ctrl-Z is entered again
*/
void handle_SIGTSTP(int signalNum){
	if(SIGTSTP_flag == false){
		SIGTSTP_flag = true;
		char* message = "Entering foreground-only mode (& is now ignored)\n";
		write(STDOUT_FILENO, message, 50);
	}
	else{
		SIGTSTP_flag = false;
		char* message = "Exiting foreground-only mode\n";
		write(STDOUT_FILENO, message, 30);
	}
}

/* Function frees the memory that was allocated to the command line input
*/
void freeCommandLineStruct(struct command_line *input){
	for(int i = 0; i < input->argc; i++){
		free(input->argv[i]);
	}
	if(input->input_file != NULL){
		free(input->input_file);
	}
	if(input->output_file != NULL){
		free(input->output_file);
	}
	free(input);
}

/* Function process foreground commands only. Handles i/o redirection and signaling.
*/
void processForegroundCommand(struct command_line* command, int* status){
	int childStatus;

	pid_t spawnPid = fork();

		switch(spawnPid){
			case -1:
				perror("fork()\n");
				fflush(stdout);
				(*status) = 1;
				exit(1);
				break;

			case 0:
				// Child process execution

				// I/O redirections
				// If input file
				if(command->input_file){
					// Open input file
					int inputFD = open(command->input_file, O_RDONLY);
					if (inputFD == -1){
						printf("cannot open %s for input\n", command->input_file);
						fflush(stdout);
						(*status) = 1;
						exit(1);
					}

					// Redirect stdin to input file
					int inResult = dup2(inputFD, 0);
					if (inResult == -1){
						perror("input dup2()");
						fflush(stdout);
						(*status) = 1;
						exit(1);
					}
				}

				// If output file
				if (command->output_file){
					// Open output file
					int outputFD = open(command->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0640);
					if (outputFD == -1){
						printf("cannot open %s for output\n", command->output_file);
						fflush(stdout);
						(*status) = 1;
						exit(1);
					}

					// Redirect stdout to output file
					int outResult = dup2(outputFD, 1);
						if (outResult == -1){
						perror("output dup2()");
						fflush(stdout);
						(*status) = 1;
						exit(1);
					}
				}
				
				// Reverting SIGINT to default so will recognize Ctrl-C
				signal(SIGINT, SIG_DFL);

				// Ignoring SIGTSTP struct created by parent so will ignore Ctrl-Z
				signal(SIGTSTP, SIG_IGN);

				// Execute command line arguments
				execvp(command->argv[0], command->argv);
				perror("execvp");
				fflush(stdout);
				(*status) = 1;
				exit(1);
				break;

			default:
				spawnPid = waitpid(spawnPid, &childStatus, 0);
				if (WIFSIGNALED(childStatus)){
					printf("terminated by signal %d\n", WTERMSIG(childStatus));
					fflush(stdout);
				}
				else{
					(*status) = WEXITSTATUS(childStatus);
				}
				break;
		}
}

/* Function processes backround commands only. Handles i/o redirection and signaling. Adds background
processes to array to check if they've completed in main before next command line input.
*/
void processBackgroundCommand(struct command_line* command, pid_t** bgPids, int* bgPidCount){
	pid_t spawnPid = fork();
		switch(spawnPid){
			case -1:
				perror("fork()\n");
				fflush(stdout);
				exit(1);
				break;
			case 0:
				int inputFD;
				// I/O redirections
				// If input file
				if(command->input_file){
					// Open input file
					inputFD = open(command->input_file, O_RDONLY);
					if (inputFD == -1){
						printf("cannot open %s for input\n", command->input_file);
						fflush(stdout);
						exit(1);
					}
				}else{
					inputFD = open("/dev/null", O_RDONLY);
					if (inputFD == -1){
						printf("open /dev/null\n");
						fflush(stdout);
						exit(1);
					}
				}
				// Redirect stdin to input file
				int inResult = dup2(inputFD, 0);
				if (inResult == -1){
					perror("input dup2()");
					fflush(stdout);
					exit(1);
				}

				// If output file
				int outputFD;
				if (command->output_file){
					// Open output file
					outputFD = open(command->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0640);
					if (outputFD == -1){
						printf("cannot open %s for output\n", command->output_file);
						fflush(stdout);
						exit(1);
					}else{
						outputFD = open("/dev/null", O_RDONLY);
						if (outputFD == -1){
						printf("open /dev/null\n");
						fflush(stdout);
						exit(1);
						}
					}
				}
				// Redirect stdout to output file
				int outResult = dup2(outputFD, 1);
					if (outResult == -1){
					perror("output dup2()");
					fflush(stdout);
					exit(1);
					}
					
				// Ignoring SIGTSTP struct created by parent
				signal(SIGTSTP, SIG_IGN);

				execvp(command->argv[0], command->argv);
				perror("excevp");
				fflush(stdout);
				exit(1);
				break;
			default:
				printf("background pid is %d\n", spawnPid);
				fflush(stdout);

				// Store background commands
				*bgPids = (pid_t *)realloc(*bgPids, (*bgPidCount + 1) * sizeof(pid_t));
				(*bgPids)[*bgPidCount] = spawnPid;
				(*bgPidCount)++;
				break;
		}
}
