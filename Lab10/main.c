#include "Timer.h"
#include "lcd.h"
#include "servo.h"

int main(void) {
    timer_init();
    servo_init();

    while(1) {
        uint16_t i;
        for (i = 0; i <= 180; i += 2) {
            servo_move(i);
        }
    }
}
