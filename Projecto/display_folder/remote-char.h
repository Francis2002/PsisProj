
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

typedef struct changed_coordinates
{
    int x;
    int y;
    char new_char;
}changed_coordinates;

typedef struct coordinates_message
{
    changed_coordinates *changed_coordinates;
    int size;
}coordinates_message;

#define FIFO_NAME "/tmp/fifo_snail" // whythis is here?


#endif 