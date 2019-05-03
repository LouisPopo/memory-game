#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <pthread.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <unistd.h>
#include <string.h>

#include "board_library.h"

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
	
	*player_fd = accept(sock_fd, NULL, NULL);
	
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

int main(){
	
	int board_dim = 4;
	
	// Initialize the board
	init_board(board_dim);
	
	// Wait for the connection...
	int player_fd;
	connect_to_player(&player_fd);
	
	// For multiplayer -> create a thread waiting for connections and one per client. 
	
	// Sends the board dim
	send_int(board_dim, player_fd);

	// Gets in a loop where : receive click, verify the card, send info
	int done = 0;
	int board_x, board_y;
	
	int cnt = 0;
	char update_string[100];
	
	while(1){
		
		//receive coords of board 
		receive_card_coords(&player_fd, &board_x, &board_y);
		
		//play and send card coord and color
		
		play_response resp = board_play(board_x, board_y);
		
		update_string[0] = '\0';
		
		switch (resp.code) {
			case 1: // First - Only one card to update Send a single update message
				update_info(&update_string, "7-200-100", "200-200-200", resp.str_play1, resp.play1[0], resp.play1[1], 1);
				write(player_fd, &update_string, sizeof(update_string));
				
				break;
			case 3: // FINNISH
				done = 1;
			case 2: //Correct - Two cards to update at the same time : send a message with two updates
				update_info(&update_string, "107-200-100", "0-0-0", resp.str_play1, resp.play1[0], resp.play1[1], 1);
				update_info(&update_string, "107-200-100", "0-0-0", resp.str_play2, resp.play2[0], resp.play2[1], 0);
				write(player_fd, &update_string, sizeof(update_string));
				//write
				break;
			case -2: //Incorrect
				update_info(&update_string, "107-200-100", "255-0-0", resp.str_play1, resp.play1[0], resp.play1[1], 1);
				update_info(&update_string, "107-200-100", "255-0-0", resp.str_play2, resp.play2[0], resp.play2[1], 0);
				write(player_fd, &update_string, sizeof(update_string));
				//write
				printf("STRING: %s\n", update_string);
				
				sleep(2);
				 
				update_string[0] = '\0';
				update_info(&update_string, "255-255-255", "255-255-255", " ", resp.play1[0], resp.play1[1], 1);
				update_info(&update_string, "255-255-255", "255-255-255", " ", resp.play2[0], resp.play2[1], 0);
				write(player_fd, &update_string, sizeof(update_string));
				
				//write
				
				break;
		}
		
		
		
		
		
	}
	
}

