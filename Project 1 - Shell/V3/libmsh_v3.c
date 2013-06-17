#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "libmsh_v3.h"


pid_t exec_simple_redir(Command_Info * cmd_info) {
	int pid = fork();
	if(pid == -1) {
		perror("fork");
		return -1;
	}
	else if(pid == 0) {
		// Verifica se há redirecionamento
		int fdi, fdo;
		if(cmd_info->infile != NULL) {
			if((fdi = open(cmd_info->infile, O_RDONLY))==-1) {			  
				perror("open: infile");
				exit(2);
			}
			dup2(fdi, STDIN_FILENO);
		}
		if(cmd_info->outfile != NULL) {
			if((fdo = open(cmd_info->outfile, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR)) == -1) {			  
				perror("open: outfile");
				exit(2);
			}
			dup2(fdo, STDOUT_FILENO);
		}

		if(execvp(cmd_info->arg[0], cmd_info->arg) == -1) {
			perror("execvp");
			return 1;
		}
		else {
			return 0;
		}
	}
	else {
		return pid;
	}
}
