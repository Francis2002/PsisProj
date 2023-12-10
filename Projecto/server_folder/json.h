#ifndef FILEA_H
#define FILEA_H

#include "remote-char.h"
char* serialize(remote_char_t m);
void deserialize(const char* json_data, remote_char_t *m);

char* serialize_coordinates(changed_coordinates *m, int size, int begining_position,int final_position);
void deserialize_coordinates(const char* json_data, coordinates_message *m);

#endif 