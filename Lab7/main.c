#include "uart-interrupt.h"
#include "cyBot_Scan.h"
#include "open_interface.h"
#include "lcd.h"
#include "Timer.h"
#include "movement.h"
#include <math.h>
#include <stdint.h>

#define NUM_OBJECTS 10
#define MAX_SCAN_RANGE 181

typedef struct {
    int id;
    int angle;
    int distance;
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

typedef struct {
    uint16_t ir_average;
    uint16_t ping_average;
} scan_result;

static cyBOT_Scan_t scan;
static char message[64];
static scan_result result;

void calibrate(void) {
    timer_init();
    lcd_init();
    cyBOT_init_Scan(0b0001);
    cyBOT_SERVO_cal();
}

void scan_field(scan_config *config, scan_result *result) {
    uart_sendStr("\r\nDegrees\tIR Value (raw)\r\n");

    uint16_t i, j;

    int ir_scan_sum = 0; //sum of all ir scan values
    int ping_scan_sum = 0; //sum of all ping scan values

    for (i = config->start; i <= config->end; i += config->increment) {
        if (received_char == 'q') break; //stop the scan if q is pressed

        //sum of multiple measurements for the same angle
        int ir_sum = 0;
        int ping_sum = 0;
        for (j = 0; j < config->num_measures; j++) {
            cyBOT_Scan(i, &scan);

            ir_sum += scan.IR_raw_val;
            ping_sum += scan.sound_dist;
        }
        uint16_t ir_average = ir_sum / config->num_measures; //average per angle
        uint16_t ping_average = ping_sum / config->num_measures; //average per angle

        ir_scan_sum += ir_average;
        ping_scan_sum += ping_average;

        result->ir_scan_vals[i - config->start] = ir_average;
        result->ping_scan_vals[i - config->start] = ping_average;

        sprintf(message, "%d\t%d\t%d\tscan\r\n", i, ir_average, ping_average);
        uart_sendStr(message);
    }
    //calculate average of all scan values and assign to scan_result struct
    result->ir_average = ir_scan_sum / config->range;
    result->ping_average = ping_scan_sum / config->range;
}

void scan_field2(scan_config *config, scan_result *result) {
    cyBOT_init_Scan(0b0101);
    uart_sendStr("\r\nDegrees\tIR Value (raw)\r\n");

    uint16_t i, j;

    int16_t tolerance = 100;
    uint8_t object_tolerance = 6;
    uint8_t consecutive = 0;

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

        if (ir_average - prev_ir_average > -tolerance && ir_average - prev_ir_average < tolerance) {
            consecutive++;
        } else {
            if (consecutive >= object_tolerance) {
                sprintf(message, "Object is %d wide\r\n", consecutive);
                uart_sendStr(message);
            }
            consecutive = 0;
        }

        prev_ir_average = ir_average;

        sprintf(message, "%d\t%d\tscan\r\n", i, ir_average);
        uart_sendStr(message);
    }
    result->ir_average = total_ir / config->range;
}

int main(void) {
    oi_t *sensor_data = oi_alloc();
    oi_init(sensor_data);

    uart_interrupt_init();
    lcd_init();
    timer_init();
    cyBOT_init_Scan(0b0111);

    right_calibration_value = 253750;
    left_calibration_value = 1235500;

    while (1) {
        if (received_char == 's') {
            uint16_t i;
            scan_config config = {181, 0, 180, 1, 1}; //range, start, end, increment, number of measures per angle

            scan_field2(&config, &result);
            if (received_char == 'q') continue; //don't do calculations if scan is quit

            double tolerance = 0.9; //how sensitive to objects, lower is more sensitive

            int object_tolerance = 3; //number of consecutive values for something to be an object
            int consecutive = 0; //number of consecutive values above threshold
            int object_id = 1; //id of object

            //create an array of object to add objects to and set values to 0
            field_object field_objects[NUM_OBJECTS] = {0};

            //maybe add edge detection based on spikes
            for (i = 0; i < config.range / config.increment; i++) {
                if (result.ir_scan_vals[i] > (tolerance * result.ir_average) && result.ping_scan_vals[i] < 100) {
                    consecutive++;
                } else {
                    if (consecutive >= object_tolerance) {
                        field_object temp;
                        temp.id = object_id++;
                        temp.angle = i - (consecutive / 2);
                        temp.distance = result.ping_scan_vals[temp.angle];
                        temp.rad_width = consecutive * config.increment;
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
                    uart_sendStr("OBJECT\n");
                    uart_sendStr(message);
                    cyBOT_Scan(o.angle, &scan);
                    timer_waitMillis(2000);
                }
            }

            uart_sendStr("END\n");

            received_char = '\0';
        }
    }
}
