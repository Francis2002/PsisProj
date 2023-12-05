#include "remote-char.h"
#include "json.h"
#include <ncurses.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>  
#include <stdlib.h>
#include <zmq.h>
#include <string.h>
#include "zhelpers.h"
#include <stdio.h>
#include <time.h>

#define MAX_BODY_SIZE 5
#define TIME_TO_REAPPEAR 5
#define WINDOW_SIZE 30 
#define NOT_EATEN -1
#define Max_Players_Count 26 
#define Max_Roaches_clients 300
#define SERVER_PORT "5555" 
#define SERVER_PORT2 "5554"
#define MAX_JSON_SIZE 10000

//To do , good practice to separate the code between  multiple files , json already done , will need to be done for the collision checks. 
//To do , we need to check snake placement when joining , might coliide if a nother player has not yet moved.

//The display still need to be done.
//
/*char message[100];
sprintf(message, "holla\n");
s_send (publisher, message);*/


typedef struct GameClock  {
    clock_t eatenTime;
    int isEaten;
}GameClock;

typedef struct lizard
{
    int head;
    int client_id;
    int password;
    int lizard_head_x, lizard_head_y;
    int body[1];
    int victory_body[1];
    int body_positions[5][2]; 
    int score;
    int Opposite_To_Last_Direction;
} lizard;

typedef struct roaches
{
    int client_id;
    int roaches_count;
    GameClock  * roach_eaten;
    int *roaches_eaten;
    int *roaches_weights;
    int **roaches_positions;
}roaches;




bool check_lizardHead_colision(int position_X , int position_Y, int head_postions_cache[Max_Players_Count][2])
{
    for(int i = 0 ; i <Max_Players_Count ; ++i)
    {
        if((head_postions_cache[i][0] == position_X) && (head_postions_cache[i][1] == position_Y))
        {
            return TRUE;
        }
    }
    return false;
}

void Repopulate_roaches(WINDOW *my_win, roaches *roaches_info)//Repopulate the roaches in the board 
{
    for (int i = 0; i < Max_Roaches_clients; ++i) 
    {
        if(roaches_info[i].client_id == i)
        {
            for (int j = 0; j < roaches_info[i].roaches_count; ++j) 
            {
                if(roaches_info[i].roach_eaten[j].isEaten == 2)
                {
                    roaches_info[i].roach_eaten[j].isEaten = 0;
                    wmove(my_win, roaches_info[i].roaches_positions[j][0],roaches_info[i].roaches_positions[j][1]);
                    waddch(my_win, roaches_info[i].roaches_weights[j] | A_BOLD);
                    wrefresh(my_win);
                }
            }
        }   
    }
}

void Check_roaches_clock(roaches *roaches_info, int head_postions_cache[Max_Players_Count][2])//Check if enough time has passed for the roach to appear again
{
    for(int i = 0 ; i < Max_Roaches_clients ; ++i)
    {
        if(roaches_info[i].client_id == i)
        {
            for(int j = 0 ; j < roaches_info[i].roaches_count; ++j)
            {
                if(roaches_info[i].roach_eaten[j].isEaten == 1)
                {
                    if((time(NULL) - roaches_info[i].roach_eaten[j].eatenTime) > TIME_TO_REAPPEAR)
                    {
                        roaches_info[i].roach_eaten[j].isEaten = 2;
                        roaches_info[i].roach_eaten[j].eatenTime = NOT_EATEN;
                        int position_x;
                        int position_y;
                        do
                        {
                            position_x = (rand() % (WINDOW_SIZE - 2)) + 1;
                            position_y = (rand() % (WINDOW_SIZE - 2)) + 1;
                        }while(check_lizardHead_colision(position_x,position_y,head_postions_cache));
                        roaches_info[i].roaches_positions[j][0] = position_x;
                        roaches_info[i].roaches_positions[j][1] = position_y;
                    }
                }
            }
        }
    }
}


bool check_lizard_V_lizard_colision(lizard *lizard_info,int client_id, int temp_x,int temp_y)//Check lizard head to head colision
{
    int mean; 
    for(int i = 0 ; i < Max_Players_Count;++i)
    {
        if(i == client_id)
        {
            continue;
        }
        if(lizard_info[i].client_id == i)
        {
            if((lizard_info[i].lizard_head_x == temp_x) && (lizard_info[i].lizard_head_y == temp_y))
            {
                mean = (lizard_info[i].score + lizard_info[client_id].score )/2;
                lizard_info[i].score = mean;
                lizard_info[client_id].score = mean;
                return TRUE;
            }
        }
    }
    return false;
}

