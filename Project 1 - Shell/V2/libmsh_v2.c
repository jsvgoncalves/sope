#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

#include "libmsh_v2.h"


pid_t exec_simple(Command_Info * cmd_info) {
	int pid = fork();
	if(pid == -1) {
		perror("fork");
		return -1;
	}
	else if(pid == 0) {
		if(execvp(cmd_info->arg[0], cmd_info->arg) == -1) {
			perror("execvp");
			exit(1);
		}
		else {
			return 0;
		}
	}
	else {
		return pid;
	}
}

