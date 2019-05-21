#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <pthread.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h> 
#include <time.h>

#include "board_library.h"
#define MAX_PLAYER 100

int under_5s = 1;

int player_fd[MAX_PLAYER];
char player_color[MAX_PLAYER][15];
pthread_t players_thread[MAX_PLAYER];
int nb_players = 0;
int board_dim;
int done = 0;
int isFiveSecRun = 0;

struct thread_args{
	int * lock;
	char * message;
	int id;
	int * running_flag;
};

int GLOBAL_SOCK_FD;

void init_connections(){
	struct sockaddr_in local_addr;
	
	GLOBAL_SOCK_FD = socket(AF_INET, SOCK_STREAM, 0);
	if(GLOBAL_SOCK_FD == -1){
		perror("socket: ");
		exit(-1);
	}
	local_addr.sin_family = AF_INET;
	local_addr.sin_port= htons(3000);
	local_addr.sin_addr.s_addr= INADDR_ANY;
	int err = bind(GLOBAL_SOCK_FD, (struct sockaddr *)&local_addr, sizeof(local_addr));
	if(err == -1) {
		perror("bind");
		exit(-1);
	}
	printf(" socket created and binded \n");
	listen(GLOBAL_SOCK_FD, 5);
}

void connect_to_player(int player_i){
	player_fd[player_i] = accept(GLOBAL_SOCK_FD, NULL, NULL);	
}

void * wait_2_seconds_return(void * arguments){
	
	int val;

	struct thread_args * args = (struct thread_args *) arguments;
	
	*(args->lock) = 1;
	printf("lock in thread : %d\n addr : %hx\n", *(args->lock), args->lock);
	size_t len = strlen(args->message);
	
	// Wait...
	sleep(2);
	
	// Return the "wrong" cards.
	printf("Sending the wrong cards to return\n");
	printf("Message to send : %s\n", args->message);
	for(int i = 0; i <nb_players; i++)
		write(player_fd[i], args->message, len);

	*(args->lock) = 0;
	//printf("after putting zero = %d\n addr = %hx\n",locker, &locker);
	
	//free(args);

	pthread_exit(0);
	
}

void * wait_5_sec_for_play(void * arguments){
	struct thread_args * args = (struct thread_args *) arguments;

	if(*(args->running_flag)){
		pthread_exit(0);
	} else {
		*(args->running_flag) = 1;
		printf("Waiting 5 seconds for second play...\n");
		//char * message = (char *) args->message;
		size_t len = strlen(args->message);
		printf("flag in thread: %d\n", *(args->lock));
		
		sleep(5);

		if(*(args->lock) == 0){
			//flip card
			for(int i = 0; i < nb_players; i++){
				write(player_fd[i], args->message, len);
			}
			reset_play(args->id);
			args->message[0] = '\0';
		}
		*(args->running_flag) = 0;
	}
	
	
	
	//free(args);

	//pthread_exit(0);
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
	read(player_fd[player_id], &coords, sizeof(coords));
	
	char *delim = "-";
	
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
	printf("in function : %s\n", color_buf);
}


void * player_main(void * args){
	int * id_pntr = args;
	int id = *id_pntr;
	printf("In player thread : %d\n", id);
	send_int(board_dim, player_fd[id]);

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

	pthread_t pid1, pid2;
	
	
	time_t t1, t2;

	int case1 = 0;


	while(1 && !done){
		printf("locker = %d\n addr = %hx\n",locker, &locker);
		
		time(&t1);
		receive_card_coords(id, &board_x, &board_y);
		time(&t2);
		
		if (locker != 1){
			
			play_response resp = board_play(board_x, board_y, id);
			
			memset(update_string, 0, sizeof(update_string));
			memset(reset_string, 0, sizeof(reset_string));

			int resp_code = resp.code;
			
			if(resp_code == 1){
				update_info(&update_string, player_color[id], "200-200-200", resp.str_play1, resp.play1[0], resp.play1[1], 1);
					
				for(int i = 0; i < nb_players; i++){
					write(player_fd[i], &update_string, sizeof(update_string));
				}
				

				update_info(&reset_string, "255-255-255", "255-255-255", " ", resp.play1[0], resp.play1[1], 1);
				args_5sec->message = reset_string;
				fsec_flag = 0;
				
				pthread_create(&pid2, NULL, wait_5_sec_for_play, args_5sec);
				//free(pid);
				

			} else if (resp_code == 2){
				
				fsec_flag = 1;

				update_info(&update_string, player_color[id], "0-0-0", resp.str_play1, resp.play1[0], resp.play1[1], 1);
				update_info(&update_string, player_color[id], "0-0-0", resp.str_play2, resp.play2[0], resp.play2[1], 0);
				for(int i = 0; i < nb_players; i++){
					write(player_fd[i], &update_string, sizeof(update_string));
				}
			} else if (resp_code == -2){

				fsec_flag = 1;

				update_info(&update_string, player_color[id], "255-0-0", resp.str_play1, resp.play1[0], resp.play1[1], 1);
				update_info(&update_string, player_color[id], "255-0-0", resp.str_play2, resp.play2[0], resp.play2[1], 0);
				
				for(int i = 0; i < nb_players; i++){
					write(player_fd[i], &update_string, sizeof(update_string));
				}

				update_string[0] = '\0';

				update_info(&update_string, "255-255-255", "255-255-255", " ", resp.play1[0], resp.play1[1], 1);
				update_info(&update_string, "255-255-255", "255-255-255", " ", resp.play2[0], resp.play2[1], 0);

				args_wait2->message = update_string;
				
				int res = pthread_create(&pid1, NULL, wait_2_seconds_return, args_wait2);
				printf("res = %d\n", res);
			}
		}

		
	}
	printf("FINISH\n");
}


