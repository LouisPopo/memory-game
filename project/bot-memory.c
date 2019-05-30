#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>

#include "UI_library.h"


#define WHITE "255-255-255"
#define BLACK "0-0-0"
#define RED "255-0-0"
#define GREY "200-200-200"

#define MEMORY_PORT 3000

int * available_cells;
int n_correct = 0;
int dim_board;
int sock_fd;

pthread_t pid_gen;
int isover = 0;

// Convert the i and j index to a single index for the one dimension array
int linear_conv(int i, int j){
	return j*dim_board+i;
}

// Thread that generates random x and y until the card is available and then sends the 
// first pick to the server, sleep for a few seconds and sends his second pick
void * play_thread(void * args){
	int card_x, card_y;
	srand(time(NULL));

	while(1 && !isover){
		
		sleep(1);
		
		do{
			card_x = rand() % dim_board;
        	card_y = rand() % dim_board;
		} while(!is_available(card_x, card_y) && n_correct < (dim_board*dim_board)); 

		if(n_correct == (dim_board*dim_board)){
			printf("faoufadiuf");
		}

		send_card_coordinates(card_x, card_y);
		//change_availability(card_x, card_y, 0);
		sleep(2);
		
		do{
			card_x = rand() % dim_board;
        	card_y = rand() % dim_board;
		} while(!is_available(card_x, card_y) && n_correct < (dim_board*dim_board));
		
		if(n_correct == (dim_board*dim_board)){
			printf("fghabhbubhb");
		}

		send_card_coordinates(card_x, card_y);
		//change_availability(card_x, card_y, 0);
		
		sleep(3);
    }
	if(isover){
		send_card_coordinates(-1,-1);
	}
	pthread_exit(1);
}

// Verify if the card is available
int is_available(int i, int j){ 
  	return available_cells[linear_conv(i,j)];
}

// Change the availability of a card
void change_availability(int x, int y, int is_available){
	
	available_cells[linear_conv(x,y)] = is_available;
	printf("(%d,%d) = %d\n", x,y,is_available);
}

// Connect to the server
void connect_to_server(char ip_addr[]){
	struct sockaddr_in server_addr;
	
	sock_fd= socket(AF_INET, SOCK_STREAM, 0);
	
	if (sock_fd == -1){
		perror("socket: ");
		exit(-1);
	}
	
	
	server_addr.sin_family = AF_INET;
	server_addr.sin_port= htons(MEMORY_PORT);
	inet_aton(ip_addr, &server_addr.sin_addr);
	

	
	if(connect(sock_fd,(const struct sockaddr *) &server_addr,sizeof(server_addr)) == -1){
		printf("Error connecting\n");
		exit(-1);
	}
	
	printf("BOT connected\n");
}

//Convert the x and y generated to a string and sends it to the server
void send_card_coordinates(int x, int y){
	char coords[10];
	sprintf(coords, "%d+%d", x, y);
	printf("Card picked : %s\n", coords);
	write(sock_fd, &coords, sizeof(coords));
}

// Receive a string and convert it to an int
void receive_int(int * num, int fd){
	int32_t ret;
	char * data = (char*)&ret;
	read(fd, data, sizeof(ret));
	*num = ntohl(ret);
}

// After receiving a string with all the information of a card that changed status, it parse the string
// to find the new availability, and the x and y of the card 
void get_coords_and_availability(int * x, int * y, int * available, char buffer[]){
	char delim[] = ":";
		
	char * card_color;
	char * text_color;
	char * char_to_write;
		
	card_color = strtok(buffer, delim);
	text_color = strtok(NULL, delim);
	char_to_write = strtok(NULL, delim);
	*x = atoi(strtok(NULL, delim));
	*y = atoi(strtok(NULL, delim));
	
	if(strcmp(text_color, WHITE) == 0){
		*available = 1;
		n_correct--;
	}
	else {
		*available = 0;
		n_correct++;
	}
	
}

// Thread that handle all update string that are sent by the server
void * receive_thread(void * args){
	

	char buffer[200];
	int x,y,is_available;
	char * updates_delimiter = "=";
	
	while(1){

		read(sock_fd, &buffer, sizeof(buffer));
		if(strcmp(buffer, "over") == 0){
			printf("RECEIVED OVER");
			close(sock_fd);
			pthread_exit(1);
		} else if(strcmp(buffer, "game-finished") == 0){
			printf("The Game is Finish!\n");
			pthread_cancel(pid_gen);
			pthread_create(&pid_gen, NULL, play_thread, NULL);
			sleep(10);
			erase_board();
		} else {
			
			if (strstr(buffer, "=") != NULL) { //there is two cells to change
			
				char * play1;
				char * play2;
				play1 = strtok(buffer, updates_delimiter);
				play2 = strtok(NULL, updates_delimiter);
			
				get_coords_and_availability(&x,&y,&is_available, play1);
				change_availability(x,y,is_available);

				get_coords_and_availability(&x,&y,&is_available, play2);
				change_availability(x,y,is_available);

			} else {
				get_coords_and_availability(&x,&y,&is_available, buffer);
				change_availability(x,y,is_available);
			}
		}
		memset(buffer, 0, sizeof(buffer));
	}
}

// Make every cell available
void erase_board(){
	printf("ERASEING BOARD MEMORY\n");
	available_cells = malloc(sizeof(int) * dim_board * dim_board);
	for (int i = 0; i < dim_board; i++){
		for(int j = 0; j < dim_board; j++){
			available_cells[linear_conv(i,j)] = 1;
		}
	}
}

// When killing this executable (CTRL + C), this global variable is changed, the bot sends his last pick
// (if he was generating numbers) and disconnect himself by closing the socket
void siginthandler(){
	
	isover = 1;
}

// Change the availability of a cell
void update_availability(char play[]){
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
	
	change_availability(x_pos, y_pos, 0);

}

// Initialize everything necessary, connect to the server, sent the information to let the server
// knows that it's a bot, get the board dimension, generate his availability array and creates two threads
int main(int argc, char * argv[]){


	signal(SIGINT, siginthandler);

	char DEFAULT_IP_ADDRESS[] = "127.0.0.1";
	char * ip_addr;
	
	
	if (argc < 2)
		ip_addr = DEFAULT_IP_ADDRESS; 
	else
		ip_addr = argv[1];
	
	connect_to_server(ip_addr);

	char bot[3];
	strcpy(bot, "b");
	write(sock_fd, &bot, sizeof(bot));
	
	// Get the dimension of the board and creates it.
	
	printf("waiting dim...\n");
	receive_int(&dim_board, sock_fd);
	printf("OK!\n");
	
	printf("received size : %d\n", dim_board);

	/// MAKE THEM ALL AVAILABLE
	available_cells = malloc(sizeof(int) * dim_board * dim_board);
	for (int i = 0; i < dim_board; i++){
		for(int j = 0; j < dim_board; j++){
			available_cells[linear_conv(i,j)] = 1;
		}
	}

	char cell_info[100];
	while(1){
		//received update of the actual board
		read(sock_fd, &cell_info, sizeof(cell_info));
		
		if(strcmp(cell_info, "***") == 0){
			break;
		} else {
			update_availability(cell_info);
			memset(cell_info, 0, sizeof(cell_info));
		}
	}
	///

	pthread_t pid_rcv;

	pthread_create(&pid_rcv, NULL, receive_thread, NULL);
	pthread_create(&pid_gen, NULL, play_thread, NULL);
	pthread_join(pid_rcv, NULL);
	pthread_join(pid_gen, NULL);
	

	//sending random cords that are active
	

	
	exit(1);

}