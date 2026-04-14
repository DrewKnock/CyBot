/*
 * servo.c
 *
 * Created on: Apr 10, 2026
 * Authors: Andy Knockel and Luc Johnson
 */

#include "servo.h"
#include "Timer.h"

void servo_init(void) {
    SYSCTL_RCGCTIMER_R |= 0x02;
    SYSCTL_RCGCGPIO_R |= 0x02;

    while((SYSCTL_PRTIMER_R & 0x2) == 0) {};
    while((SYSCTL_PRGPIO_R & 0x2) == 0) {};

    GPIO_PORTB_DEN_R   |= 0x20;
    GPIO_PORTB_DIR_R  &= ~0x20;

    GPIO_PORTB_AFSEL_R |= 0x20;
    GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R & ~0xF00000) | 0x700000;

    TIMER1_CTL_R &= ~0x100;
    TIMER1_CFG_R |= 0x4;
    TIMER1_TBMR_R = (TIMER1_TBMR_R & ~0xF) | 0xA;

    TIMER1_CTL_R &= ~0x4000;
    TIMER1_TBPR_R = 0x04;
    TIMER1_TBILR_R = 0xE200;

    //default match makes timer high for 1.5ms which should roughly be 90 degrees
    TIMER1_TBMATCHR_R = 0x8440;
    TIMER1_TBPMR_R = 0x4;

    TIMER1_CTL_R |= 0x100;
}

static uint16_t current_deg = 90;

void servo_move(uint16_t degrees) {
    //Don't let servo go past 180 or 0
    if(degrees > 180) degrees = 180;

    //0 degrees is 16000 cycles and 180 degrees is 32000 cycles so range between is 16000
    //16,000 cycles over 180 degrees is roughly 88.88 per degree
    uint32_t high_pulse_cycles = 16000 + (uint32_t)(degrees * 88.88);

    //period is 20ms which is 320,000 cycles
    uint32_t match_value = 320000 - high_pulse_cycles;

    //Set the lower 16 bits
    TIMER1_TBMATCHR_R = (match_value & 0xFFFF);
    //set the upper 8 bits
    TIMER1_TBPMR_R = (match_value >> 16) & 0xFF; //shift bits 23-16 to position 7-0 and set the prescale to that value

    // 5ms per degree + default delay (we will adjust to see what's needed)
    uint8_t default_delay = 100;
    uint32_t movement_delay = (abs(degrees - current_deg) * 5) + default_delay;
    timer_waitMillis(movement_delay);

    //make current_deg what was just sent
    current_deg = degrees;
}
