#include "remote-char.h"
#include "json.h"
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <zmq.h>
#include <time.h>
#define SERVER_PORT "5555"
#define MAX_JSON_SIZE 10000
#define Max_Roaches_clients 270
#define Max_Roaches_per_client 100


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


    remote_char_t m;
    m.msg_type = 2;
    m.head = 'a'; // random value.
    m.client_id = (rand() % Max_Roaches_clients);
    m.direction = -1; // random value , needed for the json serialization.
    int roaches_count = (rand() % Max_Roaches_per_client) + 1;
    m.roaches_amount = roaches_count;
    m.roaches_weights = malloc(roaches_count * sizeof(int));
    if (m.roaches_weights == NULL) {//Memory fail safe
        printf("Memory allocation failed!\n");
        return 1; 
    }
    m.roaches_directions = malloc(roaches_count * sizeof(direction_t));
    if (m.roaches_directions == NULL) {//Memory fail safe
        printf("Memory allocation failed!\n");
        return 1; 
    }
    for (int i = 0; i < roaches_count; i++) //Initialize roaches weights
    {
        m.roaches_weights[i] = (rand() % 5) + 49;
        m.roaches_directions[i] = -1;
    }
    char* serialized_data = serialize(m);
    zmq_send(requester, serialized_data, strlen(serialized_data), 0);
    char acknowledgment[100];
    zmq_recv (requester, acknowledgment, sizeof(acknowledgment), 0);
    int successful_connect = 0;
    if((strncmp(acknowledgment, "Succefully connected",20) == 0))
    {
        successful_connect = 1;
    
    }else if((strncmp(acknowledgment, "Invalid id number",17) == 0)){// Talvez precise de mais testing , mas por enquanto funciona
        while(successful_connect == 0)
        {
            m.client_id = (rand() % Max_Roaches_clients);
            serialized_data = serialize(m);
            zmq_send(requester, serialized_data, strlen(serialized_data), 0);
            char ack[100];
            zmq_recv(requester, ack, sizeof(ack), 0);
            if((strncmp(ack, "Succefully connected",20) == 0))
            {
                successful_connect = 1;
            }
        }
    }else if((strncmp(acknowledgment, "Maximum amount of roaches reached",33) == 0)){//Ainda tenho de testar isto
        char *components[2];
        int num_components = 0;
        char *token = strtok(acknowledgment, ":");
        while (token != NULL && num_components < 2) 
        {
            components[num_components] = token;
            num_components++;
            token = strtok(NULL, ":");
        }
        if(atoi(components[1])==1)
        {
            printf("No more roaches clients can connect to server\n");
            exit(1);
        }
        char ack[100];
        roaches_count = (rand() % (atoi(components[1])-1)) + 1;
        m.roaches_amount = roaches_count;
        int* temp_weights = realloc(m.roaches_weights, roaches_count * sizeof(int));
        if (temp_weights == NULL) {//Memory fail safe
            printf("Memory reallocation failed!\n");
        } else {
            m.roaches_weights = temp_weights;
        }
        direction_t* temp_directions = realloc(m.roaches_directions, roaches_count * sizeof(direction_t));
        if (temp_directions == NULL) { //Memory fail safe
            printf("Memory reallocation failed!\n");
        } else {
            m.roaches_directions = temp_directions;
        }
        serialized_data = serialize(m);
        zmq_send(requester, serialized_data, strlen(serialized_data), 0);
        zmq_recv(requester, ack, sizeof(ack), 0);
        if((strncmp(ack, "Succefully connected",20) ) == 0)
        {
            successful_connect = 1;
        }else
        {
            while(successful_connect == 0)
            {
                m.client_id = (rand() % Max_Roaches_clients);
                serialized_data = serialize(m);
                zmq_send(requester, serialized_data, strlen(serialized_data), 0);
                zmq_recv(requester, acknowledgment, sizeof(acknowledgment), 0);
                printf("%s\n", acknowledgment);
                if((strcmp(acknowledgment, "Succefully connected") != 0))
                {
                    successful_connect = 1;
                    break;
                }else
                {
                    printf("entrou nisto\n");
                }
            }
        }
    }


    int sleep_delay;
    direction_t direction;
    while (1)
    {
        sleep_delay = (rand() % 1000000) + 1000000;
        usleep(sleep_delay);
        int random_num;
        for (int i = 0; i < roaches_count; i++) 
        {
            random_num = rand() % 100;
            if(random_num < 75)
            {
                direction = random()%4;
                m.roaches_directions[i] = direction;
            }else
            {
                m.roaches_directions[i]=-1;
            }
        }
        m.msg_type = 3;
        char* serialized_data = serialize(m);
        zmq_send(requester, serialized_data, strlen(serialized_data), 0);
        char ack[100];
        zmq_recv (requester, ack, sizeof(ack), 0);
    }

 
	return 0;
}