#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "UI_library.h"

#define MEMORY_PORT 3000

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
	printf("data received : %d\n", ret);
	*num = ntohl(ret);
}

void send_card_coordinates(int x, int y, int * sock_fd){
	char coords[10];
	sprintf(coords, "%d+%d", x, y);
	printf("Coords: %s\n", coords);
	write(*sock_fd, &coords, sizeof(coords));
}

void * mouse_click_thread(void * sock_fd_arg){
	SDL_Event event;
	int done = 0;
	int * sock_fd = sock_fd_arg;
	
	while(!done){
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				
				case SDL_QUIT: {
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

void string_color_to_RGB(char * string_color, int * RGB){
	char delim[] = "-";
	char * copy_string = malloc(strlen(string_color) + 1);
	strcpy(copy_string, string_color);
	
	RGB[0] = atoi(strtok(copy_string, delim));
	RGB[1] = atoi(strtok(NULL, delim));
	RGB[2] = atoi(strtok(NULL, delim));
}

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
		
	printf("Char write : %s\n", char_to_write);
		
	paint_card(x_pos, y_pos, card_RGB[0], card_RGB[1], card_RGB[2]);
	write_card(x_pos, y_pos, char_to_write, text_RGB[0], text_RGB[1], text_RGB[2]);
}

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

void * update_board_thread(void * sock_fd_arg){
	int * sock_fd = sock_fd_arg;
	
	//int cnt = 0;
	while(1){
		char update_info[200];
		read(*sock_fd, &update_info, sizeof(update_info));
		
		printf("Received the message : %s \n", update_info);
		
		parse_plays(update_info);
		memset(update_info, 0, sizeof(update_info));
		update_info[0] = '\0';

		//cnt++;
	}
	
	pthread_exit(NULL);
}

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
	
	// Get the dimension of the board and creates it.
	int board_dim;
	printf("waiting dim...\n");
	receive_int(&board_dim, sock_fd);
	printf("OK!\n");
	
	printf("received size : %d\n", board_dim);
	int err = create_board_window(300, 300, board_dim);

	char cell_info[100];
	while(1){
		//received update of the actual board
		read(sock_fd, &cell_info, sizeof(cell_info));
		
		printf("received the cell info : %s\n", cell_info);
		if(strcmp(cell_info, "***") == 0){
			break;
		} else {
			printf("received to update : %s\n", cell_info);
			update_board(cell_info);
			memset(cell_info, 0, sizeof(cell_info));
		}
	}
	printf("finish receiving info board\n");
	
	// Create two Threads : 
		// 1 - Process the mouse clicks and send the info to the server
		// 2 - Receive update to apply on the board
	
	pthread_t m_thread, b_thread;	
	
	pthread_create(&m_thread, NULL, mouse_click_thread, &sock_fd);
	pthread_create(&b_thread, NULL, update_board_thread, &sock_fd);
	
	pthread_join(m_thread,NULL);
	
	exit(1);
}
