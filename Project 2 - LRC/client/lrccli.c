#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <curses.h>
#include <semaphore.h>


#include "lrccli.h"

#define MAX_MSG_LEN 1024
#define MAX_BUF_LEN	2048

#define SLASH           "/"
#define CONNECT         "/CONNECT"
#define DISCONNECT      "/DISCONNECT"
#define HELP 			"/HELP"
#define EXIT            "/EXIT"

#define ACCEPT_CONNECT      " * Connection accepted!"
#define NICK_CHANGED        " * Your nick changed \"%[^\"!]s\"!"

#define START_MESSAGE	" To list the available commands use /HELP\n"
#define HELP_MENU		" Help:\n\t/CONNECT\t\t- to connect to the server\n\t/DISCONNECT\t\t- to disconnect from the server\n\t/EXIT\t\t\t- to close the client\n\t/WHOIS nick\t\t- to check information about that user\n\t/NICK [nick]\t\t- to check and modify your nick\n\t/SILENT\t\t\t- to toggle between normal and silent mode\n\t/CREATE group\t\t- to create a talk group\n\t/LIST USERS|GROUPS\t- to list the users or groups connected\n\t/JOIN group\t\t- to join a talk group\n\t/MEMBERS\t\t- to list the members of the current group\n\t/CHAT nick\t\t- to chat with the user with that nickname\n\t/LEAVE\t\t\t- to leave a talk group\n\t/MYSTATS\t- to check your stats\n\t/STATS\t\t- to check server stats\n\t/RT ON|OFF\t\t- to receive periodical statistics\n\t/HELP\t\t\t- to list this help!"

////////////////////////////////////////////////////////////////////////////////

// Variaveis Globais
char *nick, *name, *email;
WINDOW *title_sw;
WINDOW *display_sw;
WINDOW *delimiter_sw;
WINDOW *keyboard_sw;
int pid;
int lrc_msg_cliXXX_to_srv_fd;
bool connected;
// Semaforos
sem_t ncurses_mutex;

////////////////////////////////////////////////////////////////////////////////

void update_nick() {
	wclear(delimiter_sw);
	wmove(delimiter_sw, 0, 1);
	wprintw(delimiter_sw, "[%s]", nick);
	wrefresh(delimiter_sw);
}

////////////////////////////////////////////////////////////////////////////////

void * display_messages_thread(void *arg) {
	// Criacao do fifo lrc_msg_srv_to_cliXXX
	int lrc_msg_srv_to_cliXXX_fd;
	char buffer[MAX_BUF_LEN];
	snprintf(buffer, MAX_BUF_LEN, "/tmp/lrc_msg_srv_to_cli%i", pid);
	if((lrc_msg_srv_to_cliXXX_fd = open(buffer, O_RDWR)) == -1) {
		if(mkfifo(buffer, S_IRWXU) == -1) {
		    endwin();
			perror("mkfifo: /tmp/lrc_msg_srv_to_cliXXX");
			exit(-1);
		}
		if((lrc_msg_srv_to_cliXXX_fd = open(buffer, O_RDWR)) == -1) {
		    endwin();
			perror("open: /tmp/lrc_msg_srv_to_cliXXX");
			exit(-1);
		}
	}
	// Receber mensagens do servidor
	char message[MAX_MSG_LEN];
	int message_length;
	while(true) {
		if((message_length = read(lrc_msg_srv_to_cliXXX_fd, message, MAX_MSG_LEN)) == -1) {
			endwin();
			perror("read: /tmp/lrc_msg_srv_to_cliXXX");
			exit(-1);
		}
		if(message_length != 0) {
			sem_wait(&ncurses_mutex);
            wprintw(display_sw, "%s\n", message);
            wrefresh(display_sw);
			sem_post(&ncurses_mutex);
		}
		bzero(message, MAX_MSG_LEN);
	}
	pthread_exit(NULL);
}

