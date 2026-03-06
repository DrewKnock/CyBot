#include "button.h"
#include "Timer.h"
#include "lcd.h"
#include "cyBot_uart.h"
#include "uart.h"

int main(void) {
    button_init();
    timer_init();
    lcd_init();
    cyBot_uart_init_clean();
    uart_init();

    while (1) {
        lcd_clear();
        int buffer_size = 20;
        char buffer[buffer_size + 1];
        buffer[buffer_size] = '\0';

        int i;
        int early_break = 0;
        for (i = 0; i < buffer_size; i++) {
            char current_char = uart_receive();

            uart_sendChar(current_char);

            if (current_char == '\r') {
                early_break = 1;
                uart_sendChar('\n');
                buffer[i] = '\0';
                break;
            }
            buffer[i] = current_char;

            lcd_putc(current_char);
            lcd_setCursorPos(0, 1);

            char numChars[2];
            sprintf(numChars, "%d", i + 1);
            lcd_puts(numChars);
            lcd_setCursorPos(i + 1, 0);
        }

        if (!early_break) uart_sendStr("\r\n");
        lcd_clear();
        lcd_puts(buffer);
        lcd_home();
    }

    return 0;
}
