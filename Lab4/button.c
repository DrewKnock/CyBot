/*
 * button.c
 *
 *  Created on: Jul 18, 2016
 *      Author: Eric Middleton, Zhao Zhang, Chad Nelson, & Zachary Glanz.
 *
 *  @edit: Lindsey Sleeth and Sam Stifter on 02/04/2019
 *  @edit: Phillip Jones 05/30/2019: Merged Spring 2019 version with Fall 2018
 *  @edit: Diane Rover 02/01/20: Corrected comments about ordering of switches for new LCD board and added busy-wait on PRGPIO
 */



//The buttons are on PORTE 3:0
// GPIO_PORTE_DATA_R -- Name of the memory mapped register for GPIO Port E,
// which is connected to the push buttons
#include "button.h"


/**
 * Initialize PORTE and configure bits 0-3 to be used as inputs for the buttons.
 */
void button_init() {
	static uint8_t initialized = 0;

	//Check if already initialized
	if(initialized){
		return;
	}

	// delete warning after implementing
	#warning "Unimplemented function: void button_init()"

	// Reading: To initialize and configure GPIO PORTE, visit pg. 656 in the
	// Tiva datasheet.

	// Follow steps in 10.3 for initialization and configuration. Some steps
	// have been outlined below.

	// Ignore all other steps in initialization and configuration that are not
	// listed below. You will learn more about additional steps in a later lab.

	// 1) Turn on PORTE system clock, do not modify other clock enables
	SYSCTL_RCGCGPIO_R |= 0x10;
	// You may need to add a delay here of several clock cycles for the clock to start, e.g., execute a simple dummy assignment statement, such as "long delay = SYSCTL_RCGCGPIO_R".
  // Instead, use the PRGPIO register and busy-wait on the peripheral ready bit for PORTE.
	while ((SYSCTL_PRGPIO_R & 0x10) == 0) {};
	// 2) Set the buttons as inputs, do not modify other PORTE wires
	GPIO_PORTE_DIR_R &= ~0xF; //Shortened 0xFFFFFFF0 to ~0xF

	// 3) Enable digital functionality for button inputs,
	//    do not modify other PORTE enables
	GPIO_PORTE_DEN_R |= 0xF;

	initialized = 1;
}

/**
 * Returns the position of the rightmost button being pushed.
 * @return the position of the rightmost button being pushed. 1 is the leftmost button, 4 is the rightmost button.  0 indicates no button being pressed
 */
uint8_t button_getButton() {

	#warning "Unimplemented function: uint8_t button_getButton(void)"	// delete warning after implementing

	// TODO: Write code below -- Return the rightmost button position pressed

    //Check each button from rightmost (MSB) to leftmost (LSB)
	if (((GPIO_PORTE_DATA_R >> 3) & 1) == 0) {
	    return 4;
	} else if (((GPIO_PORTE_DATA_R >> 2) & 1) == 0) {
	    return 3;
	} else if (((GPIO_PORTE_DATA_R >> 1) & 1) == 0) {
        return 2;
    } else if ((GPIO_PORTE_DATA_R & 1) == 0) {
        return 1;
    }

	//If no button is pressed, return 0
	return 0;
}
