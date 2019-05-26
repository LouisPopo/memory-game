#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "UI_library.h"

#define MEMORY_PORT 3000

int done = 0;
int board_dim;

// Connect to the server
void connect_to_server(char ip_addr[], int * sock_fd){
	struct sockaddr_in server_addr;
	
	*sock_fd= socket(AF_INET, SOCK_STREAM, 0);
	
	if (*sock_fd == -1){
		perror("socket: ");
		exit(-1);
	}
	
	
	server_addr.sin_family = AF_INET;
	server_addr.sin_port= htons(MEMORY_PORT);
	inet_aton(ip_addr, &server_addr.sin_addr);
	

	
	if(connect(*sock_fd,(const struct sockaddr *) &server_addr,sizeof(server_addr)) == -1){
		printf("Error connecting\n");
		exit(-1);
	}

	printf("Client connected\n");
}

// Function to receive int from Server
void receive_int(int * num, int fd){
	int32_t ret;
	char * data = (char*)&ret;
	read(fd, data, sizeof(ret));
	*num = ntohl(ret);
}

// Sends the x and y coordinates of the card picked to the server
void send_card_coordinates(int x, int y, int * sock_fd){
	char coords[10];
	sprintf(coords, "%d+%d", x, y);
	printf("Click on (%d,%d)\n", x, y);
	write(*sock_fd, &coords, sizeof(coords));
}

// Handle the mouses clicks and call the corresponding functions
void * mouse_click_thread(void * sock_fd_arg){
	SDL_Event event;
	
	int * sock_fd = sock_fd_arg;
	
	while(!done){
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				
				case SDL_QUIT: {
					printf("closing the window\n");
					done = SDL_TRUE;
					break;
				}
				case SDL_MOUSEBUTTONDOWN:{
					int board_x, board_y;
					get_board_card(event.button.x, event.button.y, &board_x, &board_y);
					
					// Send the card coordinates to the server
					send_card_coordinates(board_x, board_y, sock_fd);
					
					
					break;
				}
			}
		}
	}
	close_board_windows();
	//should send a message to close the client
	send_card_coordinates(-1, -1, sock_fd);
	close(*sock_fd);

	pthread_exit(NULL);
}

// Convert a string of type "255-255-255" to 3 ints
void string_color_to_RGB(char * string_color, int * RGB){
	char delim[] = "-";
	char * copy_string = malloc(strlen(string_color) + 1);
	strcpy(copy_string, string_color);
	
	RGB[0] = atoi(strtok(copy_string, delim));
	RGB[1] = atoi(strtok(NULL, delim));
	RGB[2] = atoi(strtok(NULL, delim));
}

// Clear the board
void erase_board(){
	for(int x = 0; x < board_dim; x++){
		for(int y = 0; y < board_dim; y++){
			paint_card(x,y,255,255,255); 
		}
	}
}

// Gets a string with all information needed to change one cell
void update_board(char play[]){
	char delim[] = ":";
		
	char * card_color;
	char * text_color;
	char * char_to_write;
	int  x_pos;
	int  y_pos;
		
	card_color = strtok(play, delim);
	text_color = strtok(NULL, delim);
	char_to_write = strtok(NULL, delim);
	x_pos = atoi(strtok(NULL, delim));
	y_pos = atoi(strtok(NULL, delim));
		
	int card_RGB[3];
	int text_RGB[3];
		
	string_color_to_RGB(card_color, card_RGB);
	string_color_to_RGB(text_color, text_RGB);
		
		
	paint_card(x_pos, y_pos, card_RGB[0], card_RGB[1], card_RGB[2]);
	write_card(x_pos, y_pos, char_to_write, text_RGB[0], text_RGB[1], text_RGB[2]);
}

// When receiving a string with the information of two cards to change at the same time, 
// this function separate that string and update the board one for each card 
void parse_plays(char update_info[]){
	
	if (strstr(update_info, "=") != NULL) {
		char * updates_delimiter = "=";
		
		char * play1;
		char * play2;
		play1 = strtok(update_info, updates_delimiter);
		play2 = strtok(NULL, updates_delimiter);
		
		update_board(play1);
		update_board(play2);
	} else {
		
		update_board(update_info);
		
	}
		
}

// Wait to receive the information of a card to change and parse the play
void * update_board_thread(void * sock_fd_arg){
	int * sock_fd = sock_fd_arg;
	char update_info[100];

	while(1){
		memset(update_info, 0, sizeof(update_info));
		read(*sock_fd, &update_info, sizeof(update_info));
		
		if(strcmp(update_info, "over") == 0){ //over
			break;
		} else if(strcmp(update_info, "game-finished") == 0){ //over
			
			printf("The Game is FINISHED!\n");
			sleep(10);
			erase_board();

		} else {
			printf("Received the message : %s \n", update_info);
		
			parse_plays(update_info);
		}

	}
	
	pthread_exit(NULL);
}

// Initialize SDL to be able to see the board
void init_SDL_TTF(){
	if( SDL_Init( SDL_INIT_VIDEO ) < 0 ) {
		 printf( "SDL could not initialize! SDL_Error: %s\n", SDL_GetError() );
		 exit(-1);
	}
	if(TTF_Init()==-1) {
			printf("TTF_Init: %s\n", TTF_GetError());
			exit(2);
	}
}

// Connect to the server, send a string so the server knows it's a player and not a bot
// receives the dimensions of the board , create and update the board and creates two threads :
// one to receive the clicks from the user and another one to receive information to update the board
int main(int argc, char * argv[]){
	
	init_SDL_TTF();
	
	char DEFAULT_IP_ADDRESS[] = "127.0.0.1";
	char * ip_addr;
	int sock_fd;
	
	if (argc < 2)
		ip_addr = DEFAULT_IP_ADDRESS; 
	else
		ip_addr = argv[1];
	
	connect_to_server(ip_addr, &sock_fd);

	//send the fact that its a player
	char client[3];
	strcpy(client, "cl");
	write(sock_fd, client, sizeof(client));
	
	receive_int(&board_dim, sock_fd);
	
	int err = create_board_window(600, 600, board_dim);

	char cell_info[100];
	while(1){
		read(sock_fd, &cell_info, sizeof(cell_info));
		
		if(strcmp(cell_info, "***") == 0){
			break;
		} else {
			update_board(cell_info);
			memset(cell_info, 0, sizeof(cell_info));
		}
	}
	
	pthread_t m_thread, b_thread;	
	
	pthread_create(&m_thread, NULL, mouse_click_thread, &sock_fd);
	pthread_create(&b_thread, NULL, update_board_thread, &sock_fd);
	pthread_join(m_thread,NULL);
	pthread_join(b_thread, NULL);
	
	exit(1);
}
