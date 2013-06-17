#define MAX_MSG_LEN 1024
#define MAX_BUF_LEN	2048
#define TIME_LEN       9

#define CONNECT         "/CONNECT"
#define DISCONNECT      "/DISCONNECT"
#define CREATE          "/CREATE"
#define JOIN            "/JOIN"
#define LEAVE           "/LEAVE"
#define CHAT            "/CHAT"
#define LIST			"/LIST"
#define LIST_USERS		"/LIST USERS"
#define LIST_GROUPS		"/LIST GROUPS"
#define NICK            "/NICK"
#define WHOIS           "/WHOIS"
#define SILENT          "/SILENT"
#define MEMBERS         "/MEMBERS"
#define STATS           "/STATS"
#define MYSTATS         "/MYSTATS"
#define RT              "/RT"
#define RT_ON           "/RT ON"
#define RT_OFF          "/RT OFF"

#define ACCEPT_CONNECT      " * Connection accepted!"
#define ACCEPT_JOIN         " * Joined group \"%s\"!"
#define ACCEPT_CREATE       " * Created group \"%s\"!"
#define ACCEPT_LEAVE        " * Left group \"%s\"!"
#define ACCEPT_CHAT         " * Joined chat \"%s\"!"

#define REFUSE_CHAT         " * Chat with \"%s\" was refused! User is busy!"
#define REFUSE_CONNECTED    " * You are already connected!"
#define REFUSE_CONNECT_U    " * Username is already in use!"

#define NICK_IS             " * Your nick is \"%s\"!"
#define NICK_CHANGED        " * Your nick changed \"%s\"!"
#define NICK_EXISTS         " * That nick already exists!"
#define NICK_UNKNOWN        " * That nick is unknown!"
#define NICK_WHOIS          " * The user with nick \"%s\" is \"%s\" (%s)!"

#define NICK_JOINED_GROUP   " * User \"%s\" has joined group!"
#define NICK_JOINED_SERVER  " * User \"%s\" has joined server!"
#define NICK_LEFT_GROUP     " * User \"%s\" has left group!"
#define NICK_LEFT_SERVER    " * User \"%s\" has left server!"
#define SELF_DISCONNECT     " * You're being disconnected..!"

#define SILENT_IS_ON        " * You are in silent mode now!"
#define SILENT_IS_OFF       " * You no longer are in silent mode now!"
#define RT_IS_ON            " * RT is on!"
#define RT_IS_OFF           " * RT is off!"

#define SILENT_ON   1
#define SILENT_OFF  0
#define NOTIFY_ON   1
#define NOTIFY_OFF  0

#define BROADCAST 	0
#define UNICAST 	1
#define MULTICAST	2

#define BROADCAST_TRANSMISSION -1

#define RT_TIME 60 // in secs

////////////////////////////////////////////////////////////////////////////////
// TODO
struct user_info {
	int pid;
	char *nick;
	char *name;
	char *email;
	int mode;
	int transmission;
	int silent;         // /SILENT
	int notify;         // /RT ON|OFF
	int messages_sent;
	int messages_recv;
	int commands_sent;
	int bytes;
	time_t cn_time;
    char * info_pipe;
    char * message_pipe;
	int info_pipe_fd;
	int message_pipe_fd;
	struct user_info *next;
	struct user_info *previous;
};
typedef struct user_info user_info;


struct group_info {
	char *name;
	int transmission;
	struct group_info *next;
};
typedef struct group_info group_info;

////////////////////////////////////////////////////////////////////////////////

// Variaveis globais
user_info *first_user_info;
group_info *first_group_info;
int background, logging;
int logfile_fd;
int transmission_counter;

// Estatisticas do servidor
 // TODO
int server_messages;
int server_commands_received;
int server_connection_request;
int server_user_counter;
int server_user_counter_total;
int server_bytes;
time_t server_start_time;


////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////

