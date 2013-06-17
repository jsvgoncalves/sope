#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "libmsh_v1.h"

#define MAX_READ_LINE 1024

void free_cmd_info(Command_Info *cmd_info) {
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
}

void print_cmd_info(Command_Info *cmd_info) {
	int i = 0;
	while(cmd_info->arg[i] != NULL) {
		printf("%s ", cmd_info->arg[i]);
		i++;
	}
	if(cmd_info->infile != NULL) {
		printf("< %s ", cmd_info->infile);
	}
	if(cmd_info->outfile != NULL) {
		printf("> %s ", cmd_info->outfile);
	}
	if(cmd_info->background == 1) {
		printf("&");
	}
	printf("\n");
}

/*
char ** tokenize(char * string, char delimiter,  int * num_tokens) {
	printf("In tokenizer: Original string: [%s]\n", string);
	// Fazer cópias da string, strtok modifica a string de entrada.
	char string_copy[strlen(string) + 1];	// Verificação do numero de tokens.
	char string_copy2[strlen(string) + 1];	// Passagem de tokens para um array

	strcpy(string_copy, string);
	strcpy(string_copy2, string);

	// Verificar quantos tokens temos.
	*num_tokens = 0;
	char * token = strtok(string_copy, &delimiter);

	// Existe um token?
	if(token == NULL) {
		return NULL;
	}

	*num_tokens += 1;
	while(strtok(NULL, &delimiter) != NULL) {
		*num_tokens += 1;
	}

	// Criar o apontador dos tokens.
	char **tokens = (char **) malloc(*num_tokens * sizeof(char *));
	token = strtok(string_copy2, &delimiter);
	tokens[0] = (char *) malloc(strlen(token) + 1);
	strcpy(tokens[0], token);
	printf("In tokenizer 0: [%s]\n", tokens[0]);
	int i;
	for(i = 1; i < *num_tokens; i++) {
		token = strtok(NULL, &delimiter);
		tokens[i] = (char *) malloc(strlen(token) + 1);
		strcpy(tokens[i], token);
		printf("In tokenizer %i: [%s]\n", i, tokens[i]);
	}

	return tokens;
}*/


char ** tokenize(char * string, char delimiter,  int * num_tokens) {
	// Contar o número de tokens.
	*num_tokens = 0;
	int last_is_delimiter = 1;
	char * ptr = string;
	while(*ptr != '\0') {
		if(*ptr == delimiter) {
			// Se for o limitador, e se o último token não é um
			// delimitador, terminou aqui um token.
			if(last_is_delimiter == 0) {
				last_is_delimiter = 1;
			}
		}
		else if(*ptr != delimiter) {
			// Se não for delimitador, e o último token é um
			// delimitador, começou aqui um token.
			if(last_is_delimiter == 1) {
				last_is_delimiter = 0;
				*num_tokens += 1;
			}
		}
		ptr++;
	}

	// Construir os apontadores para os tokens.
	char ** tokens = (char **) malloc(*num_tokens * sizeof(char *));
	int token_index = 0;

	int start, end, length;
	int index = 0;
	last_is_delimiter = 1;
	ptr = string;
	while(1) {

		if(*ptr == delimiter || *ptr == '\0') {
			// Se for o limitador, e se o último token não é um
			// delimitador, terminou aqui um token.
			if(last_is_delimiter == 0) {
				last_is_delimiter = 1;
				end = index - 1;

				length = end - start + 1;   // +1 po que o start também conta.
				// Alocámos length + 1 por causa do '\0'.
				tokens[token_index] = (char *) malloc((length + 1) * sizeof(char));

				memcpy(tokens[token_index], &string[start], length);
				tokens[token_index][length] = '\0';

				token_index += 1;
			}
		}
		else if(*ptr != delimiter) {
			// Se não for delimitador, e o último token é um
			// delimitador, começou aqui um token.
			if(last_is_delimiter == 1) {
				last_is_delimiter = 0;
				start = index;
			}
		}

		if(*ptr == '\0')
			break;

		ptr++;
		index++;
	}
	return tokens;
}

