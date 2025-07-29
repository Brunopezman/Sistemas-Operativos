#ifndef RUNCMD_H
#define RUNCMD_H

#include "defs.h"
#include "parsing.h"
#include "exec.h"
#include "printstatus.h"
#include "freecmd.h"
#include "builtin.h"

int run_cmd(char *cmd);

extern pid_t pgid_back;

#endif  // RUNCMD_H
