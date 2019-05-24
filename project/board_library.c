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

int play[100][2];


int play1[2];
int n_corrects;

int linear_conv(int i, int j){
  return j*dim_board+i;
}
char * get_board_place_str(int i, int j){
  return board[linear_conv(i, j)].v;
}

void reset_cell_status(int x, int y){
  int i = linear_conv(x, y); 
  cells_info[i].player_id = -1;
  strcpy(cells_info[i].str, "");
  strcpy(cells_info[i].string_color, "");
}
void reset_play(int player_id, int x, int y){
	play[player_id][0]= -1;
  reset_cell_status(x,y);
}

void init_board(int dim){
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

play_response board_play(int x, int y, int id){
  int i;
  play_response resp;
  resp.code =10;
  
  if(strcmp(get_board_place_str(x, y), "")==0){
    printf("FILLED\n");
    resp.code =0; 
  }else{
    // if the player takes his first guess

    if(play[id][0]== -1){

      //verify that no other player has taken this as his first guess
      
      for(int index = 0; index < 100; index++){
        if(index != id){
          if(play[index][0] == x){
            resp.code = 0;
            return resp;
          }
        }        
      }
        printf("FIRST\n");
        resp.code =1;

        play[id][0]=x;
        play[id][1]=y;

        //put in resp the x and y coordinates of the 1st play
        resp.play1[0]= play[id][0];
        resp.play1[1]= play[id][1];
        
        // put the stringat (x,y) into resp
        strcpy(resp.str_play1, get_board_place_str(x, y));

        //
        i = linear_conv(x,y);
        
        cells_info[i].player_id = id;
        strcpy(cells_info[i].str, resp.str_play1);
        strcpy(cells_info[i].string_color, GREY);
        

      }else{  // second pick(either will be filled, right or wrong)
        // get the first pick string
        char * first_str = get_board_place_str(play[id][0], play[id][1]);
        char * secnd_str = get_board_place_str(x, y);

        // if old == new (1st == 2nd) then it means it was a pick that was already picked
        if ((play[id][0]==x) && (play[id][1]==y)){
          resp.code =0;
          printf("FILLED\n");
        } else {
          
          
          cell_info st;
          get_cell_status(&st, x,y);
          if(st.player_id != -1){
            resp.code = 0;
            printf("FILLED\n");
            
          } else {

            // it is either good or bad
            //play1 is the 1st pick

            resp.play1[0]= play[id][0];
            resp.play1[1]= play[id][1];
            strcpy(resp.str_play1, first_str);

            //play2 is the new pick
            resp.play2[0]= x;
            resp.play2[1]= y;
            strcpy(resp.str_play2, secnd_str);

            if (strcmp(first_str, secnd_str) == 0){ // they are the same
              printf("CORRECT!!!\n");
              //erase the strings
              strcpy(first_str, "");
              strcpy(secnd_str, "");

              i = linear_conv(play[id][0],play[id][1]);
              cells_info[i].player_id = id;
              strcpy(cells_info[i].str, resp.str_play1);
              strcpy(cells_info[i].string_color, BLACK);
              i = linear_conv(x,y);
              cells_info[i].player_id = id;
              strcpy(cells_info[i].str, resp.str_play2);
              strcpy(cells_info[i].string_color, BLACK);


              n_corrects +=2;
              if (n_corrects == dim_board * dim_board)
                resp.code =3;
              else
                resp.code =2;
            }else{
              printf("INCORRECT");

              i = linear_conv(play[id][0],play[id][1]);
              cells_info[i].player_id = id;
              strcpy(cells_info[i].str, resp.str_play1);
              strcpy(cells_info[i].string_color, RED);
              i = linear_conv(x,y);
              cells_info[i].player_id = id;
              strcpy(cells_info[i].str, resp.str_play2);
              strcpy(cells_info[i].string_color, RED);

              resp.code = -2;
            }
          }
          play[id][0]= -1;
        }
      }
    }
  return resp;
}

void get_cell_status(cell_info * info, int x, int y){
  
  int i = linear_conv(x,y);
  *info = cells_info[i];
  
}


/*#include <stdlib.h>
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

int play[100][2];


int play1[2];
int n_corrects;

int linear_conv(int i, int j){
  return j*dim_board+i;
}
char * get_board_place_str(int i, int j){
  return board[linear_conv(i, j)].v;
}

void reset_cell_status(int x, int y){
  int i = linear_conv(x, y); 
  cells_info[i].player_id = -1;
  strcpy(cells_info[i].str, "");
  strcpy(cells_info[i].string_color, "");
}
void reset_play(int player_id, int x, int y){
	play[player_id][0]= -1;
  reset_cell_status(x,y);
}

void init_board(int dim){
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

play_response board_play(int x, int y, int id){
  int i;
  play_response resp;
  resp.code =10;
  cell_info cell_status;
  

  if(play[id][0] == -1){ // ITS THE PLAYER FIRST PICK

    get_cell_status(&cell_status, x,y);
    if(cell_status.player_id != -1){ //it is already picked
      printf("FILLED\n");
      resp.code = 0;
      return resp;
    }

    // this cell is available
    printf("FIRST\n");
    resp.code = 1;

    // change the status of the cell
    i = linear_conv(x,y);
    cells_info[i].player_id = id;
    strcpy(cells_info[i].str, resp.str_play1);
    strcpy(cells_info[i].string_color, GREY);

    play[id][0]=x;
    play[id][1]=y;
    resp.play1[0]= play[id][0];
    resp.play1[1]= play[id][1];

    strcpy(resp.str_play1, get_board_place_str(x, y));

    return resp;
 
  } else { // ITS THE PLAYER SECOND PICK (right or wrong)
      
    get_cell_status(&cell_status, x,y);
    
    if(cell_status.player_id != -1){ //it is already picked
      printf("FILLED\n");
      resp.code = 0;
      return resp;
    }

    // this cell is available
    char * first_str = get_board_place_str(play[id][0], play[id][1]);
    char * secnd_str = get_board_place_str(x, y);

    resp.play1[0]= play[id][0];
    resp.play1[1]= play[id][1];
    strcpy(resp.str_play1, first_str);

    resp.play2[0]= x;
    resp.play2[1]= y;
    strcpy(resp.str_play2, secnd_str);

    if(strcmp(first_str, secnd_str) == 0){
      printf("CORRECT!!!\n");

      strcpy(first_str, "");
      strcpy(secnd_str, "");

      //CHANGE THE CELL STATUS TO TAKEN
      i = linear_conv(play[id][0],play[id][1]);
      cells_info[i].player_id = id;
      strcpy(cells_info[i].str, resp.str_play1);
      strcpy(cells_info[i].string_color, BLACK);

      i = linear_conv(x,y);
      cells_info[i].player_id = id;
      strcpy(cells_info[i].str, resp.str_play2);
      strcpy(cells_info[i].string_color, BLACK);
      
      n_corrects +=2;
      if (n_corrects == dim_board * dim_board)
        resp.code =3;
      else
        resp.code =2;

      return resp;

    } else {
      printf("INCORRECT\n");
      i = linear_conv(play[id][0],play[id][1]);
      cells_info[i].player_id = id;
      strcpy(cells_info[i].str, resp.str_play1);
      strcpy(cells_info[i].string_color, RED);
      i = linear_conv(x,y);
      cells_info[i].player_id = id;
      strcpy(cells_info[i].str, resp.str_play2);
      strcpy(cells_info[i].string_color, RED);

      resp.code = -2;

      play[id][0]= -1;
      return resp;
    }
  }
}


void get_cell_status(cell_info * info, int x, int y){
  
  int i = linear_conv(x,y);
  *info = cells_info[i];
  
}
*/