int check_valid_cmd(char ** tokens, int num_tokens) {
	// Verificar se existem tokens correctos depois do "<" e ">"
	int next_position;
	char * next_token;
	int i;
	for(i = 1; i < num_tokens; i++) {
		if((strcmp(tokens[i], ">") == 0) ||
				(strcmp(tokens[i], "<") == 0)) {
			next_position = i + 1;
			// "<" e ">" como últimos tokens?
			if(next_position == num_tokens) {
				return 0;
			}
			else {
				next_token = tokens[next_position];
				// Verificar se tem tokens válidos a seguir.
				if((strcmp(next_token, ">") == 0 ) ||
						(strcmp(next_token, "<") == 0) ||
						(strcmp(next_token, "&") == 0)) {
					return 0;
				}
			}
		}
	}

	// Contar numero de ">", "<" e "&".
	int great = 0, less = 0, and = 0;
	for(i = 0; i < num_tokens; i++) {
		if(strcmp(tokens[i], ">") == 0) {
			great++;
		}
		else if(strcmp(tokens[i], "<") == 0) {
			less++;
		}
		else if(strcmp(tokens[i], "&") == 0) {
			and++;
		}
	}

	// Verificação do token ">"
	if(great > 1) {
		return 0;
	}
	// Verificação do token "<"
	if(less > 1) {
		return 0;
	}
	// Verificação do token "&".
	if(and > 1) {
		return 0;
	}
	else if(and == 1) {
		if(strcmp(tokens[num_tokens - 1], "&") != 0) {
			return 0;
		}
	}

	return num_tokens - great - less - and;
}


int parse_cmd(char *cmd_line, Command_Info *cmd_info) {
	// Construção dos tokens da linha de comandos.
	int num_tokens;
	char ** tokens = tokenize(cmd_line, ' ', &num_tokens);	// Libertar este apontador.

	// Parsing falhou?
	if(tokens == NULL) {
		return -1;
	}
	// Verificação se os tokens são válidos.
	int num_good_tokens = check_valid_cmd(tokens, num_tokens);
	// Verificação admite que os tokens são maus?
	if(num_good_tokens == 0) {
		return -1;
	}

	// Criação do array de apontadores.
	cmd_info->arg = (char **) malloc((num_good_tokens + 1) * sizeof(char *));

	// Valores por defeito.
	int i;
	for(i = 0; i < (num_good_tokens + 1); i++) {
		cmd_info->arg[i] = NULL;
	}
	cmd_info->infile = NULL;
	cmd_info->outfile = NULL;
	cmd_info->background = 0;

	// Adicionar tokens à estrutura.
	int j = 0;
	for(i = 0; i < num_tokens; i++) {
		if(strcmp(tokens[i], ">") == 0) {
			i++;
			cmd_info->outfile = tokens[i];
		}
		else if(strcmp(tokens[i], "<") == 0) {
			i++;
			cmd_info->infile = tokens[i];
		}
		else if(strcmp(tokens[i], "&") == 0) {
			// E garantido pelo check_valid_cmd que nao existem tokens & no meio.
			cmd_info->background = 1;
		}
		else {
			cmd_info->arg[j] = tokens[i];
			j++;
		}
	}

	free(tokens);
	return num_good_tokens;
}


char * read_line(const char * prompt) {
	printf("%s", prompt);
	if(fflush(STDIN_FILENO) != 0) {
		perror("fflush");
		return NULL;
	}
	char buf[MAX_READ_LINE];
	if(fgets(buf, MAX_READ_LINE, stdin) == NULL) {
		return NULL;
	}

	// Retirar o "\n" do fgets.
	buf[strlen(buf) - 1] = '\0';

	char * line;
	if((line = (char *) malloc(strlen(buf) + 1)) == NULL) {
		return NULL;;
	}
	return strcpy(line, buf);
}