void get_time(char * buffer, int buffer_size) {

    time_t t = time(NULL);
    struct tm stm;
    gmtime_r(&t, &stm);
    snprintf(buffer, buffer_size, "%02i:%02i:%02i", stm.tm_hour, stm.tm_min, stm.tm_sec);
}


user_info * add_user_info(int pid, char *nick, char *name, char *email) {
	user_info *new_user_info = (user_info *) malloc(sizeof(user_info));
	new_user_info->pid = pid;
	new_user_info->nick = strdup(nick);
	new_user_info->name = strdup(name);
	new_user_info->email = strdup(email);
	new_user_info->mode = BROADCAST;
	new_user_info->transmission = BROADCAST_TRANSMISSION;
	new_user_info->silent = SILENT_OFF;
	new_user_info->notify = NOTIFY_ON;
	new_user_info->messages_sent = 0;
	new_user_info->messages_recv = 0;
	new_user_info->commands_sent = 0;
	new_user_info->bytes = 0;
	time(&new_user_info->cn_time);
	// Abertura dos dois pipes em que o servidor vai escutar o cliente
	// Estes dois pipes sao abertos para escrito sem bloqueio para o caso do cliente falhar
    char pipe_name[MAX_BUF_LEN];
    snprintf(pipe_name, MAX_BUF_LEN, "/tmp/lrc_msg_srv_to_cli%i", new_user_info->pid);
    new_user_info->message_pipe = pipe_name;
    if((new_user_info->message_pipe_fd = open(new_user_info->message_pipe, O_WRONLY | O_NONBLOCK)) == -1) {
        perror("open: /tmp/lrc_msg_srv_to_cliXXX");
        exit(-1);
    }
    bzero(pipe_name, MAX_BUF_LEN);
	snprintf(pipe_name, MAX_BUF_LEN, "/tmp/lrc_info_srv_to_cli%i", new_user_info->pid);
	new_user_info->info_pipe = pipe_name;
	if((new_user_info->info_pipe_fd = open(pipe_name, O_WRONLY | O_NONBLOCK)) == -1) {
        perror("open: /tmp/lrc_info_srv_to_cliXXX");
        exit(-1);
    }
    // Adicionar a estrutura a lista
	new_user_info->next = NULL;
	if(first_user_info == NULL) {
		first_user_info = new_user_info;
	}
	else {
		user_info *temp_user_info = first_user_info;
		while(temp_user_info->next != NULL) {
			temp_user_info = temp_user_info->next;
		}
		new_user_info->previous = temp_user_info;
		temp_user_info->next = new_user_info;
	}
	return new_user_info;
}

group_info * add_group_info(char *name) {
	group_info *new_group_info = (group_info *) malloc(sizeof(group_info));
    new_group_info->name = strdup(name);
	new_group_info->transmission = transmission_counter;
	transmission_counter++;
	if(first_group_info == NULL) {
	    first_group_info = new_group_info;
	}
	else {
	    group_info *tmp_group_info = first_group_info;
        while(tmp_group_info->next != NULL) {
            tmp_group_info = tmp_group_info->next;
        }
        tmp_group_info->next = new_group_info;
	}
	return new_group_info;
}

void print_user_info(user_info *user_info) {
	if(user_info != NULL) {
		printf("%i ", user_info->pid);
		printf("%s ", user_info->nick);
		printf("%s ", user_info->name);
		printf("%s ", user_info->email);
  		printf("%i ", user_info->mode);
		printf("%i\n", user_info->transmission);
		if(user_info->next != NULL) {
			print_user_info(user_info->next);
		}
	}
}

void print_group_info(group_info *tmp_group_info) {
	if(tmp_group_info != NULL) {
		printf("%s ", tmp_group_info->name);
		printf("%i\n", tmp_group_info->transmission);
		if(tmp_group_info->next != NULL) {
			print_group_info(tmp_group_info->next);
		}
	}
}

