#include "Timer.h"
#include "lcd.h"
#include "ping.h"

int main(void) {
    timer_init(); // Must be called before lcd_init(), which uses timer functions
    lcd_init();
    ping_init();

    // YOUR CODE HERE
    //char message[64] = "";
    uint32_t clock_width = 0;
    uint16_t overflow_count = 0;

    while(1)
    {

      ping_trigger();

      while(g_state != DONE) {};

      lcd_clear();

      clock_width = g_start_time - g_end_time;
      if (g_start_time < g_end_time) {
          lcd_puts("OVERFLOW");
          overflow_count++;
      } else {
          lcd_printf("Clock Width: %d\nDistance(cm): %.2lf\nOverflow: %d", clock_width, ping_getDistanceFromWidth(clock_width), overflow_count);
      }
      //lcd_puts(message);

      timer_waitMillis(500);
      g_state = LOW;

    }

}
