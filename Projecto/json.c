#include "json.h"
#include "remote-char.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAX_JSON_SIZE 10000

char* serialize(remote_char_t m) {
    char* json_data = (char*)malloc(MAX_JSON_SIZE * sizeof(char));
    if (json_data == NULL) {
        printf("Memory allocation failed!\n");
        return NULL;
    }

    sprintf(json_data, "{\"msg_type\": %d,\"password\": \"%d\",\"client\": \"%d\", \"head\": \"%c\", \"roaches_amount\": %d, \"direction_player\": %d,\"roaches_weights\": [",
            m.msg_type,m.password,m.client_id, m.head, m.roaches_amount,m.direction);

    // Append roaches_weights array to JSON representation
    for (int i = 0; i < m.roaches_amount; ++i) {
        if (i > 0) {
            strcat(json_data, ", ");
        }
        char temp[20]; // Temporary buffer for converting int to string
        sprintf(temp, "%d", m.roaches_weights[i]);
        strcat(json_data, temp);
    }

    // Append roaches_directions array to JSON representation
    strcat(json_data, "], \"roaches_directions\": [");
    for (int i = 0; i < m.roaches_amount; ++i) {
        if (i > 0) {
            strcat(json_data, ", ");
        }
        char temp[20]; // Temporary buffer for converting int to string
        sprintf(temp, "%d", (int)m.roaches_directions[i]);
        strcat(json_data, temp);
    }

    strcat(json_data, "]}"); // Close the JSON object

    return json_data;
}

char* serialize_coordinates(changed_coordinates m[], int size) {
    char* json_data = (char*)malloc(MAX_JSON_SIZE * sizeof(char));
    if (json_data == NULL) {
        printf("Memory allocation failed!\n");
        return NULL;
    }

    sprintf(json_data, "{\"msg_type\": %d,\"changed_coordinates\": [" , size);

    // Append roaches_weights array to JSON representation
    for (int i = 0; i < size; ++i) {
        if (i > 0) {
            strcat(json_data, ", ");
        }
        char temp[100]; // Temporary buffer for converting int to string
        sprintf(temp, "{\"x\": %d, \"y\": %d, \"new_char\": \"%c\"}", m[i].x, m[i].y, m[i].new_char);
        strcat(json_data, temp);
    }

    strcat(json_data, "]}"); // Close the JSON object

    return json_data;
}

void deserialize_coordinates(const char* json_data, coordinates_message *m)
{
    sscanf(json_data, "{\"msg_type\": %d", &(m->size));
    const char* changed_coordinates_start = strstr(json_data, "\"changed_coordinates\": [");
    if (changed_coordinates_start != NULL) {
        changed_coordinates_start += strlen("\"changed_coordinates\": [");
        const char* changed_coordinates_end = strchr(changed_coordinates_start, ']');
        if (changed_coordinates_end != NULL) {
            int num_coordinates = changed_coordinates_end - changed_coordinates_start;
            char* changed_coordinates_str = (char*)malloc((num_coordinates + 1) * sizeof(char));
            if (changed_coordinates_str != NULL) {
                strncpy(changed_coordinates_str, changed_coordinates_start, num_coordinates);
                changed_coordinates_str[num_coordinates] = '\0';

                // Convert coordinates string to integer array
                m->changed_coordinates = (changed_coordinates*)malloc(m->size * sizeof(changed_coordinates));
                if (m->changed_coordinates != NULL) {
                    char* token = strtok(changed_coordinates_str, ", ");
                    int i = 0;
                    while (token != NULL && i < m->size) {
                        sscanf(token, "{\"x\": %d, \"y\": %d, \"new_char\": \"%c\"}", &(m->changed_coordinates[i].x), &(m->changed_coordinates[i].y), &(m->changed_coordinates[i].new_char));
                        token = strtok(NULL, ", ");
                        i++;
                    }
                }
                free(changed_coordinates_str); // Free temporary string
            }
        }
    }
}

void deserialize(const char* json_data, remote_char_t *m) 
{
    int direction_temp = 0;
    sscanf(json_data, "{\"msg_type\": %d,\"password\": \"%d\",\"client\": \"%d\", \"head\": \"%c\", \"roaches_amount\": %d, \"direction_player\": %d",
            &(m->msg_type),&(m->password),&(m->client_id), &(m->head), &(m->roaches_amount),&direction_temp);


    m->direction = direction_temp;
    const char* roaches_weights_start = strstr(json_data, "\"roaches_weights\": [");
    if (roaches_weights_start != NULL) {
        roaches_weights_start += strlen("\"roaches_weights\": [");
        const char* roaches_weights_end = strchr(roaches_weights_start, ']');
        if (roaches_weights_end != NULL) {
            int num_weights = roaches_weights_end - roaches_weights_start;
            char* roaches_weights_str = (char*)malloc((num_weights + 1) * sizeof(char));
            if (roaches_weights_str != NULL) {
                strncpy(roaches_weights_str, roaches_weights_start, num_weights);
                roaches_weights_str[num_weights] = '\0';

                // Convert roaches_weights string to integer array
                m->roaches_weights = (int*)malloc(m->roaches_amount * sizeof(int));
                if (m->roaches_weights != NULL) {
                    char* token = strtok(roaches_weights_str, ", ");
                    int i = 0;
                    while (token != NULL && i < m->roaches_amount) {
                        m->roaches_weights[i++] = atoi(token);
                        token = strtok(NULL, ", ");
                    }
                }
                free(roaches_weights_str); // Free temporary string
            }
        }
    }

    const char* roaches_directions_start = strstr(json_data, "\"roaches_directions\": [");
    if (roaches_directions_start != NULL) {
        roaches_directions_start += strlen("\"roaches_directions\": [");
        const char* roaches_directions_end = strchr(roaches_directions_start, ']');
        if (roaches_directions_end != NULL) {
            int num_directions = roaches_directions_end - roaches_directions_start;
            char* roaches_directions_str = (char*)malloc((num_directions + 1) * sizeof(char));
            if (roaches_directions_str != NULL) {
                strncpy(roaches_directions_str, roaches_directions_start, num_directions);
                roaches_directions_str[num_directions] = '\0';

                // Convert roaches_directions string to direction_t array
                m->roaches_directions = (direction_t*)malloc(m->roaches_amount * sizeof(direction_t));
                if (m->roaches_directions != NULL) {
                    char* token = strtok(roaches_directions_str, ", ");
                    int i = 0;
                    while (token != NULL && i < m->roaches_amount) {
                        m->roaches_directions[i++] = (direction_t)atoi(token);
                        token = strtok(NULL, ", ");
                    }
                }
                free(roaches_directions_str); // Free temporary string
            }
        }
    }
}