#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "libmsh_v6.h"


void free_cmd_info_pip(Command_Info_Pip * cmd_info) {
	int i = 0;
	while(cmd_info->arg[i] != NULL) {
		free(cmd_info->arg[i]);
		i++;
	}
	if(cmd_info->infile != NULL) {
		free(cmd_info->infile);
	}
	if(cmd_info->outfile != NULL) {
		free(cmd_info->outfile);
	}
	if(cmd_info->next != NULL) {
		free_cmd_info_pip(cmd_info->next);
	}
}


void print_cmd_info_pip(Command_Info_Pip *cmd_info) {
	int i = 0;
	while(cmd_info->arg[i] != NULL) {
		printf("%s ", cmd_info->arg[i]);
		fflush(stdout);
		i++;
	}
	if(cmd_info->infile != NULL) {
		printf("< %s ", cmd_info->infile);
	}
	if(cmd_info->outfile != NULL) {
		printf("> %s ", cmd_info->outfile);
	}
	if(cmd_info->background == 1) {
		printf("& ");
	}
	if(cmd_info->next == NULL) {
		printf("\n");
	}
	if(cmd_info->next != NULL) {
		printf("| ");
		fflush(stdout);
		print_cmd_info_pip(cmd_info->next);
	}
}

Command_Info_Pip * parse_cmd_pip(char * cmd_line_without_pipes) {
	// Construção dos tokens da linha de comandos.
	int num_tokens;
	char ** tokens = tokenize(cmd_line_without_pipes, ' ', &num_tokens);

	// Parsing falhou?	
	if(tokens == NULL) {
		return NULL;
	}

	// Verificacao dos tokens validos.
	int num_good_tokens = check_valid_cmd(tokens, num_tokens);
	
	// Verificacao admite que os tokens são maus?
	if(num_good_tokens <= 0) {
		return NULL;
	}

	// Criacao da estrutura.
	Command_Info_Pip * cmd_info_pip = (Command_Info_Pip *) malloc(sizeof(Command_Info_Pip));

	// Criacao do array de apontadores.
	cmd_info_pip->arg = (char **) malloc((num_good_tokens + 1) * sizeof(char *));

	// Valores por defeito.
	int i;
	for(i = 0; i < (num_good_tokens + 1); i++) {
		cmd_info_pip->arg[i] = NULL;
	}

	cmd_info_pip->infile = NULL;
	cmd_info_pip->outfile = NULL;
	cmd_info_pip->background = 0;
	cmd_info_pip->previous = NULL;
	cmd_info_pip->next = NULL;

	// Adicionar tokens a estrutura.
	int j = 0;
	for(i = 0; i < num_tokens; i++) {
		if(strcmp(tokens[i], ">") == 0) {
			i++;	
			cmd_info_pip->outfile = tokens[i];
		}
		else if(strcmp(tokens[i], "<") == 0) {
			i++;	
			cmd_info_pip->infile = tokens[i];
		}
		else if(strcmp(tokens[i], "&") == 0) {
			// E garantido pelo check_valid_cmd que nao existem tokens & no meio.
			cmd_info_pip->background = 1;
		}
		else {
			cmd_info_pip->arg[j] = tokens[i];
			j++;
		}
	}

	free(tokens); 	// Libertar tokens.
	return cmd_info_pip;
}


Command_Info_Pip * parse_cmds_pip(char * cmd_line_with_pipes) {
	int num_cmds_tokens;
	char ** cmds_tokens = tokenize(cmd_line_with_pipes, '|', &num_cmds_tokens);

	if(num_cmds_tokens == -1) {
		return NULL;
	}

	Command_Info_Pip * current_cmd_info;
	Command_Info_Pip * last_cmd_info;
	Command_Info_Pip * first_cmd_info;
	
	// Fazer o parsing de cada comando e construção da lista ligada respectiva.
	int i;
	for(i = 0; i < num_cmds_tokens; i++) {
		current_cmd_info = parse_cmd_pip(cmds_tokens[i]);

		// Verificar se o parsing foi correcto.
		if(current_cmd_info == NULL) {
			// Houve problemas no parsing, retornar mas antes libertar apontadores.
			// Libertar a estrutura que não foi preenchida.
			free(current_cmd_info);
			if(i != 0) {
				free_cmd_info_pip(first_cmd_info);
			}

			return NULL;
		}
		else {
			// Se nao houve problemas no parsing, ligamos as estruturas.
			if(i == 0) {
				// current_cmd_info->previous = NULL.
				first_cmd_info = current_cmd_info;
				last_cmd_info = current_cmd_info;
			}
			else {
				current_cmd_info->previous = last_cmd_info;
				last_cmd_info->next = current_cmd_info;
				last_cmd_info = current_cmd_info;
			}
		}
	}

	free(cmds_tokens); 	// !TODO
	return first_cmd_info;
}


int create_pipes(Command_Info_Pip * cmd_info) {
	if(cmd_info->next) {	
		// Criar pipe um pipe para a saída do comando actual.
		int fd[2];

		if(pipe(fd) == -1) {
			perror("pipe");
			return -1;
		}

		cmd_info->fd_output[READ] = fd[READ];
		cmd_info->fd_output[WRITE] = fd[WRITE];

		cmd_info->next->fd_input[READ] = fd[READ];
		cmd_info->next->fd_input[WRITE] = fd[WRITE];

		return create_pipes(cmd_info->next);
	}
	else {
		return 0;
	}
}

int close_pipes(Command_Info_Pip * cmd_info) {
	if(cmd_info->next) {
		if(close(cmd_info->fd_output[WRITE]) == -1) {
			perror("close: pipe: write");
			return -1;
		}

		if(close(cmd_info->next->fd_input[READ]) == -1) {
			perror("close: pipe: read");
			return -1;
		}
		
		return close_pipes(cmd_info->next);
	}
	return 0;
}




