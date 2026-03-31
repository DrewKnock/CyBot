/*
 * movement.c
 *
 *  Created on: Feb 6, 2026
 *      Authors: Andy Knockel, Luc Johnson
 */

#include "open_interface.h"

double move_forward(oi_t *sensor_data, double distance_mm);
double move_forward_collision(oi_t *sensor_data, double distance_mm);
double move_backward(oi_t *sensor_data, double distance_mm);
static double collision_helper(oi_t *sensor_data);
double turn_left(oi_t *sensor, double degrees);
double turn_right(oi_t *sensor, double degrees);
static int get_distance(oi_t *sensor_data);

//TODO: Consider adding a conditional with different logic for shorter distances 
double move_forward(oi_t *sensor_data, double distance_mm) {
    int i;
    int increment = 15;
    int sum = 0;
    int brake_distance = distance_mm * 0.95; //start slowing down here

    //Start building up speed slowly
    for (i = 1; i <= 10; i++) {
        oi_setWheels(i * increment, i * increment);
        sum += get_distance(sensor_data);
    }

    while (sum < brake_distance) {
        sum += get_distance(sensor_data);
    }

    for (i = 9; i >= 0; i--) {
            oi_setWheels(i * increment, i * increment);
            sum += get_distance(sensor_data);

            if (sum + (3 * increment) > distance_mm) {
                break;
            }
        }

    oi_setWheels(0,0);
    return sum;
}

//returns distance left to travel, if not zero, then a bump is assumed
double move_forward_bump(oi_t *sensor_data, double distance_mm) {
    int i;
    int increment = 15;
    int sum = 0;

    //Start building up speed slowly
    for (i = 1; i <= 10; i++) {
        oi_setWheels(i * increment, i * increment);
        sum += get_distance(sensor_data);
    }

    while (sum < distance_mm) {
        sum += get_distance(sensor_data);
        if (sensor_data->bumpLeft || sensor_data->bumpRight) {
            oi_setWheels(0, 0);
            return (distance_mm - sum) / 10;
        }
    }

    oi_setWheels(0,0);
    return 0.0;
}

double move_forward_collision(oi_t *sensor_data, double distance_mm) {
    int i = 0;
    int increment = 25;
    int sum = 0;
    int brake_distance = distance_mm * 0.95; //start slowing down at this percent distance

    //Start building up speed slowly
    while (i < 10) {
        //If a collision happens while accelerating, start over
        double setback = collision_helper(sensor_data);
        if (setback) {
            i = 0;
            sum -= setback;
            oi_update(sensor_data);
        }

        oi_setWheels(i * increment, i * increment);
        sum += get_distance(sensor_data);

        //Avoid getting stuck in acceleration loop
        if (sum > brake_distance) {
            break;
        }
        i++;
    }

    while (sum < brake_distance) {
        sum -= collision_helper(sensor_data);
        oi_setWheels(10 * increment, 10 * increment);
        sum += get_distance(sensor_data);
    }

    //Decrease speed for more accurate stopping
    for (i = 9; i >= 0; i--) {
            oi_setWheels(i * increment, i * increment);
            sum += get_distance(sensor_data);

            if (sum + increment > distance_mm) {
                oi_setWheels(0,0);
                break;
            }
        }

    oi_setWheels(0,0);
    return sum;
}

//void accelerate(oi_t *sensor_data) {
//
//}

/*
* Function to move the CyBot backwards
* Does not ease in or out like move_forward() and moves much slower for precision
*
* @return: sum, the distance moved as a positive double
*/
double move_backward(oi_t *sensor_data, double distance_mm) {
    oi_setWheels(-100, -100); 
    int sum = 0;

    while (sum < distance_mm) {
        //Currently subtracting a negative to add
        sum -= get_distance(sensor_data);
    }

    oi_setWheels(0,0);
    return sum;
}

static double collision_helper(oi_t *sensor_data) {
    double sum = 0;
    if (sensor_data -> bumpLeft) {
        sum = move_backward(sensor_data, 150);

        turn_right(sensor_data, 90);
        move_forward(sensor_data, 250);
        turn_left(sensor_data, 90);

        return sum;
    }

    if (sensor_data -> bumpRight) {
        sum = move_backward(sensor_data, 150);

        turn_left(sensor_data, 90);
        move_forward(sensor_data, 250);
        turn_right(sensor_data, 90);

        return sum;
    }

    return 0;
}


double turn_left(oi_t *sensor, double degrees) {
    double sum = 0;

    double calibration = 0.94;
    double rotation_corrected = degrees * calibration;

    oi_setWheels(50, -50);

    while (sum < rotation_corrected) {
        oi_update(sensor);

        sum += sensor -> angle;

    }

    oi_setWheels(0,0);

    return sum;
}

double turn_right(oi_t *sensor, double degrees) {
    double sum = 0;

    double calibration = 0.97;
    double rotation_corrected = degrees * calibration;

    oi_setWheels(-50, 50);

    while (sum + rotation_corrected >= 0) {
        oi_update(sensor);

        sum += sensor -> angle;

    }

    oi_setWheels(0,0);

    return sum;
}

static int get_distance(oi_t *sensor_data) {
    oi_update(sensor_data);
    return sensor_data -> distance;
}

