#include "builtin.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUILTIN_ERROR 0
#define BUILTIN_SUCCESS 1

// returns true if the 'exit' call
// should be performed
//

// aux functions for cd command
int change_to_home(void);
int change_to_path(const char *path);
void update_prompt(void);

int
exit_shell(char *cmd)
{
	if (strcmp(cmd, "exit") != 0) {
		return BUILTIN_ERROR;
	}

	return BUILTIN_SUCCESS;
}

// returns true if "chdir" was performed
//  this means that if 'cmd' contains:
// 	1. $ cd directory (change to 'directory')
// 	2. $ cd (change to $HOME)
//  it has to be executed and then return true
//
//  Remember to update the 'prompt' with the
//  	new directory.
//
// Examples:
//  1. cmd = ['c','d', ' ', '/', 'b', 'i', 'n', '\0']
//  2. cmd = ['c','d', '\0']

int
change_to_home(void)
{
	char *home = getenv("HOME");
	if (home == NULL || chdir(home) != 0) {
		perror("chdir($HOME) failed");
		return BUILTIN_ERROR;
	}
	return BUILTIN_SUCCESS;
}

int
change_to_path(const char *path)
{
	if (chdir(path) != 0) {
		perror("chdir() failed");
		return BUILTIN_ERROR;
	}
	return BUILTIN_SUCCESS;
}

void
update_prompt(void)
{
	char *cwd = getcwd(NULL, 0);
	if (cwd != NULL) {
		snprintf(prompt, sizeof prompt, "(%s)", cwd);
		free(cwd);
	} else {
		perror("getcwd failed");
	}
}

int
cd(char *cmd)
{
	if (cmd == NULL) {
		return BUILTIN_ERROR;
	}

	char *cmd_copy = malloc(strlen(cmd) + 1);
	if (cmd_copy == NULL) {
		perror("malloc failed");
		return BUILTIN_ERROR;
	}
	strcpy(cmd_copy, cmd);

	int status = BUILTIN_ERROR;

	if (strcmp(cmd_copy, "cd") == 0) {
		status = change_to_home();
	} else if (strncmp(cmd_copy, "cd ", 3) == 0) {
		char *path = cmd_copy + 3;
		status = change_to_path(path);
	}

	if (status == BUILTIN_SUCCESS) {
		update_prompt();
	}

	free(cmd_copy);
	return status;
}

// returns true if 'pwd' was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int
pwd(char *cmd)
{
	if (strcmp(cmd, "pwd") == 1) {
		char buf[BUFLEN] = { 0 };
		char *pwd = getcwd(buf, sizeof(buf));
		if (pwd != NULL) {
			printf_debug("%s\n", pwd);
			return BUILTIN_SUCCESS;
		}
	}
	return BUILTIN_ERROR;
}

// returns true if `history` was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int
history(char *cmd)
{
	// Your code here

	return 0;
}
