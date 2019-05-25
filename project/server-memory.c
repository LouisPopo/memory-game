#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <pthread.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <signal.h>

#include "board_library.h"
#define MAX_PLAYER 100


int player_fd[MAX_PLAYER][2] = {
	{0,0},
	{0,0}
};

char player_color[MAX_PLAYER][15];
pthread_t players_thread[MAX_PLAYER];
int nb_connections = 0;
int nb_active_players = 0;
int board_dim;
int done = 0;


pthread_mutex_t mutex_lock;

struct thread_args{
	int * lock;
	char * message;
	int id;
	int * running_flag;
	int x1;
	int y1;
	int x2;
	int y2;
};

int GLOBAL_SOCK_FD;

void init_connections(){
	struct sockaddr_in local_addr;
	
	GLOBAL_SOCK_FD = socket(AF_INET, SOCK_STREAM, 0);
	if(GLOBAL_SOCK_FD == -1){
		perror("socket: ");
		exit(-1);
	}

	setsockopt(GLOBAL_SOCK_FD, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));

	local_addr.sin_family = AF_INET;
	local_addr.sin_port= htons(3000);
	local_addr.sin_addr.s_addr= INADDR_ANY;


	int err = bind(GLOBAL_SOCK_FD, (struct sockaddr *)&local_addr, sizeof(local_addr));
	if(err == -1) {
		perror("bind");
		exit(-1);
	}
	listen(GLOBAL_SOCK_FD, 5);
}

void connect_to_player(int player_i){
	player_fd[player_i][0] = accept(GLOBAL_SOCK_FD, NULL, NULL);	
	player_fd[player_i][1] = 1;
}

void * wait_2_seconds_return(void * arguments){
	
	int val;

	struct thread_args * args = (struct thread_args *) arguments;
	
	*(args->lock) = 1;
	size_t len = strlen(args->message);
	
	// Wait...
	sleep(2);
	
	// Return the "wrong" cards.
	
	write_to_all(args->message, len);
	
	*(args->lock) = 0;

	reset_cell_status(args->x1, args->y1);
	reset_cell_status(args->x2, args->y2);
	pthread_exit(0);
	
}

void * wait_5_sec_for_play (void * arguments){
	struct thread_args * args = (struct thread_args *) arguments;
	
	args->lock = 1;

	sleep(5);

	size_t len = strlen(args->message);
	write_to_all(args->message, len);
	reset_play(args->id, args->x1, args->y1);

	args->lock = 0;


}

void * wait_5_sec_for_play2(void * arguments){
	struct thread_args * args = (struct thread_args *) arguments;
	
	if(*(args->running_flag)){
		pthread_exit(0);
	} else {
		*(args->running_flag) = 1;
		size_t len = strlen(args->message);
		
		sleep(5);

		if(*(args->lock) == 0){
			write_to_all(args->message, len);

			reset_play(args->id, args->x1, args->y1);
			args->message[0] = '\0';
		}
		*(args->running_flag) = 0;
	}
}

// Function to send int to client
void send_int(int num, int fd){
	int32_t conv = htonl(num);
	char * data = (char*)&conv;

	write(fd, data, sizeof(conv));
}

// Receive a string (x-y) and convert the string of x and y to integers
void receive_card_coords(int player_id, int * x, int * y){
	
	char coords[10];
	read(player_fd[player_id][0], &coords, sizeof(coords));
	
	char *delim = "+";
	
	*x = atoi(strtok(coords, delim));
	*y = atoi(strtok(NULL, delim));
	
}

void update_info(char * message[], char paint_color[], char write_color[], char string_to_write[], int x, int y, int first){
	
	char x_str[2];
	char y_str[2];
	char buffer[50];
	
	sprintf(x_str, "%d", x);
	sprintf(y_str, "%d", y);
	
	if(first != 1)
		strcat(message, "=");
		
	snprintf(buffer, sizeof(buffer), "%s:%s:%s:%s:%s", paint_color, write_color, string_to_write, x_str, y_str);
	
	strcat(message, buffer);
} 

void random_color(char * color_buf){
	int r = rand() % 256;
	int g = rand() % 256;
	int b = rand() % 256;
	sprintf(color_buf, "%d-%d-%d", r,g,b);
}

void deconnect_player(int id){
	//close the socket
	char termination_message[] = "over";
	write(player_fd[id][0], &termination_message, sizeof(termination_message));
	close(player_fd[id][0]);
	printf("closing player %d socket\n", id);
	//make it inactive
	player_fd[id][1] == 0;
	nb_active_players--;
}

