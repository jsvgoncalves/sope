#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#include "libmsh_v5.h"


pid_t exec_simple_back(Command_Info * cmd_info) {
	int pid = fork();			

	if(pid == -1) {
		perror("fork");
		return -1;
	}
	else if(pid == 0) {
		// Instalar handler para ignorar SIGINT.
		if(signal(SIGINT, SIG_IGN) == SIG_ERR) {	
		   printf("signal: child: sigint ignore failed\n"); 
		   exit(-1);
		}		
		
		int fdi, fdo;
		// Se nao houver redirecionamento de entrada abre /dev/null.
		if(cmd_info->infile == NULL) {
			if((fdi = open("/dev/null", O_RDONLY)) == -1) {			  
				perror("open: /dev/null");
				exit(2);
			}
		}
		// Se houver redirecionamento de entrada abre ficheiro.
		else {
			if((fdi = open(cmd_info->infile, O_RDONLY)) == -1) {
				perror("open: file");
				exit(2);
			}
		}
		dup2(fdi, STDIN_FILENO);  
		
		// Testar o redirecionamento de saída.
		if(cmd_info->outfile == NULL) {	
			if((fdo = open("bckgd_redir.txt", O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR)) == -1) {			  
				perror("open: bckgd_redir.txt");
				exit(2);
			}
		}
		else {
			if((fdo = open(cmd_info->outfile, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR)) == -1) {			  
			  perror("open: outfile");
			  exit(2);
			}
		}
		dup2(fdo, STDOUT_FILENO);
		dup2(fdo, STDERR_FILENO);

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

