#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "UI_library.h"


#define WHITE "255-255-255"
#define BLACK "0-0-0"
#define RED "255-0-0"
#define GREY "200-200-200"

#define MEMORY_PORT 3000

int * available_cells;
int dim_board;


int is_available(int i, int j){ // return one if the cell is available
  	return available_cells[j*dim_board+i];
}

void change_availability(int x, int y, int is_available){
	available_cells[y*dim_board+x] = is_available;
}

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
	
	printf("BOT connected\n");
}

void send_card_coordinates(int x, int y, int * sock_fd){
	char coords[10];
	sprintf(coords, "%d+%d", x, y);
	printf("Coords: %s\n", coords);
	write(*sock_fd, &coords, sizeof(coords));
}

void receive_int(int * num, int fd){
	int32_t ret;
	char * data = (char*)&ret;
	read(fd, data, sizeof(ret));
	*num = ntohl(ret);
}

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
	
	if(strcmp(text_color, WHITE))
		*available = 1;
	else
		*available = 0;
}

void * receive_thread(void * sock_fd_arg){
	
	int * sock_fd = sock_fd_arg;

	char buffer[200];
	int x,y,is_available;
	char * updates_delimiter = "=";
	
	while(1){
		read(*sock_fd, &buffer, sizeof(buffer));

		if (strstr(buffer, "=") != NULL) { //there is two cells to change
			
			char * play1;
			char * play2;
			play1 = strtok(buffer, updates_delimiter);
			play2 = strtok(NULL, updates_delimiter);
		
			get_coords_and_availability(&x,&y,&is_available, play1);
			printf("(%d,%d) : %d\n", x,y, is_available);
			change_availability(x,y,is_available);

			get_coords_and_availability(&x,&y,&is_available, play2);
			printf("(%d,%d) : %d\n", x,y, is_available);
			change_availability(x,y,is_available);

		} else {
			get_coords_and_availability(&x,&y,&is_available, buffer);
			printf("(%d,%d) : %d\n", x,y, is_available);
			change_availability(x,y,is_available);
		}

		memset(buffer, 0, sizeof(buffer));
	}
}

int main(int argc, char * argv[]){
	
	char DEFAULT_IP_ADDRESS[] = "127.0.0.1";
	char * ip_addr;
	int sock_fd;
	
	if (argc < 2)
		ip_addr = DEFAULT_IP_ADDRESS; 
	else
		ip_addr = argv[1];
	
	connect_to_server(ip_addr, &sock_fd);

	char bot[3];
	strcpy(bot, "b");
	write(sock_fd, &bot, sizeof(bot));
	
	// Get the dimension of the board and creates it.
	
	printf("waiting dim...\n");
	receive_int(&dim_board, sock_fd);
	printf("OK!\n");
	
	printf("received size : %d\n", dim_board);

	available_cells = malloc(sizeof(int) * dim_board * dim_board);
	for (int i = 0; i < dim_board; i++){
		for(int j = 0; j < dim_board; j++){
			available_cells[i] = 1;
		}
	}

	pthread_t pid;

	pthread_create(&pid, NULL, receive_thread, &sock_fd);
	int card_x, card_y;
	//sending random cords that are active
	srand(time(NULL));

	while(1){
		

		printf("thinking of first pick...");
		do{
			card_x = rand() % dim_board;
        	card_y = rand() % dim_board;
		} while(!is_available(card_x, card_y));
        printf("DONE!\n");
		send_card_coordinates(card_x, card_y, &sock_fd);
		change_availability(card_x, card_y, 0);

		sleep(3);
		printf("thinking of second pick...");
		do{
			card_x = rand() % dim_board;
        	card_y = rand() % dim_board;
		} while(!is_available(card_x, card_y));
		printf("DONE!\n");
		send_card_coordinates(card_x, card_y, &sock_fd);
		change_availability(card_x, card_y, 0);
		
		printf("sleeping 3 seconds...");
		sleep(3);	
        printf("DONE!\n");
    }

}