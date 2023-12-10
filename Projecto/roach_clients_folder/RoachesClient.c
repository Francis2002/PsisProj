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
#define MAX_JSON_SIZE 10000
#define Max_Roaches_clients 270
#define Max_Roaches_per_client 10

int verify_ip_and_tcpport(char* ip)
{
  char store_temp[5][1000];
  char temporary[1000];
  strcpy(temporary,ip);
  char *token;
  int j=0;
  token = strtok(temporary,".");    // partir a repostas nas suas componentes 
  while( token != NULL )
      {
        strcpy(store_temp[j],token);
        token = strtok(NULL,".");
        j++;
      }
  if (((strlen(store_temp[0])>0) && (strlen(store_temp[1])>0)&& (strlen(store_temp[2])>0) 
      &&(strlen(store_temp[3])<4)) && ((strcmp(store_temp[0],"\0")!=0) && (strcmp(store_temp[1],"\0")!=0)&& 
        ((atoi(store_temp[0])<266) && (atoi(store_temp[1])<266) && (atoi(store_temp[2])<266) && (atoi(store_temp[3])<266))))
  {
    return 0;
  }
  return -1;
}


int main(int argc, char *argv[])
{	 
    if(argc != 3)
    {
        printf("Invalid number of arguments\n");
        exit(1);
    }

    int SERVER_PORT = atoi(argv[2]);
    if(SERVER_PORT < 1024 || SERVER_PORT > 65535)
    {
        printf("Invalid port number\n");
        exit(1);
    }

    char *ipAddress = argv[1];
    if(verify_ip_and_tcpport(ipAddress) == -1)
    {
        printf("Invalid ip address\n");
        exit(1);
    }

    
    srand(time(NULL));

    void* context = zmq_ctx_new();
    void* requester = zmq_socket(context, ZMQ_REQ);

    // Connect to the server
    char server[100];
    sprintf(server, "tcp://%s:%d", ipAddress, SERVER_PORT);
    int connection_result = zmq_connect(requester, server);
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
        int num_components = 0;
        char *token = strtok(acknowledgment, ":");
        while (token != NULL && num_components < 2) 
        {
            m.password = atoi(token);
            num_components++;
            token = strtok(NULL, ":");
        }

        successful_connect = 1;
    
    }else if((strncmp(acknowledgment, "Invalid id number",17) == 0))
    {
        while(successful_connect == 0)
        {
            m.client_id = (rand() % Max_Roaches_clients);
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
                    m.password = atoi(token);
                    num_components++;
                    token = strtok(NULL, ":");
                }

                successful_connect = 1;
            }
        }
    }else if((strncmp(acknowledgment, "Maximum amount of roaches reached",33) == 0))
    {
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
            int num_components = 0;
            char *token = strtok(acknowledgment, ":");
            while (token != NULL && num_components < 2) 
            {
                m.password = atoi(token);
                num_components++;
                token = strtok(NULL, ":");
            }

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