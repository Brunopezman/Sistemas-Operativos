#include "defs.h"
#include "types.h"
#include "readline.h"
#include "runcmd.h"
#include <errno.h>

char prompt[PRMTLEN] = { 0 };
void print_prompt(void);
void sigchld_handler(int signum);
void check_write(ssize_t result);

void
check_write(ssize_t result)
{
	if (result == -1) {
		perror("write");
		exit(1);
	}
}

void
print_prompt()
{
	check_write(write(STDERR_FILENO, " ", 1));
	check_write(write(STDERR_FILENO, COLOR_RED, strlen(COLOR_RED)));
	check_write(write(STDERR_FILENO, prompt, strlen(prompt)));
	check_write(
	        write(STDERR_FILENO, COLOR_RESET "\n$ ", strlen(COLOR_RESET) + 3));
}

void
sigchld_handler(int signum)
{
	pid_t pid;
	int status;
	char str[BUFLEN];
	(void) signum;

	if (pgid_back == -1)
		return;

	while ((pid = waitpid(-pgid_back, &status, WNOHANG)) > 0) {
		int len =
		        snprintf(str, sizeof str, "\r==> terminado: PID=%d\n", pid);
		if (len > 0)
			check_write(write(STDERR_FILENO, str, len));

		print_prompt();

		// Verificamos si ya no quedan procesos en ese grupo.
		errno = 0;
		pid = waitpid(-pgid_back, &status, WNOHANG);
		if (pid == -1 && errno == ECHILD) {
			// No quedan procesos en el grupo de background
			pgid_back = -1;
		}
	}
}

// runs a shell command
static void
run_shell()
{
	char *cmd;
	struct sigaction sa;
	memset(&sa, 0, sizeof sa);
	sa.sa_handler = sigchld_handler;
	sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;

	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}
	while ((cmd = read_line(prompt)) != NULL)
		if (run_cmd(cmd) == EXIT_SHELL)
			return;
}

// initializes the shell
// with the "HOME" directory
static void
init_shell()
{
	char buf[BUFLEN] = { 0 };
	char *home = getenv("HOME");

	if (chdir(home) < 0) {
		snprintf(buf, sizeof buf, "cannot cd to %s ", home);
		perror(buf);
	} else {
		snprintf(prompt, sizeof prompt, "(%s)", home);
	}
}

int
main(void)
{
	init_shell();

	run_shell();

	return 0;
}
