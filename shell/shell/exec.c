#include "exec.h"
#include "parsing.h"
#define READ 0
#define WRITE 1

// aux function for case PIPE
void check_dup2(int result, int fd_to_close);

void
check_dup2(int dup2_result, int fd_to_close)
{
	close(fd_to_close);
	if (dup2_result < 0) {
		perror("dup2");
		exit(-1);
	}
}

// sets "key" with the key part of "arg"
// and null-terminates it
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  key = "KEY"
//
static void
get_environ_key(char *arg, char *key)
{
	int i;
	for (i = 0; arg[i] != '='; i++)
		key[i] = arg[i];

	key[i] = END_STRING;
}

// sets "value" with the value part of "arg"
// and null-terminates it
// "idx" should be the index in "arg" where "=" char
// resides
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  value = "value"
//
static void
get_environ_value(char *arg, char *value, int idx)
{
	size_t i, j;
	for (i = (idx + 1), j = 0; i < strlen(arg); i++, j++)
		value[j] = arg[i];

	value[j] = END_STRING;
}

// sets the environment variables received
// in the command line
//
// Hints:
// - use 'block_contains()' to
// 	get the index where the '=' is
// - 'get_environ_*()' can be useful here
static void
set_environ_vars(char **eargv, int eargc)
{
	for (int i = 0; i < eargc; ++i) {
		char *env_var = eargv[i];
		int equal_index = block_contains(env_var, '=');
		if (equal_index < 0)
			continue;

		char key[ARGSIZE];
		char value[ARGSIZE];

		get_environ_key(env_var, key);
		get_environ_value(env_var, value, equal_index);

		if (setenv(key, value, 1) != 0) {
			fprintf(stderr,
			        "Error setting environment variable %s\n",
			        key);
		}
	}
}

// opens the file in which the stdin/stdout/stderr
// flow will be redirected, and returns
// the file descriptor
//
// Find out what permissions it needs.
// Does it have to be closed after the execve(2) call?
//
// Hints:
// - if O_CREAT is used, add S_IWUSR and S_IRUSR
// 	to make it a readable normal file
static int
open_redir_fd(char *file, int flags)
{
	int fd;
	if (flags & O_CREAT) {
		fd = open(file, flags, S_IWUSR | S_IRUSR);
	} else {
		fd = open(file, flags);
	}

	if (fd < 0) {
		perror("Error al abrir el archivo");
		exit(-1);
	}

	return fd;
}

// executes a command - does not return
//
// Hint:
// - check how the 'cmd' structs are defined
// 	in types.h
// - casting could be a good option
void
exec_cmd(struct cmd *cmd)
{
	// To be used in the different cases
	struct execcmd *e;
	struct backcmd *b;
	struct execcmd *r;
	struct pipecmd *p;

	switch (cmd->type) {
	case EXEC: {
		// spawns a command
		e = (struct execcmd *) cmd;
		if (e->argv[0] == NULL) {
			fprintf(stderr, "Error: comando vacio\n");
			exit(1);
		}
		set_environ_vars(e->eargv, e->eargc);
		if (execvp(e->argv[0], e->argv) == -1) {
			perror("execvp");
			_exit(-1);
		}
		break;
	}
	case BACK: {
		b = (struct backcmd *) cmd;
		exec_cmd(b->c);
		exit(-1);
		break;
	}

	case REDIR: {
		// changes the input/output/stderr flow
		int new_fd_input;
		int new_fd_output;
		int new_fd_error;
		// int dup_result = 0;

		r = (struct execcmd *) cmd;

		char *output_file = r->out_file;
		char *input_file = r->in_file;
		char *error_file = r->err_file;

		bool has_output_redirection = strlen(output_file) > 0;
		bool has_input_redirection = strlen(input_file) > 0;
		bool has_error_redirection_to_file =
		        strlen(error_file) > 0 && error_file[0] != '&';
		bool has_error_redirection_to_stdout =
		        strlen(error_file) > 0 && error_file[0] == '&';

		if (has_output_redirection) {
			new_fd_output = open_redir_fd(output_file,
			                              O_CLOEXEC | O_CREAT |
			                                      O_WRONLY | O_TRUNC);
			if (new_fd_output == -1) {
				exit(-1);
			}

			check_dup2(dup2(new_fd_output, STDOUT_FILENO),
			           new_fd_output);
		}
		if (has_error_redirection_to_file) {
			new_fd_error = open_redir_fd(error_file,
			                             O_CLOEXEC | O_CREAT |
			                                     O_RDWR | O_TRUNC);
			if (new_fd_error == -1) {
				exit(-1);
			}

			check_dup2(dup2(new_fd_error, STDERR_FILENO),
			           new_fd_error);
		}
		if (has_input_redirection) {
			new_fd_input =
			        open_redir_fd(input_file, O_CLOEXEC | O_RDONLY);
			if (new_fd_input == -1) {
				exit(-1);
			}

			check_dup2(dup2(new_fd_input, STDIN_FILENO), new_fd_input);
		}

		if (has_error_redirection_to_stdout) {
			if (dup2(STDOUT_FILENO, STDERR_FILENO) < 0) {
				exit(-1);
			}
		}

		if (execvp(r->argv[0], r->argv) < 0) {
			perror("execvp failed");
			exit(-1);
		}
		exit(0);
		break;
	}

	case PIPE: {
		// pipes two commands
		p = (struct pipecmd *) cmd;

		int fds[2];
		int pid1;
		int pid2;

		if (pipe(fds) < 0) {
			perror("Error");
		}

		pid1 = fork();
		if (pid1 == 0) {
			// izquierdo
			close(fds[READ]);

			check_dup2(dup2(fds[WRITE], STDOUT_FILENO), fds[WRITE]);

			exec_cmd(p->leftcmd);
			exit(0);

		} else if (pid1 > 0) {
			// derecho
			close(fds[WRITE]);

			// para posibles comando/s a la derecha
			pid2 = fork();
			if (pid2 == 0) {
				check_dup2(dup2(fds[READ], STDIN_FILENO),
				           fds[READ]);

				struct cmd *right_cmd =
				        parse_line(p->rightcmd->scmd);
				exec_cmd(right_cmd);
				exit(0);

			} else if (pid2 > 0) {
				int status1, status2;
				waitpid(pid1, &status1, 0);
				waitpid(pid2, &status2, 0);

			} else {
				perror("Error");
				exit(-1);
			}
		} else {
			perror("Error");
			exit(-1);
		}

		// free the memory allocated
		// for the pipe tree structure
		free_command(parsed_pipe);
		exit(0);
		break;
	}
	}
}
