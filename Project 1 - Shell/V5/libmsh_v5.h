#include "../V4/libmsh_v4.h"

/* Executa um comando em background com possibilidade
 * de redireccionamento e com a possibilidade de enviar
 * um sinal SIGKILL ao filho.
 * Caso nao haja redirecionamento de entrada, o output
 * da execucao sera direccionado para dentro do ficheiro
 * "bckgd_redir.txt".
 */
pid_t exec_simple_back(Command_Info * cmd_info);

