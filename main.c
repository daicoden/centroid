#include <printf.h>
#include <stdlib.h>
#include <string.h>
#include "sockserv.h"

static void* add_client(unsigned char[4]);
static void delete_client(void*, char*);
static void receive(void*, char*);
static void cleanup();

typedef struct {
    double x_coordinate;
    double weight;
} centroid_t;

#define MULTI_CLIENT_AVERAGE 1


void parse_y_values(const char*, u_short*);
void calculate_centroid(u_short y_values[], int size, centroid_t* centroid_out);

// [-2, -1, 0, 1, 2]
static const short AXIS[] = {-2, -1, 0, 1, 2};
static const u_short AXIS_SIZE = sizeof(AXIS)/sizeof(short);

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

// Contains memory space for calculating centroids per client
typedef struct {
    u_short y_values[AXIS_SIZE];
    centroid_t last_calculated_centroid;
} client_info_t;

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
    calculate_centroid(client_info->y_values, AXIS_SIZE, &client_info->last_calculated_centroid);

#ifdef MULTI_CLIENT_AVERAGE
    double average = 0;
    double total_weight = 0;
    for (sbuf_t* client = server->client_list; client; client = client->next) {
        client_info_t *info = (client_info_t*) client->user_data;
        average = average + info->last_calculated_centroid.x_coordinate * info->last_calculated_centroid.weight;
        total_weight = total_weight + info->last_calculated_centroid.weight;
    }
    average = average/total_weight;
    sprintf(buffer, "%0.2f", average);
#else
    sprintf(buffer, "%0.2f", client_info->last_calculated_centroid.x_coordinate);
#endif
}

int min(int a, int b) {
    if (a < b) {
        return a;
    } else {
        return b;
    }
}

int max(int a, int b) {
    if (a < b) {
        return b;
    } else {
        return a;
    }
}


void calculate_x_centroid_segment(short x1, short y1, short x2, short y2, centroid_t* centroid_segment_out) {
    int base = x2 - x1;

    int rect_height = min(y1, y2);
    double rect_weight = rect_height * base;
    double rect_mid = (x1+x2)/2.0;

    int triangle_height = max(y1, y2) - rect_height;
    double triangle_weight = (1.0/2.0*triangle_height*base);
    double triangle_mid = 0;
    if (y1 > y2) {
        triangle_mid = x1+(x2-x1)/3.0;
    } else if (y2 > y1) {
        triangle_mid = x2-(x2-x1)/3.0;
    } else {
        triangle_mid = 0;
    }

    double total_weight = triangle_weight + rect_weight;

    if (total_weight == 0) {
        centroid_segment_out->weight = 0;
        centroid_segment_out->x_coordinate = rect_mid;
    } else {
        centroid_segment_out->weight = total_weight;
        centroid_segment_out->x_coordinate = (rect_mid*rect_weight+triangle_mid*triangle_weight)/total_weight;
    }
}

void calculate_centroid(u_short y_values[], int size, centroid_t* centroid_out) {
    printf("Looping over centroid %d \n", size);
    centroid_t centroid_segment[4];
    for (int i = 0; i < size - 1; i++) {
        calculate_x_centroid_segment(AXIS[i], y_values[i], AXIS[i+1], y_values[i+1], &centroid_segment[i]);
    }

    double total_weight = 0;
    double x_coordinate = 0;
    for (int i = 0; i < sizeof(centroid_segment)/sizeof(centroid_t); i++) {
        printf("Calculated weight %0.2f\n", centroid_segment[i].weight);
        printf("Calculated x_coordinate %0.2f\n", centroid_segment[i].x_coordinate);
        total_weight = total_weight + centroid_segment[i].weight;
        x_coordinate = x_coordinate + centroid_segment[i].weight*centroid_segment[i].x_coordinate;
    }

    centroid_out->x_coordinate = x_coordinate/total_weight;
    centroid_out->weight = total_weight;
}

// Apologies C is really rusty
void parse_y_values(const char *buffer, u_short* y_value_buffer_out) {
    int y_index = 0;
    // max input is 65536 so 5 max chars
    char* parsed_number = malloc(sizeof(char)*AXIS_SIZE);
    int parsed_number_index = 0;
    memset(parsed_number, 0, sizeof(char)*AXIS_SIZE);

    for (int i = 0; buffer[i]; i++) {
        if (buffer[i] == ' ') {
            y_value_buffer_out[y_index++] = (u_short) atoi(parsed_number);
            memset(parsed_number, 0, sizeof(char)*AXIS_SIZE);
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