group_info * get_group_info_by_name(char *name) {
    group_info *tmp_group_info = first_group_info;
    while(tmp_group_info != NULL) {
        if(strcmp(tmp_group_info->name, name) == 0) {
            return tmp_group_info;
        }
        tmp_group_info = tmp_group_info->next;
    }
    return NULL;
}

group_info * get_group_info_by_transmission(int transmission) {
    group_info *tmp_group_info = first_group_info;
    while(tmp_group_info != NULL) {
        if(tmp_group_info->transmission == transmission) {
            return tmp_group_info;
        }
        tmp_group_info = tmp_group_info->next;
    }
    return NULL;
}

user_info * get_user_info_by_pid(int pid) {
    user_info *temp_user_info = first_user_info;
    while(temp_user_info != NULL) {
        if(temp_user_info->pid == pid) {
            return temp_user_info;
        }
        temp_user_info = temp_user_info->next;
    }
    return NULL;
}

user_info * get_user_info_by_nick(char *nick) {
    user_info *temp_user_info = first_user_info;
    while(temp_user_info != NULL) {
        if(strncmp(temp_user_info->nick, nick, strlen(nick)) == 0 && strlen(temp_user_info->nick) == strlen(nick)) {
            return temp_user_info;
        }
        temp_user_info = temp_user_info->next;
    }
    return NULL;
}

void remove_user_by_pid(int pid) {
    user_info *temp_user_info = first_user_info;
    user_info *previous, *next;
    while(temp_user_info != NULL) {
        if(temp_user_info->pid == pid) {
            previous = temp_user_info->previous;
            next = temp_user_info->next;
            if(previous == NULL) {
                first_user_info = previous;
            }
            else {
                previous->next = temp_user_info->next;
            }
            if(next != NULL) {
                next->previous = previous;
            }
            free(temp_user_info);
            return;
        }
        temp_user_info = temp_user_info->next;
    }
    return;
}

///////////////////////////////////////////////////////////////////////////////

void list_users(char *buffer, int buffer_size) {
	int counter;
	int size_counter = buffer_size;
	counter = snprintf(buffer, size_counter, " * Users: ");
	size_counter -= counter;
	user_info *temp_user_info = first_user_info;
	while(temp_user_info != NULL) {
		counter += snprintf(&buffer[counter], size_counter, "\"%s\" ", temp_user_info->nick);
		size_counter -= counter;
		temp_user_info = temp_user_info->next;
	}
}

void list_groups(char *buffer, int buffer_size) {
	int counter;
	int size_counter = buffer_size;
	counter = snprintf(buffer, size_counter, " * Groups: ");
	size_counter -= counter;
	group_info *tmp_group_info = first_group_info;
	while(tmp_group_info != NULL) {
	    counter += snprintf(&buffer[counter], size_counter, "\"%s\" ", tmp_group_info->name);
		size_counter -= counter;
		tmp_group_info = tmp_group_info->next;
	}
}

void list_members(user_info *this_user_info, char * buffer, int buffer_size) {
	int counter;
	int size_counter = buffer_size;
	counter = snprintf(buffer, size_counter, " * Members: ");
	size_counter -= counter;
	user_info *temp_user_info = first_user_info;
	while(temp_user_info != NULL) {
	    if(temp_user_info->transmission == this_user_info->transmission) {
            counter += snprintf(&buffer[counter], size_counter, "\"%s\" ", temp_user_info->nick);
            size_counter -= counter;
	    }
	    temp_user_info = temp_user_info->next;
	}
}

