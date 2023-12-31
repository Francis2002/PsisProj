

//  Weather update client
//  Connects SUB socket to tcp://localhost:5556
//  Collects weather updates and finds avg temp in zipcode

#include <zmq.h>
#include "zhelpers.h"
#include "json.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ncurses.h>

#define WINDOW_SIZE 30
#define MAX_JSON_SIZE 10000
#define SERVER_PORT "5554"

int main (int argc, char *argv [])
{
    void *context = zmq_ctx_new ();
    void *subscriber = zmq_socket (context, ZMQ_SUB);
    zmq_connect(subscriber, "tcp://127.0.0.1:" SERVER_PORT);
    zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, "", 0);

    initscr();		    	
	cbreak();				
    keypad(stdscr, TRUE);   
	noecho();


    /* creates a window and draws a border */
    WINDOW * my_win = newwin(WINDOW_SIZE, WINDOW_SIZE, 0, 0);
    box(my_win, 0 , 0);	
	wrefresh(my_win);

    while(1) {
        char *string = s_recv (subscriber);
        //printf("%s\n",string);
        char received_data[MAX_JSON_SIZE];
        coordinates_message* message_received;
        message_received = (coordinates_message*)malloc(sizeof(coordinates_message));
        strcpy(received_data, string);
        deserialize_coordinates(received_data, message_received);
        /*for (int i = 0; i < message_received->size; ++i) 
        {
            printf("Coordinate %d: x = %d, y = %d ,new_char = %c\n", i, message_received->changed_coordinates[i].x, message_received->changed_coordinates[i].y,message_received->changed_coordinates[i].new_char);
        }*/
        for(int i = 0; i < message_received->size; i++) 
        {
            if( message_received->changed_coordinates[i].x < WINDOW_SIZE - 1 &&  message_received->changed_coordinates[i].y < WINDOW_SIZE - 1 
                && message_received->changed_coordinates[i].x > 0 && message_received->changed_coordinates[i].y > 0 )
            {
                wmove(my_win, message_received->changed_coordinates[i].x, message_received->changed_coordinates[i].y);
                waddch(my_win, message_received->changed_coordinates[i].new_char);
                curs_set(0);
                wrefresh(my_win);
            }
        }
        free(string);
    }

    zmq_close (subscriber);
    zmq_ctx_destroy (context);
    return 0;
}