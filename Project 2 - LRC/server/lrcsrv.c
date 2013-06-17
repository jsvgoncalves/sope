#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#include "lrcsrv.h"

void * message_thread(void * arg) {
	user_info *this_user_info = (user_info *) arg;
	// Criacao do fifo lrc_msg_cliXXX_to_srv onde esta thread vai escutar
	int lrc_msg_cliXXX_to_srv_fd;
	char pipe_name[MAX_BUF_LEN];
	snprintf(pipe_name, MAX_BUF_LEN, "/tmp/lrc_msg_cli%i_to_srv", this_user_info->pid);
	if((lrc_msg_cliXXX_to_srv_fd = open(pipe_name, O_RDWR)) == -1) {
		if(mkfifo(pipe_name, S_IRWXU) == -1) {
			perror("mkfifo: /tmp/lrc_msg_cliXXX_to_srv");
			exit(-1);
		}
		if((lrc_msg_cliXXX_to_srv_fd = open(pipe_name, O_RDWR)) == -1) {
			perror("open: /tmp/lrc_msg_cliXXX_to_srv");
			exit(-1);
		}
	}
	// Enviar resposta de aceitacao de ligacao
	if(write(this_user_info->info_pipe_fd, ACCEPT_CONNECT, strlen(ACCEPT_CONNECT)) != strlen(ACCEPT_CONNECT)) {
		perror("write: /tmp/lrc_info_srv_to_cliXXX");
		exit(-1);
	}
	// Receber mensagens e processalas
	char message[MAX_MSG_LEN];
	int message_length;
	while(1) {
		if((message_length = read(lrc_msg_cliXXX_to_srv_fd, message, MAX_MSG_LEN)) == -1) {
			perror("read: /tmp/lrc_msg_cliXXX_to_srv");
			exit(-1);
		}
		if(message_length != 0) {
		    // Estatisticas simples
            this_user_info->messages_sent += 1;
		    // Verificar as horas
            char time_string[9];
            get_time(time_string, 9);
		    // Log das mensagens
		    char log_buffer[MAX_BUF_LEN];
		    if(this_user_info->mode == BROADCAST) {
                snprintf(log_buffer, MAX_BUF_LEN, "[%s] (%s) %s \t[broadcast]\n", time_string, this_user_info->nick, message);
		    }
		    else {
		        group_info * this_group_info = get_group_info_by_transmission(this_user_info->transmission);
		        if(this_user_info->mode == MULTICAST) {
                    snprintf(log_buffer, MAX_BUF_LEN, "[%s] (%s) %s \t[multicast:%s]\n", time_string, this_user_info->nick, message, this_group_info->name);
                }
                else if(this_user_info->mode == UNICAST) {
                    snprintf(log_buffer, MAX_BUF_LEN, "[%s] (%s) %s \t[unicast:%s]\n", time_string, this_user_info->nick, message, this_group_info->name);
                }
		    }
		    // TODO Logs
		    printf("%s", log_buffer);
		    if(logging) {
                if(write(logfile_fd, log_buffer, strlen(log_buffer)) == -1) {
                    perror("write: logfile_fd");
                    exit(-1);
                }
            }
			// Criacao da mensagem de resposta para o mesmo cliente
			char formated_message[MAX_MSG_LEN];
			snprintf(formated_message, MAX_MSG_LEN, "[%s] (%s) %s", time_string, this_user_info->nick, message);
			// Envio para todos os clientes no mesmo modo
			write_msg_to_transmission(this_user_info, formated_message);
			// E para o proprio user (sem estatistica de msg recebida)
		    if(write(this_user_info->info_pipe_fd, formated_message, strlen(formated_message)) != strlen(formated_message)) {
                perror("write: /tmp/lrc_info_srv_to_cliXXX");
                exit(-1);
            }
		}
        // Limpar buffer de mensagens
		bzero(message, MAX_MSG_LEN);
	}
	pthread_exit(NULL);
}

////////////////////////////////////////////////////////////////////////////////

