#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "libmsh_v1.h"

#define PROMPT "> \0"


int main(int argc, char *argv[]) {
	char * cmd_line;
	Command_Info cmd_info;

	int exit = 0;
	while(!exit) {
		cmd_line = read_line(PROMPT);	// Libertar memória deste apontador.

		// Comando não é vazio.
		if(strcmp(cmd_line, "") != 0) {
			// Verificação se o comando é válido.
			if(parse_cmd(cmd_line, &cmd_info) != -1) {
				free(cmd_line);				// Libertar cmd_line.
				print_cmd_info(&cmd_info);	// Útil para debugging.

				/**
				 * Executar aqui os comandos.
				 */


				// Sair do ciclo
				if (strcmp(cmd_info.arg[0], "exit") == 0) {
					exit = 1;
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

