
typedef struct {
	char **arg;	// Lista de argumentos (terminada com NULL).
	char *infile;	// Fich. p/ onde redirecionar stdin ou NULL.
	char *outfile;	// Fich. p/ onde redirecionar stdout ou NULL.
	int background; // 0 ou 1 (procedimento a executar em background).
} Command_Info;


/* Liberta a memória usada na estrutura Command_Info.
 */
void free_cmd_info(Command_Info *cmd_info);


/* Imprime a estrutura do comando.
 * Util para debug.
 *
 * A estrutura tem de estar preenchida!
 */
void print_cmd_info(Command_Info *cmd_info);


/* Esta função recebe uma string e retorna um apontador
 * para apontadores que apontam para cada um dos tokens
 * da string de entrada separados pelo delimitador dado.
 * Preenche num_tokens com o número de tokens apropriado.
 *
 * É preciso libertar o apontador de retorno e respectivos
 * apontadores dos tokens resultantes.
 */
char ** tokenize(char * string, char delimiter,  int * num_tokens);


/* Verifica se o apontador que contêm os tokens de um comando 
 * representa um comando válido.
 * Retorna 0 em caso de comando inválido ou o número de bons tokens, 
 * i.e. não contando os tokens "<", ">", * "&".
 */
int check_valid_cmd(char ** tokens, int num_tokens);


/* Constroi uma estrutura Command_Info que representa
 * um comando introduzido na shell.
 * Retorna -1 se não corresponder a um comando válido.
 */
int parse_cmd(char *cmd_line, Command_Info *cmd_info);


/* Escreve a prompt e le uma linha do stdin.
 * Retorna o apontador para a string lida ou NULL.
 * O apontador resultante desta função precisa ser libertado
 * pela função que evoca o read_line.
 */
char * read_line(const char * prompt);