void * command_thread(void *arg) {
	// Criacao do fifo lrc_cmd_cli_to_srv
	int lrc_cmd_cli_to_srv_fd;
	if((lrc_cmd_cli_to_srv_fd = open("/tmp/lrc_cmd_cli_to_srv", O_RDWR)) == -1) {
		if(mkfifo("/tmp/lrc_cmd_cli_to_srv", S_IRWXU) == -1) {
			perror("mkfifo: /tmp/lrc_cmd_cli_to_srv");
			exit(-1);
		}
		if((lrc_cmd_cli_to_srv_fd = open("/tmp/lrc_cmd_cli_to_srv", O_RDWR)) == -1) {
			perror("open: /tmp/cmd_cli_to_srv");
			exit(-1);
		}
	}
	// Esperar por ligacoes dos clientes
	char request_with_pid[MAX_MSG_LEN], request[MAX_MSG_LEN];
	int request_length;
	int pid;
	user_info *this_user_info;
	while(1) {
		if((request_length = read(lrc_cmd_cli_to_srv_fd, request_with_pid, MAX_MSG_LEN)) == -1) {
			perror("read: /tmp/lrc_cmd_cli_to_srv");
			exit(-1);
		}
		if(request_length != 0) {
            // Retirar o pid dos pedidos para identificar o cliente
            sscanf(request_with_pid, "%i %[a-zA-Z0-9+*/- ]s", &pid, request);
            this_user_info = get_user_info_by_pid(pid);

            // Verificar as horas
            char time_string[TIME_LEN];
            get_time(time_string, TIME_LEN);
	        // Verificar se temos este utilizador no sistema
            if(this_user_info == NULL) {
                // Log das mensagens
                char log_buffer[MAX_BUF_LEN];
                snprintf(log_buffer, MAX_BUF_LEN, "[%s] %s\t[%i]\n", time_string, request, pid);
                printf("%s", log_buffer);
                if(logging) {
                    if(write(logfile_fd, log_buffer, strlen(log_buffer)) == -1) {
                        perror("write: logfile_fd");
                        exit(-1);
                    }
                }
                if(strncmp(CONNECT, request, strlen(CONNECT)) == 0) {
                    // SERVER STATS
                    server_connection_request++;

                    char connect[MAX_BUF_LEN];
                    char nick[MAX_BUF_LEN];
                    char name[MAX_BUF_LEN];
                    char email[MAX_BUF_LEN];
                    if(sscanf(request, "%s %s %s %s", connect, nick, name, email) == 4) {
                        user_info *user_info_by_nick = get_user_info_by_nick(nick);
                        user_info *user_info_by_pid = get_user_info_by_pid(pid);
                        // So adicionamos utilizadores se o nick nao existe na base de dados e nao e um usuario ja existente
                        if(user_info_by_nick == NULL && user_info_by_pid == NULL) {
                            this_user_info = add_user_info(pid, nick, name, email);
                            //SERVER STATS
                            server_user_counter++;
                            server_user_counter_total++;
                            pthread_t message_tid;
                            if(pthread_create(&message_tid, NULL, message_thread, (void *) this_user_info) != 0) {
                                printf("pthread_create: message_thread\n");
                                exit(-1);
                            }

                            // Avisar os outros utilizadores que entrou um cliente no server
                            char formated_message[MAX_MSG_LEN];
                            snprintf(formated_message, MAX_MSG_LEN, NICK_JOINED_SERVER, this_user_info->nick);
                            write_info_to_transmission(this_user_info, formated_message);
                        }
                        // Se o utilizador já existir construimos uma mensagem e avisamos o cliente
                        else if(user_info_by_nick == NULL) {
                           // write_info_to_user(this_user_info, REFUSE_CONNECT_U);
                        }
                    }
                }
            }
            else {
                // Log das mensagens
                char log_buffer[MAX_BUF_LEN];
                snprintf(log_buffer, MAX_BUF_LEN, "[%s] %s\t[%i:%s]\n", time_string, request, pid, this_user_info->nick);
                printf("%s", log_buffer);
                if(logging) {
                    if(write(logfile_fd, log_buffer, strlen(log_buffer)) == -1) {
                        perror("write: logfile_fd");
                        exit(-1);
                    }
                }
                if(strncmp(CONNECT, request, strlen(CONNECT)) == 0) {
                    write_info_to_user(this_user_info, REFUSE_CONNECTED);
                }
                else if(strncmp(DISCONNECT, request, strlen(DISCONNECT)) == 0 && strlen(request) == strlen(DISCONNECT)) {
                    // Avisar os outros utilizadores que saiu um um cliente no server
                    char formated_message[MAX_MSG_LEN];
                    snprintf(formated_message, MAX_MSG_LEN, NICK_LEFT_SERVER, this_user_info->nick);
                    write_info_to_transmission(this_user_info, formated_message);
                    write_info_to_user(this_user_info, SELF_DISCONNECT);
                    remove_user_by_pid(pid);
                    server_user_counter--;
                }
                else if(strncmp(CREATE, request, strlen(CREATE)) == 0){
                    char create[MAX_BUF_LEN];
                    char group_name[MAX_BUF_LEN];
                    if(sscanf(request, "%s %s", create, group_name) == 2) {
                        group_info *new_group_info = add_group_info(group_name);
                        this_user_info->mode = MULTICAST;
                        this_user_info->transmission = new_group_info->transmission;
                        char response[MAX_MSG_LEN];
                        snprintf(response, MAX_MSG_LEN, ACCEPT_CREATE, new_group_info->name);
                        write_info_to_user(this_user_info, response);
                    }
                }
                else if(strncmp(JOIN, request, strlen(JOIN)) == 0){
                    char join[MAX_BUF_LEN];
                    char group_name[MAX_BUF_LEN];
                    if(sscanf(request, "%s %s", join, group_name) == 2) {
                        group_info *temp_group_info = get_group_info_by_name(group_name);
                        if(temp_group_info != NULL) {
                            this_user_info->mode = MULTICAST;
                            this_user_info->transmission = temp_group_info->transmission;
                            char response[MAX_MSG_LEN];
                            snprintf(response, MAX_MSG_LEN, ACCEPT_JOIN, temp_group_info->name);
                            write_info_to_user(this_user_info, response);
                            // Avisar os outros utilizadores do mesmo grupo que este utilizador entrou
                            char formated_message[MAX_MSG_LEN];
                            snprintf(formated_message, MAX_MSG_LEN, NICK_JOINED_GROUP, this_user_info->nick);
                            write_info_to_transmission(this_user_info, formated_message);

                        }
                    }
                }
                else if(strncmp(LEAVE, request, strlen(LEAVE)) == 0) { // TODO VERIFICAR S E' LEAVE GROUP OU CHAT?
                    // Avisar os outros utilizadores do mesmo grupo que este utilizador vai sair do grupo
                    char formated_message[MAX_MSG_LEN];
                    snprintf(formated_message, MAX_MSG_LEN, NICK_LEFT_GROUP, this_user_info->nick);
                    write_info_to_transmission(this_user_info, formated_message);
                    // Avisar o utilizador que vai sair do grupo
                    group_info *temp_group_info = get_group_info_by_transmission(this_user_info->transmission);
                    char response[MAX_MSG_LEN];
                    snprintf(response, MAX_MSG_LEN, ACCEPT_LEAVE, temp_group_info->name);
                    write_info_to_user(this_user_info, response);
                    // Mudar o modo do utilizador
                    this_user_info->mode = BROADCAST;
                    this_user_info->transmission = -1;
                }
                else if(strncmp(CHAT, request, strlen(CHAT)) == 0){
                    char chat[MAX_BUF_LEN];
                    char nick[MAX_BUF_LEN];
                    if(sscanf(request, "%s %s", chat, nick) == 2) {
                        user_info *other_user_info = get_user_info_by_nick(nick);
                        if(other_user_info != NULL) {
                            // Vamos negar a ligacao se o outro utilizador nao estiver em BROADCAST
                            if(other_user_info->mode != BROADCAST) {
                                char response[MAX_MSG_LEN];
                                snprintf(response, MAX_MSG_LEN, REFUSE_CHAT, other_user_info->nick);
                                write_info_to_user(this_user_info, REFUSE_CHAT);
                            }
                            else {
                                // Criacao do grupo
                                char group_name[MAX_BUF_LEN];
                                snprintf(group_name, MAX_BUF_LEN, "%s-%s", this_user_info->nick, other_user_info->nick);
                                group_info * new_group_info = add_group_info(group_name);
                                this_user_info->mode = UNICAST;
                                this_user_info->transmission = new_group_info->transmission;
                                other_user_info->mode = UNICAST;
                                other_user_info->transmission = new_group_info->transmission;
                                // Assinalar os utilizadores que fazem parte de um chat
                                char response[MAX_MSG_LEN];
                                snprintf(response, MAX_MSG_LEN, ACCEPT_CHAT, new_group_info->name);
                                write_info_to_user(this_user_info, response);
                                if(write(other_user_info->info_pipe_fd, response, strlen(response)) != strlen(response)) { // ESTE UTILIZADOR N ENVIOU O COMANDO;
                                    perror("write: /tmp/lrc_info_srv_to_cliXXX");                                          // LOGO NAO USA O WRITE_INFO
                                    exit(-1);                                                                              // PRA N ADICIONAR NAS ESTATISTICAS
                                }
                            }
                        }
                        else { // Se nao reconhecer o nick
                            char response[MAX_MSG_LEN];
                            snprintf(response, MAX_MSG_LEN, NICK_UNKNOWN);
                            write_info_to_user(this_user_info, response);
                        }
                    }
                }
                else if(strncmp(LIST, request, strlen(LIST)) == 0) {
                    if(strncmp(LIST_USERS, request, strlen(LIST_USERS)) == 0) {
                        char users[MAX_BUF_LEN];
                        list_users(users, MAX_BUF_LEN);
                        write_info_to_user(this_user_info, users);
                    }
                    else if(strncmp(LIST_GROUPS, request, strlen(LIST_GROUPS)) == 0) {
                        char groups[MAX_BUF_LEN];
                        list_groups(groups, MAX_BUF_LEN);
                        write_info_to_user(this_user_info, groups);
                    }
                }
                else if(strncmp(NICK, request, strlen(NICK)) == 0){
                    char nick_command[MAX_BUF_LEN];
                    char nick[MAX_BUF_LEN];
                    char response[MAX_MSG_LEN];
                    if(sscanf(request, "%s %s", nick_command, nick) == 2) {
                        // Verificacao se ja existe um utilizador com o mesmo nick
                        user_info *other_user_info = get_user_info_by_nick(nick);
                        if(other_user_info == NULL) {
                            this_user_info->nick = strdup(nick);
                            char response[MAX_MSG_LEN];
                            snprintf(response, MAX_MSG_LEN, NICK_CHANGED, this_user_info->nick);
                            write_info_to_user(this_user_info, response);
                        }
                        else {
                            this_user_info = get_user_info_by_pid(pid);
                            snprintf(response, MAX_MSG_LEN, NICK_EXISTS);
                            write_info_to_user(this_user_info, NICK_EXISTS);
                        }
                    }
                    else if(strncmp(request, NICK, strlen(NICK)) == 0) {
                        char response[MAX_MSG_LEN];
                        snprintf(response, MAX_MSG_LEN, NICK_IS, this_user_info->nick);
                        write_info_to_user(this_user_info, response);
                    }
                }
                else if(strncmp(WHOIS, request, strlen(WHOIS)) == 0 ){
                    char whois[MAX_BUF_LEN];
                    char nick[MAX_BUF_LEN];
                    if(sscanf(request, "%s %s", whois, nick) == 2) {
                        user_info *other_user_info = get_user_info_by_nick(nick);
                        if(other_user_info != NULL) {
                            char response[MAX_MSG_LEN];
                            snprintf(response, MAX_MSG_LEN, NICK_WHOIS, other_user_info->nick, other_user_info->name, other_user_info->email);
                            write_info_to_user(this_user_info, response);
                        }
                        else {
                            write_info_to_user(this_user_info, NICK_UNKNOWN);
                        }
                    }
                }
                else if(strncmp(SILENT, request, strlen(SILENT)) == 0 && strlen(request) == strlen(SILENT)) {
                    this_user_info = get_user_info_by_pid(pid);
                    if(this_user_info->silent == SILENT_OFF) {
                        this_user_info->silent = SILENT_ON;
                        write_info_to_user(this_user_info, SILENT_IS_ON);
                    }
                    else if(this_user_info->silent == SILENT_ON) {
                        this_user_info->silent = SILENT_OFF;
                        write_info_to_user(this_user_info, SILENT_IS_OFF);
                    }
                }
                else if(strncmp(MEMBERS, request, strlen(MEMBERS)) == 0 && strlen(request) == strlen(MEMBERS)) {
                    this_user_info = get_user_info_by_pid(pid);
                    char members[MAX_BUF_LEN];
                    list_members(this_user_info, members, MAX_BUF_LEN);
                    write_info_to_user(this_user_info, members);
                }
                else if(strncmp(STATS, request, strlen(STATS)) == 0 && strlen(request) == strlen(STATS)) {
                    char response[MAX_MSG_LEN];
                    list_server_stats(response, MAX_MSG_LEN);
                    write_info_to_user(this_user_info, response);
                }
                else if(strncmp(MYSTATS, request, strlen(MYSTATS)) == 0 && strlen(request) == strlen(MYSTATS)) {
                    char response[MAX_MSG_LEN];
                    list_user_stats(this_user_info, response, MAX_MSG_LEN);
                    write_info_to_user(this_user_info, response);
                }
                else if(strncmp(RT, request, strlen(RT)) == 0) {
                    if(strncmp(RT_ON, request, strlen(RT_ON)) == 0) {
                        this_user_info->notify = NOTIFY_ON;
                        write_info_to_user(this_user_info, RT_IS_ON);
                    }
                    else if(strncmp(RT_OFF, request, strlen(RT_OFF)) == 0) {
                        this_user_info->notify = NOTIFY_OFF;
                        write_info_to_user(this_user_info, RT_IS_OFF);
                    }
                }
            }
		}
		bzero(request, MAX_MSG_LEN);
		bzero(request_with_pid, MAX_MSG_LEN);
	}
	pthread_exit(NULL);
}

