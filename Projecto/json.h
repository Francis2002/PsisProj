#ifndef FILEA_H
#define FILEA_H

#include "remote-char.h"
char* serialize(remote_char_t m);
void deserialize(const char* json_data, remote_char_t *m);

#endif 