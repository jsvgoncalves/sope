#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "libmsh_v5.h"

#define PROMPT "> \0"


int main(int argc, char *argv[]) {
  	// Instalacao de um handler para o SIGINT.
	if(signal(SIGINT, sig_handler_parent) == SIG_ERR) {
		printf("parent: sigint handler installation failed\n");
		exit(-1);
	}

	sig_t old_handler;
	int sigchild_handler_installed=0;
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
				if(strcmp(cmd_info.arg[0], "exit") == 0) {
					return 0;
				}

				int pid = 0;
				int background_pid, foreground_pid;
				if(cmd_info.background == 1) {
					
					// Instalacao de um handler para o SIGCHLD.
					// Assim os filhos não ficam zombies.
					if((old_handler = signal(SIGCHLD, SIG_IGN)) == SIG_ERR){  
						printf("parent: sigchld handler installation failed\n");
						exit(-1);
					}
					sigchild_handler_installed = 1;
					background_pid = exec_simple_back(&cmd_info); //Executa em background
					pid = background_pid;
				}
				else {
					if(sigchild_handler_installed == 1) {
						if((signal(SIGCHLD, old_handler)) == SIG_ERR) {
							printf("parent: sigchld handler installation failed\n");
							exit(-1);
						}	
						sigchild_handler_installed = 0;
					}
					foreground_pid = exec_simple_sig(&cmd_info);
					pid = foreground_pid;
				}
			
				// O fork não falhou?
				if(pid == -1) {
					printf("Fork failed!\n");
				}
				// Se e pai, ja nao espera pelo filho.
				else {
					
					// Se acabamos de iniciar processo em background, imprimir informacao.
					if(cmd_info.background == 1) {
						print_cmd_info(&cmd_info);
						printf("[%i] Process executing in background.\n", background_pid);
					}
					else {
						child_pid = foreground_pid;	// Usado para matar o filho com SIGKILL.
						
						if(waitpid(foreground_pid, NULL, 0) == -1) {
							perror("waitpid");
						}
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
