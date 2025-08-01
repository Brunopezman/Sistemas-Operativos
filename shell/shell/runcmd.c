#include "runcmd.h"

int status = 0;
struct cmd *parsed_pipe;
pid_t pgid_back = -1;

// runs the command in 'cmd'
int
run_cmd(char *cmd)
{
	pid_t p;
	struct cmd *parsed;

	// if the "enter" key is pressed
	// just print the prompt again
	if (cmd[0] == END_STRING)
		return 0;

	// "history" built-in call
	if (history(cmd))
		return 0;

	// "cd" built-in call
	if (cd(cmd))
		return 0;

	// "exit" built-in call
	if (exit_shell(cmd))
		return EXIT_SHELL;

	// "pwd" built-in call
	if (pwd(cmd))
		return 0;

	// parses the command line
	parsed = parse_line(cmd);

	// forks and run the command
	if ((p = fork()) == 0) {
		// keep a reference
		// to the parsed pipe cmd
		// so it can be freed later
		if (parsed->type == PIPE)
			parsed_pipe = parsed;

		if (parsed->type == BACK) {
			// Crear un nuevo grupo de procesos para todos los procesos en background
			setpgid(0, (pgid_back == -1) ? 0 : pgid_back);
		}

		exec_cmd(parsed);
	}

	// stores the pid of the process
	parsed->pid = p;

	// background process special treatment
	// Hint:
	// - check if the process is
	//		going to be run in the 'back'
	// - print info about it with
	// 	'print_back_info()'
	//
	// Your code here

	// waits for the process to finish
	if (parsed->type == BACK) {
		if (pgid_back == -1) {
			// Primer proceso en background: se usa su PID como grupo
			setpgid(p, p);
			pgid_back = p;
		} else {
			// No es el primero: asignarlo al grupo ya existente.
			setpgid(p, pgid_back);
		}
		print_back_info(parsed);
	} else {
		waitpid(p, &status, 0);
		print_status_info(parsed);
	}
	free_command(parsed);

	return 0;
}
