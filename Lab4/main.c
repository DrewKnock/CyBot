#include "button.h"
#include "Timer.h"
#include "lcd.h"
#include "cyBot_uart.h"

void send_string(char *str) {
    int i;
    //send string one char at a time
    for (i = 0; str[i] != '\0'; i++) {
        cyBot_sendByte(str[i]);
    }
}

 void button_messages(void) {
     while (1) {
       //get the value of the button being pressed, if any
        uint8_t button = button_getButton();

        //go through each case and send a different message to putty, if none hit then the loop just continues
        switch(button) {
            case 1:
                send_string("Hello\r\n");
                timer_waitMillis(1000);
                break;
            case 2:
                send_string("Goodbye\r\n");
                timer_waitMillis(1000);
                break;
            case 3:
                send_string("Hello Again\r\n");
                timer_waitMillis(1000);
                break;
            case 4:
                send_string("Goodbye Again\r\n");
                timer_waitMillis(1000);
                break;
        }

    }
 }

 void show_button_pressed() {
     while (1) {
         uint8_t button = button_getButton();

         if (button > 0) {
             lcd_printf("Button %u has been pressed", button);
            // timer_waitMillis(5000);
         } else {
             lcd_printf("no button has been pressed");
         }
     }
 }

int main(void) {
    //initialize buttons and uart connection
    button_init();
    timer_init();
    lcd_init();
    cyBot_uart_init();

    button_messages();

    return 0;
}
