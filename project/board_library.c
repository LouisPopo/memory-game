#include <stdlib.h>
#include "board_library.h"
#include <stdio.h>
#include <string.h>

int dim_board;
board_place * board;

int play[100][2];


int play1[2];
int n_corrects;

int linear_conv(int i, int j){
  return j*dim_board+i;
}
char * get_board_place_str(int i, int j){
  return board[linear_conv(i, j)].v;
}

void reset_play(int player_id){
	play[player_id][0]= -1;
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

  for( i=0; i < (dim_board*dim_board); i++){
    board[i].v[0] = '\0';
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

      }else{  // second pick(either will be filled, right or wrong)
        // get the first pick string
        char * first_str = get_board_place_str(play[id][0], play[id][1]);
        char * secnd_str = get_board_place_str(x, y);

        // if old == new (1st == 2nd) then it means it was a pick that was already picked
        if ((play[id][0]==x) && (play[id][1]==y)){
          resp.code =0;
          printf("FILLED\n");
        } else{ // it is either good or bad
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

            n_corrects +=2;
            if (n_corrects == dim_board * dim_board)
              resp.code =3;
            else
              resp.code =2;
          }else{
            printf("INCORRECT");

            resp.code = -2;
          }
          play[id][0]= -1;
        }
      }
    }
  return resp;
}
