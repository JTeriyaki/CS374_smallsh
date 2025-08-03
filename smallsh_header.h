#ifndef SMALLSH_HEADER
#define SMALLSH_HEADER

#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

#define INPUT_LENGTH 2048
#define MAX_ARGS 512

extern bool SIGTSTP_flag;

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

void freeCommandLineStruct(struct command_line *input);

void processForegroundCommand(struct command_line* command, int* status);

void processBackgroundCommand(struct command_line* command, pid_t** bgPids, int* bgPidCount);

#endif // SMALLSH_HEADER