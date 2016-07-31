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
static int parse_y_values(const char*, u_short*, int);

// max u_short is is 65535 so 5 max chars
static const int SHORT_CHAR_LENGTH = 5;

// Contains memory space for calculating centroids per client
typedef struct {
    u_short y_values[CENTROID_AXIS_SIZE];
    centroid_t last_calculated_centroid;
} client_info_t;


// comment for problem 1, set to `1' for problem 2
#define MULTI_CLIENT_AVERAGE 0

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
    int error;
    if ((error = parse_y_values(buffer, client_info->y_values, CENTROID_AXIS_SIZE)) != 0) {
        printf("Invalid Input\n");
        sprintf(buffer, "Invalid Input (5 numbers < 65536 which define valid polygon(s)). code: %d", error);
        return;
    }
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

/**
 * Parses out shorts from space delimited ascii text
 * @param buffer input containing space delimited ascii numbers
 * @param y_value_buffer_out array of u_short's to populate.
 * @return 0 if parsed correctly, any other number if invalid input
 *         -1 number bigger than a short
 *         -2 too many or too few numbers
 *         -3 no polygon (all 0 input)
 */
// Apologies C is really rusty
int parse_y_values(const char *buffer, u_short* y_value_buffer_out, int y_value_buffer_length) {
    int y_index = 0;
    char* parsed_number = malloc(sizeof(char)*SHORT_CHAR_LENGTH);
    int parsed_number_index = 0;
    memset(parsed_number, 0, sizeof(char)*SHORT_CHAR_LENGTH);

    for (int i = 0; buffer[i]; i++) {
        if (buffer[i] == ' ') {
            // Overflow
            if (y_index >= y_value_buffer_length) {
                return -2;
            }
            int converted_number = atoi(parsed_number);
            if (converted_number > UINT16_MAX) {
                return -1;
            }

            y_value_buffer_out[y_index++] = (u_short) converted_number;
            memset(parsed_number, 0, sizeof(char)*SHORT_CHAR_LENGTH);
            parsed_number_index = 0;
        } else {
            // overflow
            if (parsed_number_index >= 5) { return -1; }
            parsed_number[parsed_number_index++] = buffer[i];
        }
    }
    // overflow
    if (parsed_number_index >= 5) { return -1; }
    // not enough characters
    if (y_index != y_value_buffer_length - 1) { return -2; }

    y_value_buffer_out[y_index] = (u_short) atoi(parsed_number);
    int ittr = 0;
    for (ittr = 0; ittr < y_value_buffer_length; ittr++) {
        if (y_value_buffer_out[ittr] != 0) { break; }
    }
    if (ittr == y_value_buffer_length) { return -3; }

    free(parsed_number);
    return 0;
}

static void cleanup() {
    // sockserv_destroy is in sockserv.h but not implemented in sockserv.c.
    // fails with `Undefined symbols for architecture x86_64'.
    //sockserv_destroy(server);
}
