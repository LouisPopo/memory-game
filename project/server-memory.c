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

#include "board_library.h"
#define MAX_PLAYERS 100

int block_message = 0;
int under_5s = 1;

int player_fd;
int nb_players = 0;

struct thread_args{
	int * lock;
	char * message;
};



void connect_to_player(int * player_fd){
	struct sockaddr_in local_addr;
	
	int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(sock_fd == -1){
		perror("socket: ");
		exit(-1);
	}
	local_addr.sin_family = AF_INET;
	local_addr.sin_port= htons(3000);
	local_addr.sin_addr.s_addr= INADDR_ANY;
	int err = bind(sock_fd, (struct sockaddr *)&local_addr, sizeof(local_addr));
	if(err == -1) {
		perror("bind");
		exit(-1);
	}
	printf(" socket created and binded \n");
	
	listen(sock_fd, 5);
	
	* player_fd = accept(sock_fd, NULL, NULL);
	
}

void * wait_2_seconds_return(void * arguments){
	struct thread_args * args = (struct thread_args *) arguments;
	// mess = args->message
	// lock = *(args->lock)
	*(args->lock) = 1;
	
	size_t len = strlen(args->message);
	
	// Wait...
	sleep(1);
	
	// Return the "wrong" cards.
	printf("Sending thw wrgon cards to return\n");
	write(player_fd, args->message, len);
	*(args->lock) = 0;
	free(args);
	//pthread_mutex_unlock(&lock);
	pthread_exit(0);
	
}

void * wait_5_sec_for_play(void * arguments){
	struct thread_args * args = (struct thread_args *) arguments;

	printf("Waiting 5 seconds for second play...\n");
	//char * message = (char *) args->message;
	size_t len = strlen(args->message);
	
	*(args->lock) = 1;
	//under_5s = 1;
	sleep(3);
	*(args->lock) = 0;
	//under_5s= 0;
	
	printf("It took more than 5 seconds!\n");
	
	printf("Sending reset : string\n");
	printf("%s\n", args->message);
	write(player_fd, args->message, len);
	printf("Reseting play...\n");
	*(args->lock) = 1;
	//under_5s = 1;
	args->message[0] = '\0';
	reset_play();

	free(args);

	pthread_exit(0);
}

// Function to send int to client
void send_int(int num, int fd){
	int32_t conv = htonl(num);
	char * data = (char*)&conv;

	write(fd, data, sizeof(conv));
}

// Receive a string (x-y) and convert the string of x and y to integers
void receive_card_coords(int * sock_fd, int * x, int * y){
	
	char coords[10];
	read(*sock_fd, &coords, sizeof(coords));
	
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

void random_color(char * color_buffer){
	int r = rand() % 256;
	int g = rand() % 256;
	int b = rand() % 256;
	sprintf(color_buffer, "%d:%d:%d", r,g,b);
}

/*int mulitple_main(){
	int board_dim = 4;

	init_board(board_dim);

	// goes in a loop and wait for players...
	// each player connecting go to his thread...

	init_connections();
	while (1)
	{
		connect_to_player(nb_players);
		nb_players++;
		//char * color;
		//random_color(color);
		//printf("Random color : %s\n", color);	
	}
	
}*/



int main(){
	
	int board_dim = 4;
	
	
	// Initialize the board
	init_board(board_dim);
	
	// Wait for the connection...
	
	connect_to_player(&player_fd);
	
	// For multiplayer -> create a thread waiting for connections and one per client. 
	
	// Sends the board dim
	send_int(board_dim, player_fd);

	// Gets in a loop where : receive click, verify the card, send info
	int done = 0;
	int board_x, board_y;
	
	int cnt = 0;
	char update_string[100];
	char reset_string[100];
	
	pthread_t pid1, pid2;
	
	while(1 && !done){
		
		//receive coords of board 
		receive_card_coords(&player_fd, &board_x, &board_y);
		
		//play and send card coord and color if not locked
		if(!block_message){
			if(under_5s){
				play_response resp = board_play(board_x, board_y);
				
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
	
}

