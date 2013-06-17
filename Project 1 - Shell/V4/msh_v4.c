#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include "libmsh_v4.h"

#define PROMPT "> \0"


int main(int argc, char *argv[]) {
  	// Instalacao de um handler para o SIGINT.
	if(signal(SIGINT, sig_handler_parent) == SIG_ERR) {
		printf("parent: sigint handler installation failed\n");
		exit(-1);
	}
	
	char * cmd_line;
	Command_Info cmd_info;

	while(1) {
		cmd_line = read_line(PROMPT);	// Libertar memória deste apontador.
  
		// Comando nao e vazio.
		if(strcmp(cmd_line, "") != 0) {	
			// Verificação se o comando é válido.
			if(parse_cmd(cmd_line, &cmd_info) != -1) {
				free(cmd_line);					// Libertar cmd_line.
				//print_cmd_info(&cmd_info);	// Útil para debugging.

				// Sair do ciclo?
				if (strcmp(cmd_info.arg[0], "exit") == 0) {
					return 0;
				}

				int pid = exec_simple_sig(&cmd_info);
				if(pid == -1) { // O fork nao falhou?
					printf("Fork failed!\n");
				}
				// Se é pai, espera pelo filho.
				else {
					child_pid = pid;	// Usado para matar o filho.

					if(waitpid(pid, NULL, 0) == -1) {
						perror("waitpid");
					}
				}
				free_cmd_info(&cmd_info);	// Libertar command_info.
			}
			else {
				printf("Invalid command!\n");
				free(cmd_line);
			}
		}
	}
	return 0;
}