int main(){
	
	//srand(time(NULL)); 

	board_dim = 4;	

	init_board(board_dim);

	// initialize a mutex that is shared between threads with an initial value of 1

	// goes in a loop and wait for players...
	// each player connecting go to his thread...

	init_connections();
	
	while (1)
	{	
		printf("Waiting for player %d ... \n", nb_players);
		connect_to_player(nb_players);

		
		int player_id = nb_players;

		char * color = (char *)malloc(15*sizeof(char));
		random_color(color);
		strcpy(player_color[player_id], color);

		pthread_create(&players_thread[player_id], NULL, player_main, &player_id);

		nb_players++;
	}

}


/*
int s_main(){
	
	// Initialize the board
	init_board(board_dim); 
	
	// Wait for the connection...
	
	connect_to_player(&player_fd);
	
	// For multiplayer -> create a thread waiting for connections and one per client. 
	
	// Sends the board dim
	//send_int(4, player_fd);

	// Gets in a loop where : receive click, verify the card, send info
	int done = 0;
	int board_x, board_y;
	
	char update_string[100];
	char reset_string[100];
	
	pthread_t pid1, pid2;
	
	while(1 && !done){
		
		//receive coords of board 
		receive_card_coords(&player_fd, &board_x, &board_y);
		
		//play and send card coord and color if not locked
		if(!block_message){
			if(under_5s){
				play_response resp = board_play(board_x, board_y, 0);
				
				memset(update_string, 0, sizeof(update_string));
				memset(reset_string, 0, sizeof(reset_string));
				
				switch (resp.code) {
					case 1: // First - Only one card to update Send a single update message
						update_info(&update_string, "7-200-100", "200-200-200", resp.str_play1, resp.play1[0], resp.play1[1], 1);
						write(player_fd, &update_string, sizeof(update_string));
						
						
						update_info(&reset_string, "255-255-255", "255-255-255", " ", resp.play1[0], resp.play1[1], 1);
						
						//Should start a timer of 5 second, wait and return card if nothing is played.
						struct thread_args *args_5sec = malloc(sizeof(struct thread_args));
						args_5sec->lock = &under_5s;
						args_5sec->message = reset_string;

						pthread_create(&pid2, NULL, wait_5_sec_for_play, args_5sec);
						
						break;
					case 3: // FINNISH
						done = 1;
					case 2: //Correct - Two cards to update at the same time : send a message with two updates
						
						//kill thread
						pthread_cancel(pid2);
						update_info(&update_string, "107-200-100", "0-0-0", resp.str_play1, resp.play1[0], resp.play1[1], 1);
						update_info(&update_string, "107-200-100", "0-0-0", resp.str_play2, resp.play2[0], resp.play2[1], 0);
						write(player_fd, &update_string, sizeof(update_string));
						
						//write
						break;
					case -2: //Incorrect
						pthread_cancel(pid2);
						update_info(&update_string, "107-200-100", "255-0-0", resp.str_play1, resp.play1[0], resp.play1[1], 1);
						update_info(&update_string, "107-200-100", "255-0-0", resp.str_play2, resp.play2[0], resp.play2[1], 0);
						write(player_fd, &update_string, sizeof(update_string));
						//write
						
						// should lock, start a thread that wait 2 seconds, return cards, unlock and finish
						//sleep(2); //Durning this time the server should not accept new messages
						 
						update_string[0] = '\0';
						update_info(&update_string, "255-255-255", "255-255-255", " ", resp.play1[0], resp.play1[1], 1);
						update_info(&update_string, "255-255-255", "255-255-255", " ", resp.play2[0], resp.play2[1], 0);
						
						struct thread_args *args_wait2 = malloc(sizeof(struct thread_args));
						args_wait2->lock = &block_message;
						args_wait2->message = update_string;
						
						pthread_create(&pid1, NULL, wait_2_seconds_return, args_wait2);
						
						
						break;
				}
			} 
			
		}
		
		
		
	}
	
}*/