void write_to_all(char * message, int size){
	printf("Writing to all : %s\n", message);
	for(int i = 0; i < nb_connections; i++){
		if(player_fd[i][1] == 1){
			write(player_fd[i][0], message, size);	
		}
	}
}

void * bot_thread(void * args){
	
	int * id_pntr = args;
	int id = *id_pntr;

	char update_string[100];
	char reset_string[100];

	int board_x, board_y;
	
	while(nb_connections < 2){
		//wait...
	}

	send_int(board_dim, player_fd[id][0]);

	send_current_board_dispo(id);

	while(!done){

		receive_card_coords(id, &board_x, &board_y);
		
		if(board_x == -1 && board_y == -1){
			printf("bot wants to deconnect\n");
			deconnect_player(id);
			break;
		}

		play_response resp = board_play(board_x, board_y, id);

		memset(update_string, 0, sizeof(update_string));
		memset(reset_string, 0, sizeof(reset_string));

		int resp_code = resp.code;
		
		if(resp_code == 1){ //first pick
			update_info(&update_string, player_color[id], "200-200-200", resp.str_play1, resp.play1[0], resp.play1[1], 1);
			write_to_all(&update_string, sizeof(update_string));

		} else if (resp_code == 2){ // is good
			
			update_info(&update_string, player_color[id], "0-0-0", resp.str_play1, resp.play1[0], resp.play1[1], 1);
			update_info(&update_string, player_color[id], "0-0-0", resp.str_play2, resp.play2[0], resp.play2[1], 0);
			write_to_all(&update_string, sizeof(update_string));

		} else if (resp_code == -2){ //is no good

			update_info(&update_string, player_color[id], "255-0-0", resp.str_play1, resp.play1[0], resp.play1[1], 1);
			update_info(&update_string, player_color[id], "255-0-0", resp.str_play2, resp.play2[0], resp.play2[1], 0);
			write_to_all(&update_string, sizeof(update_string));

			sleep(2);

			memset(update_string, 0, sizeof(update_string));

			update_info(&update_string, "255-255-255", "255-255-255", " ", resp.play1[0], resp.play1[1], 1);
			update_info(&update_string, "255-255-255", "255-255-255", " ", resp.play2[0], resp.play2[1], 0);

			reset_cell_status(resp.play1[0], resp.play1[1]);
			reset_cell_status(resp.play2[0], resp.play2[1]);

			write_to_all(&update_string, sizeof(update_string));
		} else if (resp_code == 0) {// is is filled 
			printf("FILLED!\n");
		}
		
	}
	
}

void send_current_board_dispo(int id){
	struct cell_info infos;
	char cell_str_status[100];

	for(int i = 0; i < board_dim; i++){
		for(int j = 0; j < board_dim; j++){

			get_cell_status(&infos,i, j);

			if(infos.player_id != -1){

				int player_id = infos.player_id;
			
				char i_str[2];
				char j_str[2];
				sprintf(i_str, "%d", i);
				sprintf(j_str, "%d", j);
				
				sprintf(cell_str_status, "%s:%s:%s:%s:%s", player_color[player_id], infos.string_color, infos.str, i_str, j_str);

				write(player_fd[id][0], &cell_str_status, sizeof(cell_str_status));

				memset(cell_str_status, 0, sizeof(cell_str_status));
			}
		}
	}
	char end[3] = "***";
	write(player_fd[id][0], &end, sizeof(end));
}

