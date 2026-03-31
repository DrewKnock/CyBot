#include "Timer.h"
#include "lcd.h"
#include <stdint.h>
#include "adc.h"
#include <math.h>

int main(void) {
    timer_init();
    lcd_init();
    adc_init();

    char message[20] = "";
    while (1) {
        uint8_t i;
        uint8_t num_samples = 16;

        uint16_t total_value = 0;

        for (i = 0; i < num_samples; i++) {
            total_value += adc_read();
        }
        uint16_t value = total_value / num_samples;
        double distance = 1 / ((6.27e-5 * value) + -0.0496);

        lcd_clear();
        sprintf(message, "ADC:%d", value);
        lcd_puts(message);
        lcd_gotoLine(2);
        sprintf(message, "Distance:%.2lf", distance);
        lcd_puts(message);
        timer_waitMillis(500);
    }
}