////////////////////////////////////////////////////////////////////////////////

void * statistics_thread(void * arg) {

    user_info *other_user_info = first_user_info;
    char info[MAX_MSG_LEN];

    while(1) {
        sleep(RT_TIME);
        char time_string[TIME_LEN];
        get_time(time_string, TIME_LEN);
        list_server_stats(info, MAX_MSG_LEN);
        int rt_size;
        rt_size=strlen(info);
        //Enviar as estatisticas p/ users com RT ON
        while(other_user_info != NULL) {
            if(other_user_info->notify == NOTIFY_ON) {
                if(write(other_user_info->info_pipe_fd, info, strlen(info)) != strlen(info)) { // ESTE UTILIZADOR N ENVIOU COMANDO;
                    perror("write: /tmp/lrc_info_srv_to_cliXXX");                             // LOGO NAO USA O WRITE_INFO
                    exit(-1);                                                                // PRA N ADICIONAR NAS ESTATISTICAS
                }
                other_user_info->bytes += rt_size;
            }
            other_user_info = other_user_info->next;
        }

         // Log das mensagens
        char log_buffer[MAX_BUF_LEN];
        snprintf(log_buffer, MAX_BUF_LEN, "[%s] (server) STATISTICS SENT (RT)\t[broadcast]\n", time_string);
        printf("%s", log_buffer);
        if(logging) {
            if(write(logfile_fd, log_buffer, strlen(log_buffer)) == -1) {
                perror("write: logfile_fd");
                exit(-1);
            }
        }
        other_user_info = first_user_info;
    }

}

