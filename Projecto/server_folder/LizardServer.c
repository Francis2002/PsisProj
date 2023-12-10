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
#define Max_Roaches_clients (WINDOW_SIZE*WINDOW_SIZE)/3
#define SERVER_PORT "5555" 
#define SERVER_PORT2 "5554"
#define MAX_JSON_SIZE 10000

//To do , good practice to separate the code between  multiple files , json already done , will need to be done for the collision checks. 
//To do , we need to check snake placement when joining , might coliide if a nother player has not yet moved.
//TODO: erro de comer roaches infinitas qd encostadas a parede
//TODO: alterar direcao do corpo do lizard
//TODO: alterar aquela merda do char ja nao vir do client
//TODO: leaderboard?

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
    int password;
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

void Repopulate_roaches(WINDOW *my_win, roaches *roaches_info, void* publisher)//Repopulate the roaches in the board 
{
    changed_coordinates coordinates_array[1];
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
                    coordinates_array[0].x = roaches_info[i].roaches_positions[j][0];
                    coordinates_array[0].y = roaches_info[i].roaches_positions[j][1];
                    coordinates_array[0].new_char = roaches_info[i].roaches_weights[j];

                    char *json_data = serialize_coordinates(coordinates_array, 1, 0, 1);
                    s_send (publisher, json_data);
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

int check_score_update(lizard lizard_info, roaches *roaches_info, int total_roaches)//Check lizard vs roaches colision , update score
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
                if((roaches_info[i].roaches_positions[j][0] == lizard_info.lizard_head_x) && (roaches_info[i].roaches_positions[j][1] == lizard_info.lizard_head_y) && roaches_info[i].roach_eaten[j].isEaten != 1)
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
                *x = 1;
            break;
        case DOWN:
            (*x) ++;
            if(*x == WINDOW_SIZE-1)
                *x = WINDOW_SIZE-2;
            break;
        case LEFT:
            (*y) --;
            if(*y ==0)
                *y = 1;
            break;
        case RIGHT:
            (*y) ++;
            if(*y ==WINDOW_SIZE-1)
                *y = WINDOW_SIZE-2;
            break;
        default:
            break;
        }
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