void * display_information_thread(void *arg) {
	// Criacao do fifo lrc_info_srv_to_cliXXX
	int lrc_info_srv_to_cliXXX_fd;
	char buffer[MAX_BUF_LEN];
	snprintf(buffer, MAX_BUF_LEN, "/tmp/lrc_info_srv_to_cli%i", pid);
    if((lrc_info_srv_to_cliXXX_fd = open(buffer, O_RDWR)) == -1) {
		if(mkfifo(buffer, S_IRWXU) == -1) {
		    endwin();
			perror("mkfifo: /tmp/lrc_info_srv_to_cliXXX");
			exit(-1);
		}
		if((lrc_info_srv_to_cliXXX_fd = open(buffer, O_RDWR)) == -1) {
		    endwin();
			perror("open: /tmp/lrc_info_srv_to_cliXXX");
			exit(-1);
		}
	}
	// Receber informacao do servidor
	char information[MAX_MSG_LEN];
	int information_length;
	while(true) {
		if((information_length = read(lrc_info_srv_to_cliXXX_fd, information, MAX_MSG_LEN)) == -1) {
			endwin();
			perror("read: /tmp/lrc_info_srv_to_cliXXX");
			exit(-1);
		}
		if(information_length != 0) {
			// Recebemos uma aceitacao de ligacao e avisamos a display_message_thread
			if(strncmp(ACCEPT_CONNECT, information, strlen(ACCEPT_CONNECT)) == 0) {
				char message_pipe_name[MAX_BUF_LEN];
				snprintf(message_pipe_name, MAX_BUF_LEN, "/tmp/lrc_msg_cli%i_to_srv", pid);
				if((lrc_msg_cliXXX_to_srv_fd = open(message_pipe_name, O_RDWR)) == -1) {
					endwin();
					perror("open: /tmp/lrc_msg_cliXXX_to_srv");
					exit(-1);
				}
				connected = true;
			}
			else if(strncmp(NICK_CHANGED, information, 20) == 0) {
			    char new_nick[MAX_BUF_LEN];
                sscanf(information, NICK_CHANGED, new_nick);
                nick = strdup(new_nick);
                update_nick();
			}
            else {
                sem_wait(&ncurses_mutex);
                wprintw(display_sw, "%s\n", information);
                wrefresh(display_sw);
                sem_post(&ncurses_mutex);
            }
		}
		bzero(information, MAX_MSG_LEN);
	}
	pthread_exit(NULL);
}