int main(int argc, char *argv[]) {
	if(argc != 1 && argc != 3 && argc != 4) {
		printf("Usage: lrc-server [-b] [-l logfile]\n");
		printf("\t[-b] option implies [-l] option exists\n");
		return -1;
	}
	// Inicializacao de variaveis globais
	background = 0;
	logging = 0;
	first_user_info = NULL;
	first_group_info = NULL;
	transmission_counter = 0;
	char * logfile;
	int i;
	for(i = 1; i < argc; i++) {
		if(strcmp(argv[i], "-b") == 0) {
			background = 1;
		}
		else if(strcmp(argv[i], "-l") == 0) {
			logging = 1;
			logfile = argv[i + 1];
		}
	}
	// Verificar que nao temos modo em background sem log
	if(background && !logging) {
		printf("\t[-b] option implies [-l] option exists\n");
		return -1;
	}
	// Abrir ficheiro de log
	if(logging) {
		if((logfile_fd = open(logfile, O_WRONLY | O_CREAT | O_APPEND | O_SYNC, S_IRUSR | S_IWUSR)) == -1) {
			perror("open: logfile");
			return -1;
		}
	}
	// Redirecionar output para /dev/null se necessario
	if(background) {
	    int null_fd;
		if((null_fd = open("/dev/null", O_WRONLY, NULL)) == -1) {
			perror("open: /dev/null");
			return -1;
		}
		dup2(null_fd, STDOUT_FILENO);
	}
	// Inicializacao das primeiras estruturas de informacao
	first_user_info = NULL;
	first_group_info = NULL;
	server_messages = 0;
    server_commands_received = 0;
    server_connection_request = 0;
    server_user_counter = 0;
    server_user_counter_total = 0;
    server_bytes = 0;
    time(&server_start_time);

	// Criar a thread responsavel por receber as ligacoes
	pthread_t command_tid;
	if(pthread_create(&command_tid, NULL, command_thread, NULL) != 0) {
		printf("pthread_create: command_thread\n");
		return -1;
	}
	// Criar a thread responsavel enviar estatisticas RT
	pthread_t statistics_tid;
	if(pthread_create(&statistics_tid, NULL, statistics_thread, NULL) != 0) {
		printf("pthread_create: statistics_thread\n");
		return -1;
	}
	// Deixar os outros threads a correr
	int ret_val = 0;
	pthread_exit(&ret_val);
}
