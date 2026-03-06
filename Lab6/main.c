/**
 * lab6_template.c
 *
 * Template file for CprE 288 Lab 6
 *
 * @author Diane Rover, 2/15/2020
 *
 */

#include "Timer.h"
#include "lcd.h"
#include "cyBot_Scan.h"  // For scan sensors
#include "uart-interrupt.h"
#include <stdbool.h>
#include "driverlib/interrupt.h"

// Uncomment or add any include directives that are needed
#include "open_interface.h"
#include "movement.h"
#include "button.h"

#define REPLACEME 0

int main(void) {
    timer_init(); // Must be called before lcd_init(), which uses timer functions
    lcd_init();
    uart_interrupt_init();
    cyBOT_init_Scan(0b0111);

    command_byte = 'a';

    while(1)
    {

    }

}
