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
#include <signal.h>

#include "board_library.h"
#define MAX_PLAYER 100

int under_5s = 1;

int player_fd[MAX_PLAYER][2] = {
	{0,0},
	{0,0}
};

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
	player_fd[player_i][0] = accept(GLOBAL_SOCK_FD, NULL, NULL);	
	player_fd[player_i][1] = 1;
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
	
	write_to_all(args->message, len);
	
	*(args->lock) = 0;

	reset_cell_status(args->x1, args->y1);
	reset_cell_status(args->x2, args->y2);
	pthread_exit(0);
	
}

void * wait_5_sec_for_play(void * arguments){
	struct thread_args * args = (struct thread_args *) arguments;

	if(*(args->running_flag)){
		pthread_exit(0);
	} else {
		*(args->running_flag) = 1;
		printf("Waiting 5 seconds for second play (%d,%d)...\n", args->x1, args->y1);
		//char * message = (char *) args->message;
		size_t len = strlen(args->message);
		printf("flag in thread: %d\n", *(args->lock));
		
		sleep(5);

		if(*(args->lock) == 0){
			//flip card
			write_to_all(args->message, len);

			reset_play(args->id, args->x1, args->y1);
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
	printf("in function : %s\n", color_buf);
}

void deconnect_player(int id){
	//close the socket
	close(player_fd[id][0]);
	//make it inactive
	player_fd[id][1] == 0;
}

void write_to_all(char * message, int size){
	for(int i = 0; i < nb_players; i++){
		if(player_fd[i][1] == 1){
			write(player_fd[i][0], message, size);
		}
	}
}


void * player_main(void * args){
	int * id_pntr = args;
	int id = *id_pntr;
	printf("In player thread : %d\n", id);
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
	

	pthread_t pid1, pid2;
	
	
	time_t t1, t2;

	int case1 = 0;

	//has to update board
	struct cell_info infos;
	char cell_str_status[100];

	//cell_status = (* board_place)malloc(sizeof(board_place));
	for(int i = 0; i < board_dim; i++){
		for(int j = 0; j < board_dim; j++){
			get_cell_status(&infos,i, j);
			printf("received for (%d,%d)\n", i, j);
			
			if(infos.player_id != -1){
				printf("need to update (%d,%d)\n", i, j);
				int player_id = infos.player_id;
				
				printf("player_color = %s\n", player_color[id]);
				printf("str_color = %s\n", infos.string_color);
				printf("str = %s\n", infos.str);

				char i_str[2];
				char j_str[2];
				sprintf(i_str, "%d", i);
				sprintf(j_str, "%d", j);
				

				sprintf(cell_str_status, "%s:%s:%s:%s:%s", player_color[player_id], infos.string_color, infos.str, i_str, j_str);

				write(player_fd[id][0], &cell_str_status, sizeof(cell_str_status));

				cell_str_status[0] = '\0';

				printf("writing : %s\n", cell_str_status);
				printf("sended\n");
			}
		}
	}

	char end[3] = "***";
	write(player_fd[id][0], &end, sizeof(end));

	printf("send end of message ***\n");
	
	


	while(1 && !done){
		printf("locker = %d\n addr = %hx\n",locker, &locker);
		
		time(&t1);
		receive_card_coords(id, &board_x, &board_y);
		if(board_x == -1 && board_y == -1){
			printf("client %d wants to deconect\n", id);
			deconnect_player(id);
			
			break;
		}
		time(&t2);
		
		if (locker != 1){
			
			play_response resp = board_play(board_x, board_y, id);
			
			memset(update_string, 0, sizeof(update_string));
			memset(reset_string, 0, sizeof(reset_string));

			int resp_code = resp.code;
			
			if(resp_code == 1){
				update_info(&update_string, player_color[id], "200-200-200", resp.str_play1, resp.play1[0], resp.play1[1], 1);
				
				printf("update string : %s with size : %d\n", update_string, sizeof(update_string));
				
				write_to_all(&update_string, sizeof(update_string));
				

				update_info(&reset_string, "255-255-255", "255-255-255", " ", resp.play1[0], resp.play1[1], 1);
				args_5sec->message = reset_string;
				args_5sec->x1 = board_x;
				args_5sec->y1 = board_y;

				fsec_flag = 0;
				
				pthread_create(&pid2, NULL, wait_5_sec_for_play, args_5sec);
				//free(pid);

				

			} else if (resp_code == 2){
				
				fsec_flag = 1;

				update_info(&update_string, player_color[id], "0-0-0", resp.str_play1, resp.play1[0], resp.play1[1], 1);
				update_info(&update_string, player_color[id], "0-0-0", resp.str_play2, resp.play2[0], resp.play2[1], 0);
				write_to_all(&update_string, sizeof(update_string));

			} else if (resp_code == -2){

				fsec_flag = 1;

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

				
				int res = pthread_create(&pid1, NULL, wait_2_seconds_return, args_wait2);

			}
		}

		
	}
	printf("FINISH\n");
}

void siginthandler(){
	close(GLOBAL_SOCK_FD);
	exit(1);
}

int main(){

	//redirect to close the socket
	signal(SIGINT, siginthandler);
	
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



