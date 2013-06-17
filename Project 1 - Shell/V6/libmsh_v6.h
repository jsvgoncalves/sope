#include "../V5/libmsh_v5.h"

#define READ 0
#define WRITE 1


struct command_info {
	char **arg;		// Lista de argumentos (terminada com NULL).
	char *infile;	// Fich. p/ onde redirecionar stdin ou NULL.
	char *outfile;	// Fich. p/ onde redirecionar stdout ou NULL.
	int background;	// 0 ou 1 (procedimento a executar em background).
	int fd_input[2];	// Descritores de leitura do pipdo actual.
	int fd_output[2];	// Descritores de escrita do actual.
	struct command_info * next; 	// Aponta para a próxima estrutura.
	struct command_info * previous; // Aponta para a estrutura anterior.
};

typedef struct command_info Command_Info_Pip;


/* Liberta a memória usada na estrutura Command_Info_Pip.
 */
void free_cmd_info_pip(Command_Info_Pip * cmd_info);


/* Imprime a estrutura do comando.
 * Util para debug.
 *
 * A estrutura tem de estar preenchida!
 */
void print_cmd_info_pip(Command_Info_Pip * cmd_info);


/* Constroi uma estrutura Command_Info_Pip que representa
 * um comando introduzido na shell.
 * Retorna NULL se não corresponder a um comando válido.
 */
Command_Info_Pip * parse_cmd_pip(char * cmd_line_without_pipes);


/* Constroi uma lista duplamente ligada de Command_Info_Pip.
 * Cada uma destas estruturas representa um comando a ser
 * executado dividido por pipes.
 * Em caso de erro, retorna NULL se não corresponder a um comando válido.
 */
Command_Info_Pip * parse_cmds_pip(char * cmd_line_with_pipes);

/* Esta funcao cria os pipes segundo a estrutura de lista ligada
 * de Command_Info_Pip e armazena os descritores de forma apropriada.
 * Retorn -1 em caso de erro.
 */
int create_pipes(Command_Info_Pip * cmd_info);

/* Esta funcao fecha os pipes que foram utilizados para comunicacao entre processos.
 */
int close_pipes(Command_Info_Pip * cmd_info);