void * keyboard_thread(void *arg) {
    // Abertura do fifo lrc_cmd_cli_to_srv
    int lrc_cmd_cli_to_srv_fd;
    if((lrc_cmd_cli_to_srv_fd = open("/tmp/lrc_cmd_cli_to_srv", O_RDWR)) == -1) {
        endwin();
        perror("open: /tmp/lrc_cmd_cli_to_srv");
        printf("LRC server is not up!\n");
        exit(-1);
    }
	// Definicao do estado de ligacao
    connected = false;
    // Receber messagens a enviar do teclado
    bool leave = false;
	char buffer[MAX_MSG_LEN];
	while(!leave) {
        wgetstr(keyboard_sw, buffer);
        // E um outro comando ou uma mensagem?
        if(strncmp(SLASH, buffer, strlen(SLASH)) == 0) {
             // Menu de ajuda
            if(strncmp(HELP, buffer, strlen(HELP)) == 0 && strlen(buffer) == strlen(HELP)) {
                sem_wait(&ncurses_mutex);
                wprintw(display_sw, "%s\n", HELP_MENU);
                wrefresh(display_sw);
                sem_post(&ncurses_mutex);
            }
            else if(!connected) {
                if(strncmp(CONNECT, buffer, strlen(CONNECT)) == 0 && strlen(buffer) == strlen(CONNECT)) {
                    char connect_information[MAX_MSG_LEN];
                    snprintf(connect_information, MAX_BUF_LEN, "%i %s %s %s %s", pid, CONNECT, nick, name, email);
                    if(write(lrc_cmd_cli_to_srv_fd, connect_information, strlen(connect_information)) != strlen(connect_information)) {
                        endwin();
                        perror("write: /tmp/lrc_cmd_cli_to_srv");
                        exit(-1);
                    }
                }
            }
            else if(connected) {
                if(strncmp(DISCONNECT, buffer, strlen(DISCONNECT)) == 0 && strlen(buffer) == strlen(DISCONNECT)) {
                    connected = false;
                }
                else if(strncmp(EXIT, buffer, strlen(EXIT)) == 0 && strlen(buffer) == strlen(EXIT)) {
                    connected = false;
                    leave = true;
                    bzero(buffer, MAX_MSG_LEN);  // Substituir o buffer por um disconnect
                    snprintf(buffer, MAX_BUF_LEN, "%s", DISCONNECT);
                }
                // Enviar comando
                char buffer_with_pid[MAX_MSG_LEN];
                snprintf(buffer_with_pid, MAX_BUF_LEN, "%i %s", pid, buffer);
                if(write(lrc_cmd_cli_to_srv_fd, buffer_with_pid, strlen(buffer_with_pid)) != strlen(buffer_with_pid)) {
                    endwin();
                    perror("write: /tmp/lrc_cmd_cli_to_srv");
                    exit(-1);
                }
                // Se o comando nao e valido ignoramos
            }
        }
        else {
            // Enviar mensagens
			if(connected) {
		        if(write(lrc_msg_cliXXX_to_srv_fd, buffer, strlen(buffer)) != strlen(buffer)) {
		            endwin();
		            perror("write: /tmp/lrc_msg_cliXXX_to_srv");
		            exit(-1);
		        }
		    }
        }
        sem_wait(&ncurses_mutex);
        wclear(keyboard_sw);
        wrefresh(keyboard_sw);
		bzero(buffer, MAX_MSG_LEN);
		sem_post(&ncurses_mutex);
	}
    // Sair da thread
    pthread_exit(NULL);
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[]) {
	if(argc != 1 && argc != 3 && argc != 5 && argc != 7) {
		printf("Usage: [-u nickname][-n name][-e email]\n");
		return -1;
	}
	nick = NULL;
	name = NULL;
	email = NULL;
	int i;
	for(i = 1; i < argc; i++) {
		if(strcmp(argv[i], "-u") == 0) nick = argv[i + 1];
		else if(strcmp(argv[i], "-n") == 0) name = argv[i + 1];
		else if(strcmp(argv[i], "-e") == 0) email = argv[i + 1];
	}
    // Determinar a pid do processo
	pid = getpid();
	// Determinar o tamanho do terminal
    struct winsize ws;
    if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1) {
		perror("ioctl: TIOCGWINSZ");
		return -1;
	}
	int lines = ws.ws_row;
	int columns = ws.ws_col;
	// Inicializar ncurses 'stdscr' e 'curscr'
	initscr();
	// Iniciar cores
	start_color();
	init_pair(1, COLOR_YELLOW, COLOR_BLUE);
	init_pair(2, COLOR_WHITE, 0);
	bkgd(COLOR_PAIR(2));
	// Outras configuracoes
	clear();		// Apaga o ecra
	echo();			// Permite echo do teclado
	refresh();		// Actualiza a informacao da janela base
	// Criar janelas sobre a janela 'stdscr'
	title_sw = subwin(stdscr, 1, columns, 0, 0);
	display_sw = subwin(stdscr, lines-3, columns, 1, 0);
	delimiter_sw = subwin(stdscr, 1, columns, lines-2, 0);
	keyboard_sw = subwin(stdscr, 1, columns, lines-1, 0);
	// Titulo da janela
	wbkgd(title_sw, COLOR_PAIR(1) | A_BOLD);
	wmove(title_sw, 0, 5);
	wprintw(title_sw, "Local Relay Chat Client");
	wrefresh(title_sw);
	// Janela de display
	scrollok(display_sw, true);
	wrefresh(display_sw);
	// Espacador
	wbkgd(delimiter_sw, COLOR_PAIR(1) | A_BOLD);
	wmove(delimiter_sw, 0, 1);
	wrefresh(delimiter_sw);
	// Janela de escrita
	wmove(keyboard_sw, 0, 0);
	wrefresh(keyboard_sw);
	// Adicionar o nick, nome e email caso nao existam
	char buffer[MAX_BUF_LEN];
	if(nick == NULL) {
		wprintw(display_sw, " Nick?\n");
		wrefresh(display_sw);
		bzero(buffer, MAX_BUF_LEN);
		wgetstr(keyboard_sw, buffer);
     	wclear(keyboard_sw);
        wrefresh(keyboard_sw);
		wprintw(display_sw, "%s\n", buffer);
		nick = strdup(buffer);
	}
	if(name == NULL) {
		wprintw(display_sw, " Name?\n");
		wrefresh(display_sw);
		bzero(buffer, MAX_BUF_LEN);
		wgetstr(keyboard_sw, buffer);
     	wclear(keyboard_sw);
        wrefresh(keyboard_sw);
		wprintw(display_sw, "%s\n", buffer);
		name = strdup(buffer);
	}
	if(email == NULL) {
		wprintw(display_sw, " Email?\n");
		wrefresh(display_sw);
		bzero(buffer, MAX_BUF_LEN);
		wgetstr(keyboard_sw, buffer);
		wclear(keyboard_sw);
        wrefresh(keyboard_sw);
		wprintw(display_sw, "%s\n", buffer);
		email = strdup(buffer);
	}
	// Actualizar o nick
	update_nick();
	// Mensagem de ajuda
	wprintw(display_sw, START_MESSAGE);
	wrefresh(display_sw);
	// Janela de escrita
	wmove(keyboard_sw, 0, 0);
	wrefresh(keyboard_sw);
	// Semaforo
	sem_init(&ncurses_mutex, 0, 1);
	// Criar 3 threads que monitorizam o envio e recepcao de mensagens
	pthread_t display_message_tid;
	if(pthread_create(&display_message_tid, NULL, display_messages_thread, NULL) != 0) {
		endwin();
		printf("pthread_create: display_messages_thread\n");
		return -1;
	}
	pthread_t display_information_tid;
	if(pthread_create(&display_information_tid, NULL, display_information_thread, NULL) != 0) {
		endwin();
		printf("pthread_create: display_information_thread\n");
		return -1;
	}
 	pthread_t keyboard_tid;
	if(pthread_create(&keyboard_tid, NULL, keyboard_thread, NULL) != 0) {
		endwin();
		printf("pthread_create: keyboard_thread\n");
		return -1;
	}
    // Esperar pela keyboard_thread
    int *ret_val;
    if(pthread_join(keyboard_tid, (void **) &ret_val) != 0) {
		endwin();
        printf("pthread_join: keyboard_thread\n");
        return - 1;
    }
	// Terminar ncurses
	endwin();
	return 0;
}