void new_body_positions(lizard *lizard_info) 
{//Function to calculate the body postions of the lizard.
    int temp_direction  = Find_Opposite_To_Last_Direction(lizard_info->Opposite_To_Last_Direction);

    for (int i = 0; i < MAX_BODY_SIZE; i++) {
            switch (temp_direction) {
            case UP:
                lizard_info->body_positions[i][0] = lizard_info->lizard_head_x + (i+1) ;
                lizard_info->body_positions[i][1] = lizard_info->lizard_head_y;
                break;
            case DOWN:
                lizard_info->body_positions[i][0] = lizard_info->lizard_head_x - (i+1) ;
                lizard_info->body_positions[i][1] = lizard_info->lizard_head_y;
                break;
            case LEFT:
                lizard_info->body_positions[i][0] = lizard_info->lizard_head_x;
                lizard_info->body_positions[i][1] = lizard_info->lizard_head_y + (i+1) ;
                break;
            case RIGHT:
                lizard_info->body_positions[i][0] = lizard_info->lizard_head_x;
                lizard_info->body_positions[i][1] = lizard_info->lizard_head_y - (i+1);
                break;
            default:

                break;
        }    
    }
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
    //int Number_of_roaches_clients = 0 ;

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
        Repopulate_roaches(my_win,roaches_info, publisher);
        //forward message to all subscribers
        if(message_received.msg_type == -1)//Player disconnection
        {
            int client_position = message_received.client_id;
            if(lizard_info[client_position].client_id == client_position && lizard_info[client_position].password == message_received.password)
            {
                lizard_info[client_position].client_id = -1;//Set clien_id to null.
                lizard_info[client_position].score = 0;//Reset score.

                wmove(my_win, lizard_info[client_position].lizard_head_x, lizard_info[client_position].lizard_head_y);
                waddch(my_win,' ');

                for(int i = 0; i < MAX_BODY_SIZE; ++i)//Removing snake
                {

                    if(!check_lizard_V_lizard_colision(lizard_info,client_position,lizard_info[client_position].body_positions[i][0], lizard_info[client_position].body_positions[i][1]) &&
                            lizard_info[client_position].body_positions[i][0] > 0 && lizard_info[client_position].body_positions[i][0] < WINDOW_SIZE - 1 &&
                            lizard_info[client_position].body_positions[i][1] > 0 && lizard_info[client_position].body_positions[i][1] < WINDOW_SIZE - 1)//Only print if body is not over another lizard head , and is inside the board.
                        {
                            wmove(my_win, lizard_info[client_position].body_positions[i][0], lizard_info[client_position].body_positions[i][1]);
                            waddch(my_win,' ');
                        }
                }
                wrefresh(my_win);
                number_of_lizards--;  

                char acknowledgment[] = "You have been disconnected";
                zmq_send(responder, acknowledgment, strlen(acknowledgment), 0);

                changed_coordinates coordinates_array[6];
                for (int i = 0; i < 5; i++)
                {
                    coordinates_array[i].x = lizard_info[client_position].body_positions[i][0];
                    coordinates_array[i].y = lizard_info[client_position].body_positions[i][1];
                    coordinates_array[i].new_char = ' ';
                }
                coordinates_array[MAX_BODY_SIZE].x = lizard_info[client_position].lizard_head_x;
                coordinates_array[MAX_BODY_SIZE].y = lizard_info[client_position].lizard_head_y;
                coordinates_array[MAX_BODY_SIZE].new_char = ' ';
                int counter = 0 ;
                int size = 5 ; 
                int upper_limit = 6;
                while(counter < upper_limit)//Do five positions at a given time , due to s_send buffer limit.
                {
                    if( upper_limit-counter < 5)
                    {
                        size = upper_limit-counter;
                    }
                    char *json_data = serialize_coordinates(coordinates_array, size, counter, counter+size);
                    s_send (publisher, json_data);
                    counter  = counter + size;
                }
            }else
            {
                char acknowledgment[] = "Nice try hacker";
                zmq_send(responder, acknowledgment, strlen(acknowledgment), 0);
            }
            
        }
        if(message_received.msg_type == 0)// Player connection
        {
            if(number_of_lizards < Max_Players_Count) 
            {
                int client_position = message_received.client_id;
                if(lizard_info[client_position].client_id != message_received.client_id) //Check if client id , is already in use.
                {
                    lizard_info[client_position].head = message_received.head;
                    lizard_info[client_position].client_id = client_position;

                    int maybe_position_x; 
                    int maybe_position_y;
                    do 
                    {
                        maybe_position_x = (rand() % (WINDOW_SIZE - 2)) + 1;
                        maybe_position_y = (rand() % (WINDOW_SIZE - 2)) + 1;
                    }while(check_lizard_V_lizard_colision(lizard_info,client_position,maybe_position_x,maybe_position_y));//Try positions until find one which is valid.

                    lizard_info[client_position].lizard_head_x = maybe_position_x;
                    lizard_info[client_position].lizard_head_y = maybe_position_y;

                    int default_body = 46;
                    int victory_body = 42;

                    lizard_info[client_position].Opposite_To_Last_Direction = -1;//Not needed anymore.

                    lizard_info[client_position].body[0] = default_body;
                    lizard_info[client_position].victory_body[0] = victory_body;

                    for(int i = 0; i < MAX_BODY_SIZE; ++i) //setting inicial body positions , allways start with the snake to the right.
                    {
                        lizard_info[client_position].body_positions[i][0] = lizard_info[client_position].lizard_head_x;
                        lizard_info[client_position].body_positions[i][1]= (lizard_info[client_position].lizard_head_y - (i+1));
                    }

                    head_postions_cache[client_position][0] = lizard_info[client_position].lizard_head_x;//Cache head positions.
                    head_postions_cache[client_position][1] = lizard_info[client_position].lizard_head_y;

                    wmove(my_win, lizard_info[client_position].lizard_head_x, lizard_info[client_position].lizard_head_y);//Printing initial snake head.
                    waddch(my_win,lizard_info[client_position].head| A_BOLD);

                    for(int i = 0; i < MAX_BODY_SIZE; ++i)//Printing initial snake body.
                    {
                        if(!check_lizard_V_lizard_colision(lizard_info,client_position,lizard_info[client_position].body_positions[i][0], lizard_info[client_position].body_positions[i][1]) &&
                            lizard_info[client_position].body_positions[i][0] > 0 && lizard_info[client_position].body_positions[i][0] < WINDOW_SIZE - 1 &&
                            lizard_info[client_position].body_positions[i][1] > 0 && lizard_info[client_position].body_positions[i][1] < WINDOW_SIZE - 1)//Only print if body is not over another lizard head , and is inside the board.
                        {
                            wmove(my_win, lizard_info[client_position].body_positions[i][0], lizard_info[client_position].body_positions[i][1]);
                            waddch(my_win,lizard_info[client_position].body[0]| A_BOLD);
                        }
                    }

                    wrefresh(my_win);
                    number_of_lizards++;
                    int password[8];
                    for(int i = 0; i< 8 ;++i)//Create an equivalent of an SSL key.
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
                    //s_send (publisher, acknowledgment);
                    zmq_send(responder, acknowledgment, strlen(acknowledgment), 0);

                    changed_coordinates coordinates_array[6];
                    for (int i = 0; i < 5; i++)
                    {
                        coordinates_array[i].x = lizard_info[client_position].body_positions[i][0];
                        coordinates_array[i].y = lizard_info[client_position].body_positions[i][1];
                        coordinates_array[i].new_char = lizard_info[client_position].body[0];
                    }
                    coordinates_array[MAX_BODY_SIZE].x = lizard_info[client_position].lizard_head_x;
                    coordinates_array[MAX_BODY_SIZE].y = lizard_info[client_position].lizard_head_y;
                    coordinates_array[MAX_BODY_SIZE].new_char = lizard_info[client_position].head;//Store new positions.

                    int counter = 0 ;
                    int size = 5 ; 
                    int upper_limit = 6;
                    while(counter < upper_limit)//Do five positions at a given time , due to s_send buffer limit. 
                    {
                        if( upper_limit-counter < 5)
                        {
                            size = upper_limit-counter;
                        }
                        char *json_data = serialize_coordinates(coordinates_array, size, counter, counter+size);
                        s_send (publisher, json_data);
                        counter  = counter + size;
                    }

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
                    //Number_of_roaches_clients++;

                    int password[8];
                    for(int i = 0; i< 8 ;++i)//Create an equivalent of an SSL key.
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
                    roaches_info[client_position].password = atoi(concatenated_password);
                    char acknowledgment[100];
                    sprintf(acknowledgment, "Succefully connected:%s\n",concatenated_password);
                    zmq_send(responder, acknowledgment, strlen(acknowledgment), 0);

                    //create changed_coordinates array with roaches positions
                    changed_coordinates coordinates_array[roaches_info[client_position].roaches_count];
                    for (int i = 0; i < roaches_info[client_position].roaches_count; i++)
                    {
                        coordinates_array[i].x = roaches_info[client_position].roaches_positions[i][0];
                        coordinates_array[i].y = roaches_info[client_position].roaches_positions[i][1];
                        coordinates_array[i].new_char = roaches_info[client_position].roaches_weights[i];
                    }
                    int counter = 0 ; 
                    while(counter <  roaches_info[client_position].roaches_count)
                    {
                        char *json_data = serialize_coordinates(coordinates_array, 5, counter, counter+5);
                        s_send (publisher, json_data);
                        counter  = counter + 5;
                    } 
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
            if(lizard_info[client_id].client_id == client_id && lizard_info[client_id].password == message_received.password)//Just in case someone tries to send a player movement without registering , or tries to hack a movement.
            {
                direction = message_received.direction;
                int temp_x = lizard_info[client_id].lizard_head_x;
                int temp_y = lizard_info[client_id].lizard_head_y;
                new_position(&temp_x, &temp_y, direction);// Calculate new hypothetical position.
                if(!check_lizard_V_lizard_colision(lizard_info,client_id,temp_x,temp_y))//Check if it colides with another lizard.
                {
                    lizard_info[client_id].Opposite_To_Last_Direction = Find_Opposite_To_Last_Direction(direction);//Not needed anymore , due to movement changes.

                    changed_coordinates coordinates_array[(MAX_BODY_SIZE+1)*2];//We need to store old and new positions.

                    coordinates_array[MAX_BODY_SIZE].x = lizard_info[client_id].lizard_head_y;
                    coordinates_array[MAX_BODY_SIZE].y = lizard_info[client_id].lizard_head_x;
                    coordinates_array[MAX_BODY_SIZE].new_char = ' ';//Store old positions.

                    new_position(&(lizard_info[client_id].lizard_head_x), &(lizard_info[client_id].lizard_head_y), direction);//Calcualte new head position.(Operation not needed , need to fix)

                    for (int i = 0; i < MAX_BODY_SIZE; i++)//Only erase if the body is not over positioned in another lizards head, and f it is not on the border. 
                    {
                        if(!check_lizard_V_lizard_colision(lizard_info,client_id,lizard_info[client_id].body_positions[i][0], lizard_info[client_id].body_positions[i][1]) &&
                            lizard_info[client_id].body_positions[i][0] > 0 && lizard_info[client_id].body_positions[i][0] < WINDOW_SIZE - 1 &&
                            lizard_info[client_id].body_positions[i][1] > 0 && lizard_info[client_id].body_positions[i][1] < WINDOW_SIZE - 1)
                        {
                            wmove(my_win, lizard_info[client_id].body_positions[i][0], lizard_info[client_id].body_positions[i][1]);
                            waddch(my_win,' ');
                        }
                    }
                    for (int i = 0; i < MAX_BODY_SIZE; i++)//Store old body positions.
                    {
                        
                        coordinates_array[i].x = lizard_info[client_id].body_positions[i][0];
                        coordinates_array[i].y = lizard_info[client_id].body_positions[i][1];
                        coordinates_array[i].new_char = ' ';
                    }
                    new_body_positions(&(lizard_info[client_id]));///Calculate new body positions.
                    
                    head_postions_cache[client_id][0] = lizard_info[client_id].lizard_head_x;//Cache head positions.
                    head_postions_cache[client_id][1] = lizard_info[client_id].lizard_head_y;

                    lizard_info[client_id].score = lizard_info[client_id].score + check_score_update(lizard_info[client_id],roaches_info,total_amount_of_roaches);//Update Score.

                    wmove(my_win, lizard_info[client_id].lizard_head_x, lizard_info[client_id].lizard_head_y);//Print lizard head to the board.
                    waddch(my_win,lizard_info[client_id].head| A_BOLD);

                    int helper = 6;
                    for (int i = 0; i < MAX_BODY_SIZE; i++)//Print body.
                    {
                        wmove(my_win, lizard_info[client_id].body_positions[i][0], lizard_info[client_id].body_positions[i][1]);
                        if(!check_lizard_V_lizard_colision(lizard_info,client_id,lizard_info[client_id].body_positions[i][0], lizard_info[client_id].body_positions[i][1]) &&
                            lizard_info[client_id].body_positions[i][0] > 0 && lizard_info[client_id].body_positions[i][0] < WINDOW_SIZE - 1 &&
                            lizard_info[client_id].body_positions[i][1] > 0 && lizard_info[client_id].body_positions[i][1] < WINDOW_SIZE - 1)//Only print if body is not over another lizard head , and is inside the board.
                        {
                            coordinates_array[helper].x = lizard_info[client_id].body_positions[i][0];
                            coordinates_array[helper].y = lizard_info[client_id].body_positions[i][1];//Store new positions.
                            if(lizard_info[client_id].score >= 50)//Check if won.
                            {
                                coordinates_array[helper].new_char  = lizard_info[client_id].victory_body[0];
                                waddch(my_win,lizard_info[client_id].victory_body[0]| A_BOLD);
                            }else
                            {
                                coordinates_array[helper].new_char  = lizard_info[client_id].body[0];
                                waddch(my_win,lizard_info[client_id].body[0]| A_BOLD);
                            }
                            helper++;  
                        }   
                    }

                    coordinates_array[helper].x = lizard_info[client_id].lizard_head_x;
                    coordinates_array[helper].y = lizard_info[client_id].lizard_head_y;
                    coordinates_array[helper].new_char = lizard_info[client_id].head;

                    wrefresh(my_win);
                    char acknowledgment[100];

                    sprintf(acknowledgment, "Your score is:%d\n",lizard_info[client_id].score);
                    zmq_send(responder, acknowledgment, strlen(acknowledgment), 0);
                    int counter = 0 ;
                    int size = 5 ;
                    int upper_limit = helper+1;
                    while(counter < upper_limit)//Do five positions at a given time , due to s_send buffer limit.
                    {           
                        if( upper_limit-counter < 5)
                        {
                            size = upper_limit-counter;
                        }
                        char *json_data = serialize_coordinates(coordinates_array, size, counter, counter+size);
                        s_send (publisher, json_data);
                        counter  = counter + size;
                    } 

                }else
                {
                    wrefresh(my_win);
                    char acknowledgment[] = "Invalid Move";
                    zmq_send(responder, acknowledgment, strlen(acknowledgment), 0);
                }
            }else
            {
                char acknowledgment[] = "Nice try hacker";
                zmq_send(responder, acknowledgment, strlen(acknowledgment), 0);
            }
            wrefresh(my_win);

        }
        if(message_received.msg_type == 3) //Roach movement
        {
            int client_position = message_received.client_id;
            if(roaches_info[client_position].client_id == client_position && roaches_info[client_position].password == message_received.password)//Just in case someone tries to send a roach movement without registering.
            {

                changed_coordinates* coordinates_array;
                coordinates_array = malloc((roaches_info[client_position].roaches_count*2) * sizeof(changed_coordinates));//Allocate memory for the old positions and for the new ones.
                if (coordinates_array == NULL) //Memory fail safe
                {
                    endwin();	
                    printf("Memory allocation failed!\n");
                    exit(1);
                }
                int moved_roaches_times_2 = 0;

                for(int i  = 0; i< roaches_info[client_position].roaches_count; i++)
                {
                    if(message_received.roaches_directions[i] != -1)//Check if the roach was chosen to move.
                    {
                        if((roaches_info[client_position].roach_eaten[i].isEaten != 1) && ((time(NULL) -roaches_info[client_position].roach_eaten[i].eatenTime) > TIME_TO_REAPPEAR || (roaches_info[client_position].roach_eaten[i].eatenTime == NOT_EATEN)))
                        {   
                            //If entered means the roach is not eaten , or if eaten enough time has passed for it to move.

                            int temp_pos_x = roaches_info[client_position].roaches_positions[i][0];
                            int temp_pos_y = roaches_info[client_position].roaches_positions[i][1];

                            new_position(&(temp_pos_x), &(temp_pos_y), message_received.roaches_directions[i]);//Calculate new hypothetical roach position

                            if(!check_lizardHead_colision(temp_pos_x,temp_pos_y,head_postions_cache))//Check against any lizards head.
                            {
                                //If it entered here , roach is allowed to move
                                pos_x = roaches_info[client_position].roaches_positions[i][0];
                                pos_y = roaches_info[client_position].roaches_positions[i][1];
                                wmove(my_win, pos_x, pos_y);
                                waddch(my_win,' '); // Erase old roach position.

                                coordinates_array[moved_roaches_times_2].x = pos_x;
                                coordinates_array[moved_roaches_times_2].y = pos_y;
                                coordinates_array[moved_roaches_times_2].new_char = ' '; //Store old positions 

                                new_position(&(roaches_info[client_position].roaches_positions[i][0]), &(roaches_info[client_position].roaches_positions[i][1]), message_received.roaches_directions[i]);// calculate new position for raoch , this is not necessary due to prevous calculation , butfor now ok.
                                wmove(my_win, roaches_info[client_position].roaches_positions[i][0], roaches_info[client_position].roaches_positions[i][1]);//Move roach
                                waddch(my_win,roaches_info[client_position].roaches_weights[i]| A_BOLD);
                                wrefresh(my_win);

                                moved_roaches_times_2 ++ ;

                                coordinates_array[moved_roaches_times_2].x = roaches_info[client_position].roaches_positions[i][0];
                                coordinates_array[moved_roaches_times_2].y = roaches_info[client_position].roaches_positions[i][1];
                                coordinates_array[moved_roaches_times_2].new_char = roaches_info[client_position].roaches_weights[i];//Store new roach positions

                                moved_roaches_times_2 ++;
                            }
                        }
                    } 
                }
                int counter = 0 ;
                int size = 5 ; 
                while(counter < moved_roaches_times_2)
                {
                    if( moved_roaches_times_2-counter < 5)
                    {
                        size = moved_roaches_times_2-counter;
                    }
                    char *json_data = serialize_coordinates(coordinates_array, size, counter, counter+size);
                    s_send (publisher, json_data);
                    counter  = counter + size;
                } 
    
                char acknowledgment[100];
                sprintf(acknowledgment, "Mexeu\n");
                zmq_send(responder, acknowledgment, strlen(acknowledgment), 0);
            }
        }		
    }
    zmq_close(responder);
    zmq_ctx_destroy(context);
  	endwin();			/* End curses mode		  */

	return 0;
}