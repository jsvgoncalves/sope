#include "../V3/libmsh_v3.h"


/* Guarda a pid do filho para ser morto pelo
 * pai no handler dele.
 */
int child_pid;

/* Handler utilizado para o pai matar o processo
 * filho.
 */
void sig_handler_parent(int sig);

/* Executa um comando com redireccionamento
 * com a possibilidade de enviar um sinal SIGKILL
 * ao filho.
 */
pid_t exec_simple_sig(Command_Info * cmd_info);
