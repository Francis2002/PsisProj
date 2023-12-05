//  Weather update client
//  Connects SUB socket to tcp://localhost:5556
//  Collects weather updates and finds avg temp in zipcode

#include "zhelpers.h"

#define SERVER_PORT "5554"
int main (int argc, char *argv [])
{
    void *context = zmq_ctx_new ();
    void *subscriber = zmq_socket (context, ZMQ_SUB);
    zmq_connect(subscriber, "tcp://127.0.0.1:" SERVER_PORT);
    zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, "", 0);
    while(1) {
        char *string = s_recv (subscriber);
        printf("recevd message - %s\n", string);
        free(string);
    }

    zmq_close (subscriber);
    zmq_ctx_destroy (context);
    return 0;
}