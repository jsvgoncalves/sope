#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>


#include "libmsh_v6.h"

#define PROMPT "> \0"
int is_background(Command_Info_Pip * first_cmd_info) {
	Command_Info_Pip * tmp_cmd_info = first_cmd_info;
	while(tmp_cmd_info->next) {
		tmp_cmd_info = tmp_cmd_info->next;
	}
	return tmp_cmd_info->background;
}

int child_no=0;

int exec_multiple(Command_Info_Pip * first_cmd_info) {
	Command_Info_Pip * cmd_info = first_cmd_info;
	int background = is_background(first_cmd_info);
	int fdi, fdo;
	int pid, child_number = 0;
	if(background){
		printf("Processes are executing in background.\n");
	}
	while(cmd_info) {
		pid = fork();
		
		if(pid == -1){
			printf("Fork failed");
			return -1;
		}
		else if(pid == 0) {
			// Fecha o pipe anterior de escrita.
			if(cmd_info->previous) {
				close(cmd_info->previous->fd_output[WRITE]);
				dup2(cmd_info->previous->fd_output[READ], STDIN_FILENO);
			}
			else {
				/* Se nao houver estrutura anterior, entao e a primeira estrutura
				 * e pode haver redirecionamento.
				 */ 
				if(background) {
					if((fdi = open("/dev/null", O_RDONLY)) == -1) {			  
						perror("open: /dev/null");
						exit(2);
					}					
				}
				else if(cmd_info->infile != NULL){
					if((fdi = open(cmd_info->infile, O_RDONLY))==-1) {			  
						perror("open: infile");
						exit(2);
					}
				}
				dup2(fdi, STDIN_FILENO);
			}
					
			// Fecha o pipe posterior de leitura.
			if(cmd_info->next) {
				close(cmd_info->fd_output[READ]);
				dup2(cmd_info->fd_output[WRITE], STDOUT_FILENO);
			}
			else {
				/* Se nao houver estrutura seguinte, entao e a ultima estrutura
				 * e pode haver redirecionamento.
				 */
				if(background){
					if((fdo = open("bckgd_redir.txt", O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR)) == -1) {			  
						perror("open: bckgd_redir.txt");
						exit(2);
					}					
				}					
				else if(cmd_info->outfile != NULL) {
					if((fdo = open(cmd_info->outfile, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR)) == -1) {			  
						perror("open: outfile");
						exit(2);
					}
				}
				dup2(fdo, STDOUT_FILENO);
				dup2(fdo, STDERR_FILENO);
			}

			// Executar o comando respectivo.
			if(execvp(cmd_info->arg[0], cmd_info->arg) == -1) {
				perror("execvp");
				exit(-1);
			}
		}
		else {
			child_number++;
			cmd_info = cmd_info->next;
		}
	}
	return child_number;
}

void child_counter_handler(int sig) {
	child_no--;
} 

int main() {
	char * cmd_line;
	sig_t old_handler;
	Command_Info_Pip * first_cmd_info;
	int child_temp;
	

	//Instalar handler pro filho
	if((old_handler = signal(SIGCHLD, child_counter_handler)) == SIG_ERR){  
		printf("parent: sigchld handler installation failed\n");
		exit(-1);
	}
	
	while(1) {
		cmd_line = read_line(PROMPT);	// Libertar memória deste apontador.
		// Comando não é vazio.
		if(strcmp(cmd_line, "") != 0) {	
			// Verificação se o comando é válido.
			if((first_cmd_info = parse_cmds_pip(cmd_line)) != NULL) {
				//print_cmd_info_pip(first_cmd_info);	// Útil para debugging.
				
				// Sair do ciclo
				if (strcmp(first_cmd_info->arg[0], "exit") == 0) {
					return 0;
				}
				
				if(create_pipes(first_cmd_info) == -1) {
					printf("Failed to create pipes.\n");
					exit(-1);
				}
				else {
					child_no = exec_multiple(first_cmd_info);
					child_temp = child_no;
					/* So e retornado ao wait o sinal de um filho, 
					 * como tal, usamos esta varivel para comparar
					 */
				}
				
				// O fork não falhou?
				if(child_temp == -1) {
					printf("Fork failed!\n");
					
				}
				else { // Tem de esperar pelos child_no filhos
					while(child_no == child_temp);
					child_no=0;
				}


				free_cmd_info_pip(first_cmd_info);	// Libertar command_info_pips.
			}

			free(cmd_line);	// Libertar cmd_line.
		}
	}
	return 0;
}