void * player_main(void * args){
	
	int * id_pntr = args;
	int id = *id_pntr;



	while(nb_connections < 2){
		//wait...
	}

	send_int(board_dim, player_fd[id][0]);

	int board_x, board_y;
	char update_string[100];
	char reset_string[100];

	done = 0;

	struct thread_args * args_wait2 = malloc(sizeof(struct thread_args));
	int locker = 0;
	args_wait2->lock = &locker;

	struct thread_args *args_5sec = malloc(sizeof(struct thread_args));
	int fsec_flag = 0;
	int running_flag = 0;
	args_5sec->lock = &fsec_flag;
	args_5sec->id = id;
	args_5sec->running_flag = &running_flag;
	

	pthread_t five_sec_wait, two_sec_wait;

	send_current_board_dispo(id);

	while(!done) {

		receive_card_coords(id, &board_x, &board_y);
		printf("Received (%d,%d) - %d\n", board_x, board_y, id);
		
		if(board_x == -1 && board_y == -1){
			deconnect_player(id);
			break;
		}

		//printf("locker = %d\n", locker);

		if(nb_active_players >= 2) {
			
			if(locker != 1){

				play_response resp = board_play(board_x, board_y, id);
			
				memset(update_string, 0, sizeof(update_string));
				memset(reset_string, 0, sizeof(reset_string));

				int resp_code = resp.code;
				printf("resp coce = %d\n", resp.code);
				if(resp_code == 1){
					
					update_info(&update_string, player_color[id], "200-200-200", resp.str_play1, resp.play1[0], resp.play1[1], 1);
					write_to_all(&update_string, sizeof(update_string));

					update_info(&reset_string, "255-255-255", "255-255-255", " ", resp.play1[0], resp.play1[1], 1);
					
					args_5sec->message = reset_string;
					args_5sec->x1 = board_x;
					args_5sec->y1 = board_y;
					
					pthread_create(&five_sec_wait, NULL, wait_5_sec_for_play, args_5sec);
					

				} else if (resp_code == 2){

					if(args_5sec->lock == 1){
						pthread_cancel(five_sec_wait);
					}
					
					//fsec_flag = 1;

					update_info(&update_string, player_color[id], "0-0-0", resp.str_play1, resp.play1[0], resp.play1[1], 1);
					update_info(&update_string, player_color[id], "0-0-0", resp.str_play2, resp.play2[0], resp.play2[1], 0);
					write_to_all(&update_string, sizeof(update_string));

				} else if (resp_code == -2){

					if(args_5sec->lock == 1){
						pthread_cancel(five_sec_wait);
					}

					//fsec_flag = 1;

					update_info(&update_string, player_color[id], "255-0-0", resp.str_play1, resp.play1[0], resp.play1[1], 1);
					update_info(&update_string, player_color[id], "255-0-0", resp.str_play2, resp.play2[0], resp.play2[1], 0);
					write_to_all(&update_string, sizeof(update_string));

					update_string[0] = '\0';

					update_info(&update_string, "255-255-255", "255-255-255", " ", resp.play1[0], resp.play1[1], 1);
					update_info(&update_string, "255-255-255", "255-255-255", " ", resp.play2[0], resp.play2[1], 0);

					args_wait2->message = update_string;
					args_wait2->x1 = resp.play1[0];
					args_wait2->y1 = resp.play1[1];
					args_wait2->x2 = resp.play2[0];
					args_wait2->y2 = resp.play2[1];

					int res = pthread_create(&two_sec_wait, NULL, wait_2_seconds_return, args_wait2);
				} else if (resp_code == 3){

					printf("GAME IS FINISH!\n");
					if(args_5sec->lock == 1){
						pthread_cancel(five_sec_wait);
					}
					update_info(&update_string, player_color[id], "0-0-0", resp.str_play1, resp.play1[0], resp.play1[1], 1);
					update_info(&update_string, player_color[id], "0-0-0", resp.str_play2, resp.play2[0], resp.play2[1], 0);
					write_to_all(&update_string, sizeof(update_string));
					
					char game_finish[] = "game-finished";
					write_to_all(game_finish, sizeof(game_finish));
			

					//create a new board
					sleep(10);
					clear_board(board_dim);
				}
			}
		}
		// if it gets here, ther is not enough players
	}

}

void siginthandler(){
	close(GLOBAL_SOCK_FD);
	printf("closing global sock_fd");
	pthread_mutex_destroy(&mutex_lock);
	exit(1);
}

int main(){

	signal(SIGINT, siginthandler);
	
	pthread_mutex_init(&mutex_lock, NULL);

	board_dim = 4;	

	init_board(board_dim);

	init_connections();

	while (1)
	{	
		printf("Waiting for player %d ... \n", nb_connections);
		connect_to_player(nb_connections);

		//receive information about bot or player
		int player_id = nb_connections;

		char bot[3];
		read(player_fd[player_id][0], &bot, sizeof(bot));
		
		char * color = (char *)malloc(15*sizeof(char));
		random_color(color);
		strcpy(player_color[player_id], color);
		
		if(strcmp(bot, "b") == 0){
			printf("is a bot\n");
			pthread_create(&players_thread[player_id], NULL, bot_thread, &player_id);
		} else {
			printf("is a client\n");
			pthread_create(&players_thread[player_id], NULL, player_main, &player_id);
		}

		nb_connections++;
		nb_active_players++;
	}

}



