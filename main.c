#include <printf.h>
#include <stdlib.h>
#include <string.h>
#include "sockserv.h"
#include "centroid.h"

const short CENTROID_AXIS[] = {-2, -1, 0, 1, 2};
const u_short CENTROID_AXIS_SIZE = sizeof(CENTROID_AXIS)/sizeof(short);

static void* add_client(unsigned char[4]);
static void delete_client(void*, char*);
static void receive(void*, char*);
static void cleanup();
static void parse_y_values(const char*, u_short*);

// Contains memory space for calculating centroids per client
typedef struct {
    u_short y_values[CENTROID_AXIS_SIZE];
    centroid_t last_calculated_centroid;
} client_info_t;


// comment for problem 1, set to `1' for problem 2
//#define MULTI_CLIENT_AVERAGE 0

sockserv_t* server = NULL;

int main() {
    server = sockserv_create("34543");
    if (!server) {
        exit(-1);
    }

    server->client_add_hook = add_client;
    server->client_del_hook = delete_client;
    server->client_recv_hook = receive;
    printf("Listening on port: %d\n", server->port_listen);
    atexit(cleanup);
    for(;;) {
        sockserv_run(server, SOCKSERV_WAIT);
    };
}

void* add_client(unsigned char remote_id[4]) {
    client_info_t* client_info;
    client_info = calloc(sizeof(client_info_t), sizeof(client_info_t));
    return client_info;
}

void delete_client(void * cinfo, char* buffer) {
    free(cinfo);
}

void receive(void * cinfo, char* buffer) {
    printf("Received packed from client: %s\n", buffer);
    client_info_t* client_info = (client_info_t*)cinfo;
    printf("Parsing y values\n");
    parse_y_values(buffer, client_info->y_values);
    printf("Calculating centroid\n");
    calculate_centroid(client_info->y_values, &client_info->last_calculated_centroid);

#ifdef MULTI_CLIENT_AVERAGE
    double average = 0;
    double total_weight = 0;
    for (sbuf_t* connected_client = server->client_list; connected_client; connected_client = connected_client->next) {
        client_info_t *connected_client_info = (client_info_t*) connected_client->user_data;
        average = average + connected_client_info->last_calculated_centroid.x_coordinate *
                                    connected_client_info->last_calculated_centroid.weight;
        total_weight = total_weight + connected_client_info->last_calculated_centroid.weight;
    }
    average = average/total_weight;
    sprintf(buffer, "%0.2f", average);
#else
    sprintf(buffer, "%0.2f", client_info->last_calculated_centroid.x_coordinate);
#endif
}

// Apologies C is really rusty
void parse_y_values(const char *buffer, u_short* y_value_buffer_out) {
    int y_index = 0;
    // max input is 65536 so 5 max chars
    char* parsed_number = malloc(sizeof(char)*CENTROID_AXIS_SIZE);
    int parsed_number_index = 0;
    memset(parsed_number, 0, sizeof(char)*CENTROID_AXIS_SIZE);

    for (int i = 0; buffer[i]; i++) {
        if (buffer[i] == ' ') {
            y_value_buffer_out[y_index++] = (u_short) atoi(parsed_number);
            memset(parsed_number, 0, sizeof(char)*CENTROID_AXIS_SIZE);
            parsed_number_index = 0;
        } else {
            parsed_number[parsed_number_index++] = buffer[i];
        }
    }
    y_value_buffer_out[y_index] = (u_short) atoi(parsed_number);
    free(parsed_number);
}

static void cleanup() {
//    sockserv_destroy(server);
}