int check_score_upadte(lizard lizard_info, roaches *roaches_info, int total_roaches)//Check lizard vs roaches colision , update score
{
    int roaches_checked = 0;
    int score = 0 ;
    for(int i =0 ; i < Max_Roaches_clients ; ++i)
    {
        if(roaches_checked == total_roaches)
        {
            return score;
        }
        if(roaches_info[i].client_id == i)
        {
            for(int j =0 ; j < roaches_info[i].roaches_count ; ++j)
            {
                if((roaches_info[i].roaches_positions[j][0] == lizard_info.lizard_head_x) && (roaches_info[i].roaches_positions[j][1] == lizard_info.lizard_head_y))
                {
                    roaches_info[i].roach_eaten[j].eatenTime = time(NULL);
                    roaches_info[i].roach_eaten[j].isEaten = 1;
                    score  = score + (roaches_info[i].roaches_weights[j]-48);
                }
                roaches_checked++;
            }
        }
    }
    return score;
}

void new_position(int *x, int *y, direction_t direction){//Find new head position.
        switch (direction)
        {
        case UP:
            (*x) --;
            if(*x ==0)
                *x = 2;
            break;
        case DOWN:
            (*x) ++;
            if(*x ==WINDOW_SIZE-1)
                *x = WINDOW_SIZE-3;
            break;
        case LEFT:
            (*y) --;
            if(*y ==0)
                *y = 2;
            break;
        case RIGHT:
            (*y) ++;
            if(*y ==WINDOW_SIZE-1)
                *y = WINDOW_SIZE-3;
            break;
        default:
            break;
        }
}


void shift_array_down(lizard *lizard_info, int size) {//Function to shift the body postions of the lizard.
    for (int i = size - 1; i > 0; i--) {
        for (int j = 0; j < 2; j++) {
            lizard_info->body_positions[i][j] = lizard_info->body_positions[i - 1][j];
        }
    }
    
    lizard_info->body_positions[0][0] = lizard_info->lizard_head_x; 
    lizard_info->body_positions[0][1] = lizard_info->lizard_head_y;
}

int Find_Opposite_To_Last_Direction(int direction) {//Find the direction opposite to the last move (can´t eat own body).

    int opposite_direction = -1;
    switch (direction) {
        case UP:
            opposite_direction = DOWN;
            break;
        case DOWN:
            opposite_direction = UP;
            break;
        case LEFT:
            opposite_direction = RIGHT;
            break;
        case RIGHT:
            opposite_direction = LEFT;
            break;
        default:

            break;
    }

    return opposite_direction;
}


