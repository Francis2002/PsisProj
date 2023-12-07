#include "remote-char.h"
#include "json.h"
#include <ncurses.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>  
#include <ctype.h> 
#include <stdlib.h>
#include <zmq.h>
#include <string.h>
#include <time.h>
#define SERVER_PORT "5555"
#define MAX_JSON_SIZE 10000

//Ideas: maybe store the already tried id for a quicker evalutaion

int main()
{
    srand(time(NULL));
    void* context = zmq_ctx_new();
    void* requester = zmq_socket(context, ZMQ_REQ);

    // Connect to the server
    int connection_result = zmq_connect(requester, "tcp://127.0.0.1:" SERVER_PORT);
    if (connection_result == 0) {
        //printf("Connection to server successful!\n");
    } else {
        fprintf(stderr, "Failed to connect to server.\n");
    }


    char ch;
    do{
        printf("what is your character(a..z)?: ");
        ch = getchar();
        ch = tolower(ch);  
    }while(!isalpha(ch));


    remote_char_t m;
    m.msg_type = 0;
    m.password = 0;
    m.client_id = (rand() % 26);
    m.roaches_amount = 0;
    m.head = ch;
    char* serialized_data = serialize(m);
    zmq_send(requester, serialized_data, strlen(serialized_data), 0);
    char acknowledgment[100];
    zmq_recv (requester, acknowledgment, sizeof(acknowledgment), 0);
    int successful_connect = 0;
    if((strncmp(acknowledgment, "Succefully connected",20) == 0))
    {
        int num_components = 0;
        char *token = strtok(acknowledgment, ":");
        while (token != NULL && num_components < 2) 
        {
            m.password = atoi(token);
            num_components++;
            token = strtok(NULL, ":");
        }

        printf("%d\n",m.password);

        successful_connect = 1;
    
    }else if((strncmp(acknowledgment, "Invalid id number",17) == 0))
    {                                       
        while(successful_connect == 0)
        {
            m.client_id = (rand() % 29);
            serialized_data = serialize(m);
            zmq_send(requester, serialized_data, strlen(serialized_data), 0);

            char ack[100];
            zmq_recv(requester, ack, sizeof(ack), 0);
            if((strncmp(ack, "Succefully connected",20) == 0))
            {
                int num_components = 0;
                char *token = strtok(acknowledgment, ":");
                while (token != NULL && num_components < 2) 
                {
                    m.password  = atoi(token);
                    num_components++;
                    token = strtok(NULL, ":");
                }
                successful_connect = 1;
            }
        }
    }else if((strncmp(acknowledgment, "Server at max Capacity",23) == 0))
    {
        endwin();
        printf("Server cannot accept anymore players at the moment\n");
        exit(1);
    }


	initscr();		
	cbreak();				
	keypad(stdscr, TRUE);
    scrollok(stdscr, TRUE);		
	noecho();			

    //int n = 0;
    int key;
    int previous_score;
    do
    {
    	key = getch();		
        //n++;
        switch (key)
        {
        case KEY_LEFT:
           m.direction = LEFT;
            break;
        case KEY_RIGHT:
            m.direction = RIGHT;
            break;
        case KEY_DOWN:
           m.direction = DOWN;
            break;
        case KEY_UP:
            m.direction = UP;
            break;
        case 'q':
        case 'Q':
            m.msg_type = -1;
            char* serialized_data = serialize(m);
            zmq_send(requester, serialized_data, strlen(serialized_data), 0);
            char ack[100];
            zmq_recv (requester, ack, sizeof(ack), 0);
            endwin();
            printf("%s\n",ack);
            exit(1); 
            break;
        default:
            key = 'x'; 
            break;
        }

         if (key != 'x') {
            m.msg_type = 1; 
            char* serialized_data = serialize(m);
            zmq_send(requester, serialized_data, strlen(serialized_data), 0);
            char ack[100];
            zmq_recv (requester, ack, sizeof(ack), 0);
            char *components[2];
            int num_components = 0;
            char *token = strtok(ack, ":");
            while (token != NULL && num_components < 2) 
            {
                components[num_components] = token;
                num_components++;
                token = strtok(NULL, ":");
            }
            if(previous_score == atoi(components[1]))
            {
                continue;
            }
            printw("Your score is : %d\n",atoi(components[1]));
            previous_score = atoi(components[1]);
        }
        refresh();
    }while(key != 27);
    
    
  	endwin();			/* End curses mode		  */

	return 0;
}