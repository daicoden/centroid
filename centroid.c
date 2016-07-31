//
// Created by Matt Wilson on 7/31/16.
//
#include <printf.h>
#include "centroid.h"

static void calculate_x_centroid_segment(short x1, short y1, short x2, short y2, centroid_t* centroid_segment_out);

void calculate_centroid(u_short y_values[], centroid_t* centroid_out) {
    centroid_t centroid_segment[4];
    for (int i = 0; i < CENTROID_AXIS_SIZE - 1; i++) {
        calculate_x_centroid_segment(CENTROID_AXIS[i], y_values[i], CENTROID_AXIS[i+1], y_values[i+1], &centroid_segment[i]);
    }

    double total_weight = 0;
    double x_coordinate = 0;
    for (int i = 0; i < sizeof(centroid_segment)/sizeof(centroid_t); i++) {
        total_weight = total_weight + centroid_segment[i].weight;
        x_coordinate = x_coordinate + centroid_segment[i].weight*centroid_segment[i].x_coordinate;
    }

    centroid_out->x_coordinate = x_coordinate/total_weight;
    centroid_out->weight = total_weight;
}

static int min(int a, int b) {
    if (a < b) {
        return a;
    } else {
        return b;
    }
}

static int max(int a, int b) {
    if (a < b) {
        return b;
    } else {
        return a;
    }
}

static void calculate_x_centroid_segment(short x1, short y1, short x2, short y2, centroid_t* centroid_segment_out) {
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

