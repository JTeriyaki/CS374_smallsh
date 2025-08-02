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
#include <fcntl.h>


#define INPUT_LENGTH 2048
#define MAX_ARGS 512

bool SIGTSTP_flag = false;

struct command_line
{
	char *argv[MAX_ARGS + 1];
	int argc;
	char *input_file;
	char *output_file;
	bool is_bg;
};

struct command_line *parse_input();

void processCommands(struct command_line* command, pid_t** bgPids, int* bgPidCount, int* status);

void handle_SIGTSTP(int signalNum);

int main()
{
	struct command_line *curr_command;
	pid_t* bgPids = NULL;
	int bgPidCount = 0;
	int status = 0;

	// Set up signal handlers
	// Sigint I want to change to ignore which will carry through to child foreground and background
	// However, I will need to rest in the foreground child process from Sig_ign back to sig_dfl so
	// that Ctrl-C actually stops a child foreground process. This happens after fork but before exec()

	// Setting up SIGINT struct
	struct sigaction SIGINT_action = {0};
	SIGINT_action.sa_handler = SIG_IGN;
	sigfillset(&SIGINT_action.sa_mask);
	SIGINT_action.sa_flags = 0;
	sigaction(SIGINT, &SIGINT_action, NULL);

	// Setting up SIGTSTP struct
	struct sigaction SIGTSTP_action = {0};
	SIGTSTP_action.sa_handler = handle_SIGTSTP;
	sigfillset(&SIGTSTP_action.sa_mask);
	SIGTSTP_action.sa_flags = 0;
	sigaction(SIGTSTP, &SIGTSTP_action, NULL);

	while(true)
	{

		//Iterate through bg processes
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

		curr_command = parse_input();

		if(curr_command->argv[0] == NULL){
			continue;
		}

		if(!strcmp(curr_command->argv[0], "exit")){
			exit(EXIT_SUCCESS);
		}
		else if(curr_command->argv[0][0] == '#'){
			continue;
		}
		else if(!strcmp(curr_command->argv[0], "cd")){
			if(curr_command->argc == 1){
				char* homeDir = getenv("HOME");
				//printf("Home dir = %s\n", homeDir);
				chdir(homeDir);
				//fflush(stdout);
			}
			else{
				//printf("Directory to change to = %s\n", curr_command->argv[1]);
				chdir(curr_command->argv[1]);
				//fflush(stdout);
			}
		}
		else if(!strcmp(curr_command->argv[0], "status")){
			printf("exit value %d\n", status);
			fflush(stdout);
			// will need to add in something that tracks foreground commands
		}
		else{
			//https://stackoverflow.com/questions/31714475/how-to-pass-an-integer-pointer-to-a-function
			processCommands(curr_command, &bgPids, &bgPidCount, &status);
		}

	}
	freeCommandLineStruct(curr_command);
	return EXIT_SUCCESS;
}


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

void processCommands(struct command_line* command, pid_t** bgPids, int* bgPidCount, int* status){
	int childStatus;

	// SIGTSTP is false allowing for background processes to run in background
	if(SIGTSTP_flag == false){

		// Background command
		if(command->is_bg){
			pid_t spawnPid = fork();

			switch(spawnPid){
				case -1:
					perror("fork()\n");
					fflush(stdout);
					exit(1);
					break;
				case 0:

					// Setting up SIGINT struct
					struct sigaction SIGTSTP_action = {0};
					SIGTSTP_action.sa_handler = SIG_IGN;
					sigfillset(&SIGTSTP_action.sa_mask);
					SIGTSTP_action.sa_flags = 0;
					sigaction(SIGINT, &SIGTSTP_action, NULL);

					execvp(command->argv[0], command->argv);
					perror("excevp");
					fflush(stdout);
					exit(1);
					break;
				default:
					printf("background pid is %d\n", spawnPid);
					//printf("foreground Pid should be: %d\n", getpid());
					fflush(stdout);
					*bgPids = (pid_t *)realloc(*bgPids, (*bgPidCount + 1) * sizeof(pid_t));
					(*bgPids)[*bgPidCount] = spawnPid;
					(*bgPidCount)++;
					break;
			}
		}

		// Foreground process
		else{
			pid_t spawnPid = fork();

			switch(spawnPid){
				case -1:
					perror("fork()\n");
					fflush(stdout);
					(*status) = 1;
					exit(1);
					break;

				case 0:
					// I/O redirections

					// If input file
					if(command->input_file){
						// printf("This works\n");
						// fflush(stdout);

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

					// I/O redirection is complete now completing child processes
					// Signal handling will need to pass in struct to function
					//SIGINT_action.sa_handler = SIG_DFL;
					//sigaction(SIGINT, &SIGINT_action, NULL);
					struct sigaction SIGINT_action_fg_child ={0};
					SIGINT_action_fg_child.sa_handler = SIG_DFL;
					sigfillset(&SIGINT_action_fg_child.sa_mask);
					SIGINT_action_fg_child.sa_flags = 0;
					sigaction(SIGINT, &SIGINT_action_fg_child, NULL);

					// Setting up SIGINT struct
					struct sigaction SIGTSTP_action = {0};
					SIGTSTP_action.sa_handler = SIG_IGN;
					sigfillset(&SIGTSTP_action.sa_mask);
					SIGTSTP_action.sa_flags = 0;
					sigaction(SIGINT, &SIGTSTP_action, NULL);

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
	}else{
		pid_t spawnPid = fork();

			switch(spawnPid){
				case -1:
					perror("fork()\n");
					fflush(stdout);
					(*status) = 1;
					exit(1);
					break;

				case 0:
					// I/O redirections

					// If input file
					if(command->input_file){
						// printf("This works\n");
						// fflush(stdout);

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

					// I/O redirection is complete now completing child processes
					// Signal handling will need to pass in struct to function
					//SIGINT_action.sa_handler = SIG_DFL;
					//sigaction(SIGINT, &SIGINT_action, NULL);
					struct sigaction SIGINT_action_fg_child ={0};
					SIGINT_action_fg_child.sa_handler = SIG_DFL;
					sigfillset(&SIGINT_action_fg_child.sa_mask);
					SIGINT_action_fg_child.sa_flags = 0;
					sigaction(SIGINT, &SIGINT_action_fg_child, NULL);

					// Setting up SIGINT struct
					struct sigaction SIGTSTP_action = {0};
					SIGTSTP_action.sa_handler = SIG_IGN;
					sigfillset(&SIGTSTP_action.sa_mask);
					SIGTSTP_action.sa_flags = 0;
					sigaction(SIGINT, &SIGTSTP_action, NULL);

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

}


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
