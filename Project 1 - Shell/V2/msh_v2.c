#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "libmsh_v2.h"

#define PROMPT "> \0"

int main(int argc, char *argv[]) {
	char * cmd_line;
	Command_Info cmd_info;

	while(1) {
		cmd_line = read_line(PROMPT);	// Libertar memória deste apontador.

		if(strcmp(cmd_line, "") != 0) {	// Comando não é vazio.
			if(parse_cmd(cmd_line, &cmd_info) != -1) { // Verificação se o comando é válido.
				free(cmd_line);					// Libertar cmd_line.
				//print_cmd_info(&cmd_info);	// Útil para debugging.

				if (strcmp(cmd_info.arg[0], "exit") == 0) { // Sair do ciclo?
					return 0;
				}

				int pid = exec_simple(&cmd_info);
				
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

