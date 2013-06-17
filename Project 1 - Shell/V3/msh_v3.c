#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>

#include "libmsh_v3.h"

#define PROMPT "> \0"


int main(int argc, char *argv[]) {
	char * cmd_line;
	Command_Info cmd_info;

	while(1) {
		cmd_line = read_line(PROMPT);	// Libertar memória deste apontador.
  
		// Comando não é vazio.
		if(strcmp(cmd_line, "") != 0) {	
			// Verificação se o comando é válido.
			if(parse_cmd(cmd_line, &cmd_info) != -1) {
				free(cmd_line);					// Libertar cmd_line.
				//print_cmd_info(&cmd_info);	// Útil para debugging.

				// Sair do ciclo?
				if (strcmp(cmd_info.arg[0], "exit") == 0) {
					return 0;
				}

				int pid = exec_simple_redir(&cmd_info);
						
				if(pid == -1) { // O fork nao falhou?
					printf("Fork failed!\n");
				}	
				else { 			//Se nao falhou, espera pelo filho.
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

