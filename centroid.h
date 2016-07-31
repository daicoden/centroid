//
// Created by Matt Wilson on 7/31/16.
//

#ifndef CENTROID_CENTROID_H
#define CENTROID_CENTROID_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
    double x_coordinate;
    double weight;
} centroid_t;

/**
 * y_values should match the same dimension as CENTROID_AXIS
 *
 * centroid_out will contain the x coordinate of centroid for the polygon described by y_values, and the
 * total area (weight) of the polygon described by y_values.
 */
void calculate_centroid(u_short y_values[], centroid_t* centroid_out);

// [-2, -1, 0, 1, 2]
const short CENTROID_AXIS[];
const u_short CENTROID_AXIS_SIZE;

#ifdef __cplusplus
}
#endif

#endif //CENTROID_CENTROID_H_H
