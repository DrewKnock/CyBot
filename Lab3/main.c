#include "cyBot_uart.h"
#include "cyBot_Scan.h"
#include "open_interface.h"
#include "lcd.h"
#include "Timer.h"
#include "movement.h"

typedef struct {
    int num;
    int angle;
    double distance;
    int rad_width;
} field_object;

void send_string(char *str) {
    int i;
    //send string one char at a time
    for (i = 0; str[i] != '\0'; i++) {
        cyBot_sendByte(str[i]);
    }
}

void putty_test() {
    char receive_char;
    char char_message[50];

    while (1) {

        //Wait until a char is received from putty
        receive_char = cyBot_getByte();

        //Print char on lcd screen
        lcd_putc(receive_char);

        //Format message with given char
        sprintf(char_message, "Received an %c\r\n", receive_char);

        //Send char array to send_string method
        send_string(char_message);
    }
}

void calibrate(void) {
    timer_init();
    lcd_init();
    cyBOT_init_Scan(0b0001);
    cyBOT_SERVO_cal();
}

int main(void) {
    cyBOT_Scan_t scan;

    oi_t *sensor_data = oi_alloc();
    oi_init(sensor_data);

    cyBot_uart_init();
    lcd_init();
    timer_init();
    cyBOT_init_Scan(0b011);

    right_calibration_value = 316750;
    left_calibration_value = 1303750;

    char receive_char;
    char message[50] = "";

    while (1) {
        //wait for character
        receive_char = cyBot_getByte();

        send_string("Angle (degrees\tDistance (cm)\r\n");

        if (receive_char == 'm') {
            //perform scan and send values to putty
            int i;
            for (i = 0; i <= 180; i += 2) {
                cyBOT_Scan(i, &scan);

                //send distance value of each angle to putty
                uint16_t dist = scan.sound_dist;
                sprintf(message, "%d\t%d\r\n", i, dist);
                send_string(message);
            }
            //move scanner back to 90
            cyBOT_Scan(90, &scan);
        }
    }

        /*
        if (receive_char == 'm') {
            double scan_vals[181];
            double distance_tolerance = 0.5;

            //perform scan and send values to putty
            int i;
            for (i = 0; i <= 180; i++) {
                cyBOT_Scan(i, &scan);
                timer_waitMillis(10);

                //send distance value of each angle to putty
                double dist = scan.sound_dist;
                sprintf(message, "%d\t%.5lf\r\n", i, dist);
                send_string(message);

                //add values to array
                scan_vals[i] = dist;
            }
            //move scanner back to 90
            cyBOT_Scan(90, &scan);

            //find the average and send to putty
            double sum = 0;
            for (i = 0; i <= 180; i++) {
                sum += scan_vals[i];
            }

            double average = sum / 181;
            sprintf(message, "Average distance is: %.5lf\r\n", average);
            send_string(message);

            //variable definitions for field objects
            field_object field_objects[30] = {0};
            int object_tolerance = 2; //don't create an object unless the number of consecutive distances under the tolerance is less than this variable
            int object_num = 1;
            int consecutive = 0;

            //create objects based on sensor data
            for (i = 0; i <= 180; i++) {
                if (scan_vals[i] < average * distance_tolerance || scan_vals[i] < 100) {
                    consecutive++;
                } else {
                    if (consecutive >= object_tolerance) {
                        //create object
                        field_object temp;
                        temp.num = object_num++; //assign the object num and increment after
                        temp.angle = i - (consecutive / 2); //angle is the current i - 1/2 total distance of the object
                        temp.distance = scan_vals[temp.angle]; //get the distance of the middle angle
                        temp.rad_width = consecutive;
                        field_objects[object_num - 2] = temp;
                    }
                    consecutive = 0;
                }
            }


            //find the object with lowest width out of all objects found
            field_object min_width_object = field_objects[0];
            for (i = 0; i < 30; i++) {
                field_object object = field_objects[i];
                if (object.num == 0) break;

                if (object.rad_width < min_width_object.rad_width) {
                    min_width_object = object;
                }

                //send a string description of all the objects
                send_string("Object#\tAngle\tDistance\tRadial Width\r\n");
                sprintf(message, "%d\t%d\t%.5lf\t%d\r\n", object.num, object.angle, object.distance, object.rad_width);
                send_string(message);
            }

            //point the scanner towards the object with smallest width
            int min_width_angle = min_width_object.angle;
            cyBOT_Scan(min_width_angle, &scan);
            timer_waitMillis(2000);

            //reset scanner to 90
            cyBOT_Scan(90, &scan);
            timer_waitMillis(2000);

            double scan_turn_offset = 0.87; //since the scanner is slightly forward compared to center of robot, reduce the angle it turns
            //turn the robot towards smallest object
            if (min_width_angle >= 90) {
                turn_left(sensor_data, (min_width_angle - 90) * scan_turn_offset);
            } else {
                turn_right(sensor_data, (90 - min_width_angle) * scan_turn_offset);
            }

            //moved toward the object
            move_forward(sensor_data, (min_width_object.distance * 10) - 160);

        } else if (receive_char == 'q') break;
    } */

    oi_free(sensor_data);

    return 0;
}