int main()
{	
    int total_amount_of_roaches = 0;
    srand(time(NULL));
    lizard lizard_info[Max_Players_Count];
    roaches roaches_info[Max_Roaches_clients];
    for(int i = 0 ; i < Max_Roaches_clients; i++)
    {
        roaches_info[i].client_id = -1;
    }
    int head_postions_cache[Max_Players_Count][2]; //Sort of like a dict so that there is easy comparisons;
    int number_of_lizards = 0;
    int Number_of_roaches_clients = 0 ;

	void* context = zmq_ctx_new();
    void* responder = zmq_socket(context, ZMQ_REP);
    zmq_bind(responder, "tcp://*:" SERVER_PORT); 

    void* context2 = zmq_ctx_new();
    void* publisher = zmq_socket(context2, ZMQ_PUB);
    zmq_bind(publisher, "tcp://*:" SERVER_PORT2); 

	initscr();		    	
	cbreak();				
    keypad(stdscr, TRUE);   
	noecho();			    

    /* creates a window and draws a border */
    WINDOW * my_win = newwin(WINDOW_SIZE, WINDOW_SIZE, 0, 0);
    box(my_win, 0 , 0);	
	wrefresh(my_win);

    int pos_x;
    int pos_y;


    direction_t  direction;
    while (1)
    {
        char received_data[MAX_JSON_SIZE];
        remote_char_t message_received;
        zmq_recv(responder, &received_data, sizeof(received_data), 0);
        deserialize(received_data, &message_received);
        Check_roaches_clock(roaches_info,head_postions_cache);
        Repopulate_roaches(my_win,roaches_info);
        if(message_received.msg_type == -1)//Player disconnection
        {
            int client_position = message_received.client_id;
            lizard_info[client_position].client_id = -1;
            lizard_info[client_position].score = 0;

            wmove(my_win, lizard_info[client_position].lizard_head_x, lizard_info[client_position].lizard_head_y);
            waddch(my_win,' ');

            for(int i = 0; i < MAX_BODY_SIZE; ++i)//Removing snake
            {

                wmove(my_win, lizard_info[client_position].body_positions[i][0], lizard_info[client_position].body_positions[i][1]);
                waddch(my_win,' ');
            }
            wrefresh(my_win);
            number_of_lizards--;  

            char acknowledgment[] = "You have been disconnected";
            zmq_send(responder, acknowledgment, strlen(acknowledgment), 0);
        }
        if(message_received.msg_type == 0)// Player connection
        {
            if(number_of_lizards < Max_Players_Count) 
            {
                int client_position = message_received.client_id;
                if(lizard_info[client_position].client_id != message_received.client_id) //Check if client id , is already in use
                {
                    lizard_info[client_position].head = message_received.head;
                    lizard_info[client_position].client_id = client_position;
                    lizard_info[client_position].lizard_head_x = WINDOW_SIZE/2; // need to be changed to not overlap snakes on spawning
                    lizard_info[client_position].lizard_head_y = WINDOW_SIZE/2;
                    int default_body = 46;
                    int victory_body = 42;
                    lizard_info[client_position].Opposite_To_Last_Direction = -1;
                    lizard_info[client_position].body[0] = default_body;
                    lizard_info[client_position].victory_body[0] = victory_body;
                    //Check against lizard 
                    for(int i = 0; i < MAX_BODY_SIZE; ++i) //setting inicial body positions
                    {
                        lizard_info[client_position].body_positions[i][0] = lizard_info[client_position].lizard_head_x;
                        lizard_info[client_position].body_positions[i][1]= (lizard_info[client_position].lizard_head_y - (i+1));
                    }
                    head_postions_cache[client_position][0] = lizard_info[client_position].lizard_head_x;
                    head_postions_cache[client_position][1] = lizard_info[client_position].lizard_head_y;

                    wmove(my_win, lizard_info[client_position].lizard_head_x, lizard_info[client_position].lizard_head_y);
                    waddch(my_win,lizard_info[client_position].head| A_BOLD);

                    for(int i = 0; i < MAX_BODY_SIZE; ++i) //Printing initial snake state
                    {

                        wmove(my_win, lizard_info[client_position].body_positions[i][0], lizard_info[client_position].body_positions[i][1]);
                        waddch(my_win,lizard_info[client_position].body[0]| A_BOLD);
                    }
                    wrefresh(my_win);
                    number_of_lizards++;
                    int password[8];
                    for(int i = 0; i< 8 ;++i)
                    {
                        password[i] = (rand() % 10);
                    }
                    char concatenated_password[9];
                    strcpy(concatenated_password, ""); 
                    for (int i = 0; i < 8; ++i) {
                        char temp[2];
                        snprintf(temp, sizeof(temp), "%d", password[i]);
                        strcat(concatenated_password, temp);
                    }
                    lizard_info[client_position].password = atoi(concatenated_password);
                    char acknowledgment[100];
                    sprintf(acknowledgment, "Succefully connected:%s\n",concatenated_password);
                    s_send (publisher, acknowledgment);

                    zmq_send(responder, acknowledgment, strlen(acknowledgment), 0);
                }else
                {
                    char acknowledgment[] = "Invalid id number";
                    zmq_send(responder, acknowledgment, strlen(acknowledgment), 0);
                }
            }else
            {
                char acknowledgment[] = "Server at max Capacity";
                zmq_send(responder, acknowledgment, strlen(acknowledgment), 0);
            }
            
        }
        if(message_received.msg_type == 2) // Roach connection
        {
            if(total_amount_of_roaches + message_received.roaches_amount < Max_Roaches_clients)
            {
                int client_position = message_received.client_id;
                if(roaches_info[client_position].client_id != message_received.client_id) //Check if client id , is already in use
                {
                    roaches_info[client_position].client_id = message_received.client_id;
                    roaches_info[client_position].roaches_count = message_received.roaches_amount;
                    roaches_info[client_position].roach_eaten = malloc(roaches_info[client_position].roaches_count * sizeof(GameClock)); //Allocated memory for the clock counter
                    if (roaches_info[client_position].roach_eaten == NULL) {//memory fail safe
                        printf("Memory allocation failed!\n");
                        return 1; 
                    }
                    for(int j  = 0 ;j < roaches_info[client_position].roaches_count; ++j )// Store weights
                    {
                        roaches_info[client_position].roach_eaten[j].isEaten = 0;
                        roaches_info[client_position].roach_eaten[j].eatenTime = NOT_EATEN; 
                    }
                    roaches_info[client_position].roaches_weights = malloc(roaches_info[client_position].roaches_count * sizeof(int));
                    if (roaches_info[client_position].roaches_weights == NULL) {//memory fail safe
                        printf("Memory allocation failed!\n");
                        return 1; 
                    }
                    for(int j  = 0 ;j < roaches_info[client_position].roaches_count; ++j )// Store weights
                    {
                        roaches_info[client_position].roaches_weights[j] = message_received.roaches_weights[j];
                    }
                    roaches_info[client_position].roaches_positions = (int **)malloc(roaches_info[client_position].roaches_count * sizeof(int *));
                    if (roaches_info[client_position].roaches_positions == NULL) {//memory fail safe
                        printf("Memory allocation failed!\n");
                        return 1; 
                    }
                    for (int i = 0; i < roaches_info[client_position].roaches_count; i++) { //Allocate and assign random inicitial position to the roaches
                        roaches_info[client_position].roaches_positions[i] = (int *)malloc(2 * sizeof(int));
                        if (roaches_info[client_position].roaches_positions[i] == NULL) {
                            printf("Memory allocation failed!\n");
                            return 1; 
                        }
                        for (int j = 0; j < 2; j++) {
                            int position_x;
                            int position_y;
                            do
                            {
                                position_x = (rand() % (WINDOW_SIZE - 2)) + 1;
                                position_y = (rand() % (WINDOW_SIZE - 2)) + 1;
                            }while(check_lizardHead_colision(position_x,position_y,head_postions_cache));
                            roaches_info[client_position].roaches_positions[i][j] = (rand() % (WINDOW_SIZE - 2)) + 1; 
                        }
                    }
                    for(int i = 0; i < roaches_info[client_position].roaches_count; ++i) // Print roaches initial position.
                        {
                            wmove(my_win, roaches_info[client_position].roaches_positions[i][0], roaches_info[client_position].roaches_positions[i][1]);
                            waddch(my_win,roaches_info[client_position].roaches_weights[i]| A_BOLD);
                        }
                    wrefresh(my_win);
                    total_amount_of_roaches = total_amount_of_roaches + message_received.roaches_amount;
                    Number_of_roaches_clients++;
                    char acknowledgment[] = "Succefully connected";
                    zmq_send(responder, acknowledgment, strlen(acknowledgment), 0);
                }
                else
                {
                    char acknowledgment[] = "Invalid id number";
                    zmq_send(responder, acknowledgment, strlen(acknowledgment), 0);
                }
                
            }
            else 
            {
                char message[100];
                sprintf(message, "Maximum amount of roaches reached:%d\n",(Max_Roaches_clients-total_amount_of_roaches));
                zmq_send(responder, message, strlen(message), 0);
            }
            

        }
        if(message_received.msg_type == 1) // Player movement
        {
            int client_id = message_received.client_id;
            if(lizard_info[client_id].client_id == client_id && lizard_info[client_id].password == message_received.password)//Just in case someone tries to send a player movement without registering
            {
                direction = message_received.direction;
                
                if(direction == lizard_info[client_id].Opposite_To_Last_Direction)//You can´t "eat" your own body;
                {
                    wrefresh(my_win);
                    char acknowledgment[] = "Invalid Move";
                    zmq_send(responder, acknowledgment, strlen(acknowledgment), 0);
                    continue;
                }
                int temp_x = lizard_info[client_id].lizard_head_x;
                int temp_y = lizard_info[client_id].lizard_head_y;
                new_position(&temp_x, &temp_y, direction);
                if(!check_lizard_V_lizard_colision(lizard_info,client_id,temp_x,temp_y))//Check if it colides with another lizard
                {
                    lizard_info[client_id].Opposite_To_Last_Direction = Find_Opposite_To_Last_Direction(direction);
                    pos_x = lizard_info[client_id].body_positions[4][0];
                    pos_y = lizard_info[client_id].body_positions[4][1];
                    wmove(my_win, pos_x, pos_y);
                    waddch(my_win,' ');

                    shift_array_down(&(lizard_info[client_id]), MAX_BODY_SIZE); // We can change shift the array down , to update body positions  
                    

                    
                    new_position(&(lizard_info[client_id].lizard_head_x), &(lizard_info[client_id].lizard_head_y), direction);// Calculate new positions and print
                    head_postions_cache[client_id][0] = lizard_info[client_id].lizard_head_x;
                    head_postions_cache[client_id][1] = lizard_info[client_id].lizard_head_y;
                    lizard_info[client_id].score = lizard_info[client_id].score + check_score_upadte(lizard_info[client_id],roaches_info,total_amount_of_roaches);
                    wmove(my_win, lizard_info[client_id].lizard_head_x, lizard_info[client_id].lizard_head_y);
                    waddch(my_win,lizard_info[client_id].head| A_BOLD);


                    wmove(my_win, lizard_info[client_id].body_positions[0][0], lizard_info[client_id].body_positions[0][1]);
                    if(lizard_info[client_id].score >= 50)//Check if won
                    {
                        waddch(my_win,lizard_info[client_id].victory_body[0]| A_BOLD);
                    }else
                    {
                        waddch(my_win,lizard_info[client_id].body[0]| A_BOLD);
                    }
                    wrefresh(my_win);
                    char acknowledgment[100];
                    char message[100]; 
                    sprintf(message, ":%d\n",lizard_info[client_id].score);
                    s_send (publisher, message);
                    sprintf(acknowledgment, "Your score is:%d\n",lizard_info[client_id].score);
                    zmq_send(responder, acknowledgment, strlen(acknowledgment), 0);
                }else
                {
                    wrefresh(my_win);
                    char acknowledgment[] = "Invalid Move";
                    zmq_send(responder, acknowledgment, strlen(acknowledgment), 0);
                }
            }
            wrefresh(my_win);

        }
        if(message_received.msg_type == 3) //Roach movement
        {
            int client_position = message_received.client_id;
            if(roaches_info[client_position].client_id == client_position)//Just in case someone tries to send a player movement without registering
            {
                //Todo check collision with a snakes head
                for(int i  = 0; i< roaches_info[client_position].roaches_count; i++)
                {
                    if(message_received.roaches_directions[i] != -1)
                    {
                        if((roaches_info[client_position].roach_eaten[i].isEaten != 1) && ((time(NULL) -roaches_info[client_position].roach_eaten[i].eatenTime) > TIME_TO_REAPPEAR || (roaches_info[client_position].roach_eaten[i].eatenTime == NOT_EATEN)))
                        {
                            int temp_pos_x = roaches_info[client_position].roaches_positions[i][0];
                            int temp_pos_y = roaches_info[client_position].roaches_positions[i][1];
                            new_position(&(temp_pos_x), &(temp_pos_y), message_received.roaches_directions[i]);
                            if(!check_lizardHead_colision(temp_pos_x,temp_pos_y,head_postions_cache))
                            {
                                pos_x = roaches_info[client_position].roaches_positions[i][0];
                                pos_y = roaches_info[client_position].roaches_positions[i][1];
                                wmove(my_win, pos_x, pos_y);
                                waddch(my_win,' ');

                                new_position(&(roaches_info[client_position].roaches_positions[i][0]), &(roaches_info[client_position].roaches_positions[i][1]), message_received.roaches_directions[i]);
                                wmove(my_win, roaches_info[client_position].roaches_positions[i][0], roaches_info[client_position].roaches_positions[i][1]);
                                waddch(my_win,roaches_info[client_position].roaches_weights[i]| A_BOLD);
                                wrefresh(my_win);
                            }
                        }
                    } 
                }
                char ack[] = "Mexeu";
                zmq_send(responder, ack, strlen(ack), 0);
            }
        }		
    }
    zmq_close(responder);
    zmq_ctx_destroy(context);
  	endwin();			/* End curses mode		  */

	return 0;
}