#include "uart-interrupt.h"
#include "cyBot_Scan.h"
#include "open_interface.h"
#include "lcd.h"
#include "Timer.h"
#include "movement.h"
#include <math.h>
#include <stdint.h>
#include <string.h>

#define NUM_OBJECTS 10
#define MAX_SCAN_RANGE 181

typedef struct {
    int id;
    int angle;
    double distance;
    int rad_width;
    double linear_width;
} field_object;

typedef struct {
    uint8_t range;
    uint8_t start;
    uint8_t end;
    uint8_t increment;
    uint8_t num_measures;
} scan_config;

static cyBOT_Scan_t scan;
static char message[64];
static field_object field_objects[NUM_OBJECTS] = {0}; //global objects array
static oi_t *sensor_data;

void calibrate_servo(void) {
    timer_init();
    lcd_init();
    cyBOT_init_Scan(0b0001);
    cyBOT_SERVO_cal();
}

//takes in a 2 digit binary number where the 0th bit is 1 if you need to calibrate right and 1th bit is 1 if you need to calibrate left
void calibrate_turn(uint8_t turn_config) {
    oi_t *sensor_data = oi_alloc();
    oi_init(sensor_data);

    if (turn_config & 0b1) {
        timer_waitMillis(20000);
        turn_right(sensor_data, 90);
    }
    if (turn_config & 0b10 || turn_config & 0b11) {
        timer_waitMillis(20000);
        turn_left(sensor_data, 90);
    }
}

//returns index of minimum width object, -1 if no object
uint8_t find_min_width_object() {
    uint8_t i;
    field_object min_width_object = field_objects[0];
    field_object object;

    for (i = 0; i < NUM_OBJECTS; i++){
        object = field_objects[i];
        if (object.id == 0) break;

        if (object.linear_width < min_width_object.linear_width) {
            min_width_object = object;
        }
    }

    return min_width_object.id - 1;
}

//returns angle of biggest gap, straight left or right if no gaps
uint16_t find_recourse_angle() {
    int8_t i;
    field_object first_object;
    field_object second_object;

    uint16_t gap_size;
    uint16_t biggest_gap = 0;
    uint8_t reference_angle; //use this to add half of the biggest gap to find center of gap

    for (i = -1; i < NUM_OBJECTS; i++){
        if (i == NUM_OBJECTS - 1) break;

        if (i != -1) {
            uart_sendStr(message);
            first_object = field_objects[i];
            if (first_object.id == 0) break;
        }
        second_object = field_objects[i + 1];

        //check gap between angle 0 and first object
        if (i == -1) {
            gap_size = second_object.angle; //this is checked when i is -1, so second object is the 0th object and the first object is not initialized
            //since the first iteration is always going to be the largest at first, set values and continue
            reference_angle = 0;
            biggest_gap = gap_size;

            sprintf(message, "gap angle: %d\r\n", gap_size);
            uart_sendStr(message);

            continue;
        }
        //check gap between last object and 180
        else if (second_object.id == 0) {
            gap_size = 180 - first_object.angle;
        }
        //otherwise check gap between two objects
        else {
            gap_size = second_object.angle - first_object.angle;
        }

        sprintf(message, "gap angle: %d\r\n", gap_size);
        uart_sendStr(message);

        //for the rest of iterations, if the gap angle is bigger than the max, update the reference angle and biggest gap
        if (gap_size > biggest_gap) {
            reference_angle = first_object.angle;
            biggest_gap = gap_size;
            sprintf(message, "biggest gap: %d\r\n", biggest_gap);
            uart_sendStr(message);
        }
        uart_sendStr("\r\n");
    }

    sprintf(message, "final biggest gap: %d\r\n", biggest_gap);
    uart_sendStr(message);

    sprintf(message, "reference angle: %d\r\n", reference_angle);
    uart_sendStr(message);
    uart_sendStr("\r\n");

    //return the angle you want to go towards to get through the gap
    return reference_angle + (biggest_gap / 2); //goes halfway between the largest gap
}

void turn_to_angle(uint16_t angle) {
    cyBOT_Scan(angle, &scan);
    timer_waitMillis(500);

    cyBOT_Scan(90, &scan);
    timer_waitMillis(500);

    double scan_turn_offset = 0.73;

    if (angle >= 90) {
        turn_left(sensor_data, (angle - 90) * scan_turn_offset);
    } else {
        turn_right(sensor_data, (90 - angle) * scan_turn_offset);
    }
}

void send_object_data(void) {
    uint8_t i;
    //send object data to gui
    for (i = 0; i < NUM_OBJECTS; i++) {
        if (field_objects[i].id != 0) {
            field_object o = field_objects[i];
            uart_sendStr("Object Id\tAngle\tDistance\tRadial Width\tLinear Width\r\n");
            sprintf(message, "%d       \t%d   \t%.3lf      \t%d          \t%.3lf        \r\n", o.id, o.angle, o.distance, o.rad_width, o.linear_width);
            uart_sendStr("OBJECT\n");
            uart_sendStr(message);
        }
    }
    timer_waitMillis(2000);
}