void list_server_stats(char *buffer, int buffer_size) {
    int counter;
	int size_counter = buffer_size;
    double total_time;
    time_t acttime;

    time(&acttime);
	total_time= difftime(acttime,server_start_time);

    counter = snprintf(buffer, size_counter, " * Server stats:\n");
    size_counter -= counter;

    counter += snprintf(&buffer[counter], size_counter, "\tTotal messages sent:\t%i\n", server_messages);
    size_counter -= counter;

    counter += snprintf(&buffer[counter], size_counter, "\tCommands received:\t%i\n", server_commands_received);
    size_counter -= counter;

    counter += snprintf(&buffer[counter], size_counter, "\tConnection requests:\t%i\n", server_connection_request);
    size_counter -= counter;

    counter += snprintf(&buffer[counter], size_counter, "\tTotal users (since server start):\t%i\n", server_user_counter_total);
    size_counter -= counter;

    counter += snprintf(&buffer[counter], size_counter, "\tCurrent users:\t%i\n", server_user_counter);
    size_counter -= counter;

    counter += snprintf(&buffer[counter], size_counter, "\tTotal bytes transmitted:\t%i\n", server_bytes);
    size_counter -= counter;

    snprintf(&buffer[counter], size_counter, "\tServer is up for:\t%.2lf secs\n", total_time);

}

void list_user_stats(user_info *this_user_info, char *buffer, int buffer_size) {
    int counter;
	int size_counter = buffer_size;
    double total_time;
    time_t acttime;

    time(&acttime);
	total_time= difftime(acttime,this_user_info->cn_time);

    counter = snprintf(buffer, size_counter, " * Your stats:\n");
    size_counter -= counter;
    counter += snprintf(&buffer[counter], size_counter, "\tMessages received:\t%i\n", this_user_info->messages_recv);
    size_counter -= counter;
    counter += snprintf(&buffer[counter], size_counter, "\tMessages sent:\t%i\n", this_user_info->messages_sent);
    size_counter -= counter;
    counter += snprintf(&buffer[counter], size_counter, "\tCommands sent:\t%i\n", this_user_info->commands_sent);
    size_counter -= counter;
    counter += snprintf(&buffer[counter], size_counter, "\tBytes Transmitted:\t%i\n", this_user_info->bytes);
    size_counter -= counter;
    snprintf(&buffer[counter], size_counter, "\tConnected for:\t%.2lf secs\n", total_time);
}

////////////////////////////////////////////////////////////////////////////////

void write_info_to_user(user_info *this_user_info, char info[MAX_MSG_LEN]){
    //Estatisticas
    int bytes = strlen(info);
    server_commands_received++;
    this_user_info->commands_sent++;
    server_bytes += bytes;
    this_user_info->bytes += bytes;

    if(write(this_user_info->info_pipe_fd, info, strlen(info)) != strlen(info)) {
        perror("write: /tmp/lrc_info_srv_to_cliXXX");
        exit(-1);
    }

}

void write_info_to_transmission(user_info *this_user_info, char info[MAX_MSG_LEN]) {
    user_info *other_user_info = first_user_info;

    while(other_user_info != NULL) {
        if(this_user_info != other_user_info && this_user_info->mode == other_user_info->mode && this_user_info->transmission == other_user_info->transmission && other_user_info->silent == SILENT_OFF) {
            write_info_to_user(other_user_info, info);
        }
        other_user_info = other_user_info->next;
    }
}

void write_msg_to_user(user_info *this_user_info, char msg[MAX_MSG_LEN]){
    //Estatisticas
    int bytes = strlen(msg);
    server_messages++;
    this_user_info->messages_recv++;
    server_bytes += bytes;
    this_user_info->bytes += bytes;

    if(write(this_user_info->info_pipe_fd, msg, strlen(msg)) != strlen(msg)) {
        perror("write: /tmp/lrc_info_srv_to_cliXXX");
        exit(-1);
    }
}

void write_msg_to_transmission(user_info *this_user_info, char msg[MAX_MSG_LEN]) {
    user_info *other_user_info = first_user_info;

    while(other_user_info != NULL) {
        if(this_user_info != other_user_info && this_user_info->mode == other_user_info->mode && this_user_info->transmission == other_user_info->transmission) {
            write_msg_to_user(other_user_info, msg);
        }
        other_user_info = other_user_info->next;
    }
}
////////////////////////////////////////////////////////////////////////////////
