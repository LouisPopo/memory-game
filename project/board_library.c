#include <stdlib.h>
#include "board_library.h"
#include <stdio.h>
#include <string.h>

#define WHITE "255-255-255"
#define BLACK "0-0-0"
#define RED "255-0-0"
#define GREY "200-200-200"

int dim_board;
board_place * board;
cell_info * cells_info;

pthread_mutex_t mutex_lock;

int play[100][2];


int play1[2];
int n_corrects;

// Convert the i and j index to a single index
int linear_conv(int i, int j){
  	return j*dim_board+i;
}

// Gets ths string of the card at the i and j position
char * get_board_place_str(int i, int j){
  	return board[linear_conv(i, j)].v;
}

// Places the string to random places
void reset_board(int dim){
	
	int count  = 0;
	int i, j;
	char * str_place;

	dim_board= dim;
	n_corrects = 0;
	
	for( i = 0; i < 100; i++){
		play[i][0]= -1;
	}
	
	board = malloc(sizeof(board_place)* dim *dim);
	cells_info = malloc(sizeof(cell_info) * dim *dim);
	
	for( i=0; i < (dim_board*dim_board); i++){
		board[i].v[0] = '\0';
		cells_info[i].str[0]= '\0';
		strcpy(cells_info[i].string_color, "");
		cells_info[i].player_id = -1;
	}

	for (char c1 = 'a' ; c1 < ('a'+dim_board); c1++){
		for (char c2 = 'a' ; c2 < ('a'+dim_board); c2++){
		do{
			i = random()% dim_board;
			j = random()% dim_board;
			str_place = get_board_place_str(i, j);
			printf("%d %d -%s-\n", i, j, str_place);
		}while(str_place[0] != '\0');
		str_place[0] = c1;
		str_place[1] = c2;
		str_place[2] = '\0';
		do{
			i = random()% dim_board;
			j = random()% dim_board;
			str_place = get_board_place_str(i, j);
			printf("%d %d -%s-\n", i, j, str_place);
		}while(str_place[0] != '\0');
		str_place[0] = c1;
		str_place[1] = c2;
		str_place[2] = '\0';
		count += 2;
		if (count == dim_board*dim_board)
			return;
		}
	}
}

// Init the mutex and create a board
void init_board(int dim){

	pthread_mutex_init(&mutex_lock, NULL);
	reset_board(dim);

}

// Reveive a pick, verify the availability, if available, makes the pick by changing the availability
// and return the response code
play_response board_play(int x, int y, int id){
	play_response resp;
	resp.code = 10;

	cell_info status;
	get_cell_status(&status, x, y);

	if(status.player_id == -1){ //FREE CELL
		
		if(play[id][0] == -1){		//First Pick
			
			printf("FIRST\n");
			
			resp.code = 1;
			
			play[id][0] = x;
			play[id][1] = y;
			
			resp.play1[0]= play[id][0];
			resp.play1[1]= play[id][1];
			strcpy(resp.str_play1, get_board_place_str(x, y));

			change_cell_status(x, y, id, resp.str_play1, GREY);

		} else {					//Second Pick

			char * first_str = get_board_place_str(play[id][0], play[id][1]);
			char * secnd_str = get_board_place_str(x, y);

			resp.play1[0]= play[id][0];
			resp.play1[1]= play[id][1];
			strcpy(resp.str_play1, first_str);

			resp.play2[0]= x;
			resp.play2[1]= y;
			strcpy(resp.str_play2, secnd_str);

			if (strcmp(first_str, secnd_str) == 0){ // Correct
				
				printf("CORRECT!!!\n");

				change_cell_status(play[id][0], play[id][1], id, first_str, BLACK);
				change_cell_status(x, y, id, secnd_str, BLACK);

				n_corrects +=2;
				if (n_corrects == dim_board * dim_board)
					resp.code =3;
				else
					resp.code =2;

			} else {								// Incorrect
				printf("INCORRECT\n");

				resp.code = -2;
				change_cell_status(play[id][0], play[id][1], id, first_str, RED);
				change_cell_status(x, y, id, secnd_str, RED);
			}

			first_str = NULL;
			secnd_str = NULL;	

			play[id][0]= -1;
				
		}
	} else {					//FILLED CELL
		printf("FILLED\n");
		resp.code = 0;
	}
	return resp;
}

// Change the play of player id to be his first pick
void reset_play(int player_id, int x, int y){
	play[player_id][0]= -1;
	reset_cell_status(x,y);
}

// Change the status of the cell (x,y)
void change_cell_status(int x, int y, int id, char str_play[], char color[]){
	int i = linear_conv(x,y);

	pthread_mutex_lock(&mutex_lock);
	cells_info[i].player_id = id;
	strcpy(cells_info[i].str, str_play);
	strcpy(cells_info[i].string_color, color);
	pthread_mutex_unlock(&mutex_lock);

	//printf("	SET -	cell(%d,%d) status = %d\n", x, y, id);
}

// Make every cell available
void reset_cell_status(int x, int y){
	//change_cell_status(x,y,-1,"","");
	
	int i = linear_conv(x, y);

	pthread_mutex_lock(&mutex_lock);
	cells_info[i].player_id = -1;
	strcpy(cells_info[i].str, "");
	strcpy(cells_info[i].string_color, "");
	pthread_mutex_unlock(&mutex_lock);

	//printf("	RESET -	cell(%d,%d) status = %d\n", x, y, cells_info[i].player_id);
}

// Return the status of cell (x,y)
void get_cell_status(cell_info * info, int x, int y){
	
	int i = linear_conv(x,y);

	pthread_mutex_lock(&mutex_lock);
	*info = cells_info[i];
	pthread_mutex_unlock(&mutex_lock);
}