void scan_field(scan_config *config) {
    //reset field_objects array to 0's
    memset(field_objects, 0, sizeof(field_objects));

    cyBOT_init_Scan(0b0101);
    uart_sendStr("scan start\r\n");
    uart_sendStr("\r\nDegrees\tIR Value (raw)\r\n");

    uint16_t i, j;

    int16_t tolerance = 220 * 5 / config->num_measures + 2;
    uint8_t object_tolerance = 8;
    uint8_t consecutive = 0;
    uint8_t id = 1;

    uint16_t ir_average;
    uint16_t prev_ir_average;
    int total_ir = 0;

    for (i = config->start; i <= config->end; i += config->increment) {
        if (received_char == 'q') break; //stop the scan if q is pressed

        //sum of multiple measurements for the same angle
        uint16_t ir_sum = 0;
        for (j = 0; j < config->num_measures; j++) {
            cyBOT_Scan(i, &scan);

            ir_sum += scan.IR_raw_val;
        }
        ir_average = ir_sum / config->num_measures; //average per angle

        total_ir += ir_average;

        if (i == config->start) {
            prev_ir_average = ir_average;
        }

        if (ir_average - prev_ir_average > -tolerance && ir_average - prev_ir_average < tolerance && ir_average > 850 && i != config->end) {
            consecutive++;
        } else {
            if (consecutive * config->increment >= object_tolerance) {
                //create temporary field_object to assign to the array of objects, can't assign linear_width or distance until ping scan
                field_object temp;
                temp.id = id++;
                temp.angle = i - (consecutive / 2);
                temp.rad_width = consecutive * config->increment;
                field_objects[id - 2] = temp;

                sprintf(message, "Object is %d wide\r\n", consecutive * config->increment);
                uart_sendStr(message);
            }
            consecutive = 0;
        }

        prev_ir_average = ir_average;

        sprintf(message, "%d\t%d\tscan\r\n", i, ir_average);
        uart_sendStr(message);
    }

    cyBOT_init_Scan(0b0111);
    //go through the created objects and get the distance and linear width
    for (i = 0; i < NUM_OBJECTS; i++) {
        if (field_objects[i].id != 0) {
            field_object temp = field_objects[i];
            float ping_total = 0;
            for (j = 0; j < config->num_measures * 4; j++) {
                cyBOT_Scan(temp.angle, &scan);
                ping_total += scan.sound_dist;
            }
            temp.distance = ping_total / (config->num_measures * 4);
            temp.linear_width = M_PI * temp.distance * ((double)temp.rad_width / 180.0);
            field_objects[i] = temp;
        }
    }
    send_object_data();
}

//main drive loop, currently called after scanning
uint8_t auto_drive(void) {
    field_object min_width_object = field_objects[find_min_width_object()];

    int min_width_angle = min_width_object.angle;
    double distance_left;
    double distance;

    uint8_t turn_back_f = 0; //if the cybot needs to turn back towards the target object
    int16_t reorient_angle; //positive or negative angle to reorient the cybot after recourse

    turn_to_angle(min_width_angle);
    distance = min_width_object.distance - 10;
    uint8_t num_recourses = 0;
    uint8_t backup_distance = 20;

    while (1) {
        if (num_recourses >= 10) break;
        if (turn_back_f == 1) {
            if (reorient_angle <= 0) {
                turn_right(sensor_data, -reorient_angle);
            } else {
                turn_left(sensor_data, reorient_angle);
            }

            scan_config config = {91, 45, 135, 2, 2};
            scan_field(&config);

            int8_t min_id = find_min_width_object();
            if (min_id == -1) {
                reorient_angle = (reorient_angle < 0) ? 20 : -20;
                continue;
            }

            min_width_object = field_objects[min_id];
            turn_to_angle(min_width_object.angle);
            distance = min_width_object.distance - 20;
        }

        sprintf(message, "distance before drive forward: %.2lf\r\n", distance);
        uart_sendStr(message);
        distance_left = move_forward_bump(sensor_data, distance * 10);

        if (distance_left != 0) {
            move_backward(sensor_data, backup_distance * 10);
            scan_config config = {181, 0, 180, 2, 2};
            scan_field(&config);

            uint16_t recourse_angle = find_recourse_angle();
            sprintf(message, "recourse angle: %d\r\n", recourse_angle);
            uart_sendStr(message);

            reorient_angle = (int)90 - recourse_angle;
            reorient_angle = (reorient_angle > 0) ? reorient_angle + 90 : reorient_angle - 90;
            sprintf(message, "reorient angle is: %d\r\n", reorient_angle);
            uart_sendStr(message);

            turn_to_angle(recourse_angle);

            distance = (distance_left + backup_distance + 10) / cos(recourse_angle * M_PI / 180);
            distance = (distance < 0) ? -distance : distance;
            distance = (distance > 80) ? 80 : distance;

            sprintf(message, "distance left: %.2lf calculated recourse distance: %.2lf\r\n", distance_left, distance);
            uart_sendStr(message);

            sprintf(message, "weird angle value: %.2lf\r\n", cos(recourse_angle * M_PI / 180));
            uart_sendStr(message);

            turn_back_f = 2;

            num_recourses++;
        } else if (distance_left == 0 && turn_back_f != 2){
            return 1;
        } else {
            turn_back_f--;
        }
    }
    return 0;
}

int main(void) {
    sensor_data = oi_alloc();
    oi_init(sensor_data);
    uart_interrupt_init();
    lcd_init();
    timer_init();
    cyBOT_init_Scan(0b0111);

    right_calibration_value = 337750;
    left_calibration_value = 1382500;

    while (1) {
        if (received_char == 's') {
            scan_config config = {181, 0, 180, 1, 3}; //range, start, end, increment, number of measures per angle

            scan_field(&config);
            if (received_char == 'q') continue; //don't do calculations if scan is quit

            //right now just drives every time you hit the scan button
            uint8_t success = auto_drive();

            if (success) {
                uart_sendStr("Success!\n");
            } else {
                uart_sendStr("I'm literally Sisyphus");
            }

            uart_sendStr("END\n");

            received_char = '\0';
        }
    }
}
