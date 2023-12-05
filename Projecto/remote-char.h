
#ifndef REMOTE_CHAR
#define REMOTE_CHAR


typedef enum direction_t {UP, DOWN, LEFT, RIGHT} direction_t;

typedef struct remote_char_t
{   
    int msg_type;
    int password;
    char head;
    int client_id;
    int roaches_amount; 
    int *roaches_weights;
    direction_t direction;
    direction_t *roaches_directions;
}remote_char_t;

#define FIFO_NAME "/tmp/fifo_snail"


#endif 