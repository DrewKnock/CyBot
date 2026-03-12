#include "uart-interrupt.h"
#include "cyBot_Scan.h"
#include "open_interface.h"
#include "lcd.h"
#include "Timer.h"
#include "movement.h"
#include <math.h>
#include <stdint.h>

#define NUM_OBJECTS 10

typedef struct {
    int id;
    int angle;
    int distance;
    int rad_width;
    double linear_width;
} field_object;

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

    uart_interrupt_init();
    lcd_init();
    timer_init();
    cyBOT_init_Scan(0b0111);

    right_calibration_value = 253750;
    left_calibration_value = 1235500;

    char message[64];

    while (1) {
        if (received_char == 's') {
            uart_sendStr("\r\nDegrees\tIR Value (raw)\r\n");
            //might want to change to uintsomething at some point for memory
            uint16_t i, j;
            uint16_t scan_range = 181; //total amount of angles scanned
            uint16_t scan_start = 0; //starting angle for scan
            uint16_t scan_end = 180; //ending angle for scan
            uint8_t scan_increment = 1; //angle increment of the scan

            uint16_t ir_scan_vals[scan_range]; //holds the averaged ir scan values
            uint16_t ping_scan_vals[scan_range]; //holds the averaged ping scan values

            int num_measurements = 3; //amount of times the sensor value is taken per angle increment

            int ir_scan_sum = 0; //sum of all ir scan values
            int ping_scan_sum = 0; //sum of all ping scan values
            uint16_t ir_distance_average; //average distance of entire scan ir values
            uint16_t ping_distance_average; //average distance of entire scan ping values

            double tolerance = 0.9; //how sensitive to objects

            for (i = scan_start; i <= scan_end; i += scan_increment) {
                if (received_char == 'q') break; //stop the scan if q is pressed

                int ir_sum = 0;
                int ping_sum = 0;
                for (j = 0; j < num_measurements; j++) {
                    cyBOT_Scan(i, &scan);

                    ir_sum += scan.IR_raw_val;
                    ping_sum += scan.sound_dist;
                }
                int ir_average = ir_sum / num_measurements; //average per angle
                int ping_average = ping_sum / num_measurements; //average per angle

                ir_scan_sum += ir_average;
                ping_scan_sum += ping_average;

                ir_scan_vals[i - scan_start] = ir_average;
                ping_scan_vals[i - scan_start] = ping_average;

                sprintf(message, "%d\t%d\r\n", i, ir_average);
                uart_sendStr(message);
            }
            if (received_char == 'q') continue; //don't do calculations if scan is quit

            ir_distance_average = ir_scan_sum / scan_range;
            ping_distance_average = ping_scan_sum / scan_range;

            int object_tolerance = 3; //number of consecutive values for something to be an object
            int consecutive = 0; //number of consecutive values above threshold
            int object_id = 1; //id of object

            //create an array of object to add objects to and set values to 0
            field_object field_objects[NUM_OBJECTS] = {0};

            //maybe add edge detection based on spikes
            for (i = 0; i < scan_range / scan_increment; i++) {
                if (ir_scan_vals[i] > (tolerance * ir_distance_average) && ping_scan_vals[i] < 100) {
                    consecutive++;
                } else {
                    if (consecutive >= object_tolerance) {
                        field_object temp;
                        temp.id = object_id++;
                        temp.angle = i - (consecutive / 2);
                        temp.distance = ping_scan_vals[temp.angle];
                        temp.rad_width = consecutive * scan_increment;
                        temp.linear_width = M_PI * temp.distance * ((double)temp.rad_width / 180.0);
                        field_objects[temp.id - 1] = temp;
                    }
                    consecutive = 0;
                }
            }

            for (i = 0; i < NUM_OBJECTS; i++) {
                if (field_objects[i].id != 0) {
                    field_object o = field_objects[i];
                    uart_sendStr("Object Id\tAngle\tDistance\tRadial Width\tLinear Width\r\n");
                    sprintf(message, "%d       \t%d   \t%d      \t%d          \t%.3lf        \r\n", o.id, o.angle, o.distance, o.rad_width, o.linear_width);
                    uart_sendStr(message);
                    cyBOT_Scan(o.angle, &scan);
                    timer_waitMillis(5000);
                }
            }



            received_char = '\0';
        }
    }
}
