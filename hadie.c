/* hadie - High Altitude Balloon flight software              */
/*============================================================*/
/* Copyright (C)2010 Philip Heron <phil@sanslogic.co.uk>      */
/*                                                            */
/* This program is distributed under the terms of the GNU     */
/* General Public License, version 2. You may use, modify,    */
/* and redistribute it under the terms of this license. A     */
/* copy should be included with this source.                  */

#include "config.h"
#include <avr/interrupt.h>
#include "rtty.h"
#include "rs8.h"

int main(void)
{
	/* Connections: (PDIP)
	 * 
	 *  1: PB0      - Output, RTTY space
	 *  2: PB1      - Output, RTTY Mark
	 *  3: PB2      - Enable Radio
	 *  5: PB4      - To pin 1 of MMC card (/CS)
	 *  6: PB5/MOSI - To pin 4 of ISP header
	 *              - To pin 2 of MMC card
	 *  7: PB6/MISO - To pin 1 of ISP header
	 *                To pin 7 of MMC card
	 *  8: PB7/SCK  - To pin 3 of ISP header
	 *                To pin 5 of MMC card
	 *  9: RESET    - To pin 5 of ISP header
	 * 10: VCC      - 3.3v
	 * 11: GND      - GND
	 * 12: XTAL2    - 7.3728 MHz crystal
	 * 13: XTAL1    - 7.3728 MHz crystal
	 * 14: PD0/RXD0 - Input, UART camera
	 * 15: PD1/TXD0 - Output, UART camera
	 * 16: PD2/RXD1 - Input, GPS data
	 * 17: PD3/TXD1 - Output, not connected - GPS receive only
	 * 30: AVCC     - 3.3v
	 * 31: GND      - GND
	*/
	
	rtx_init();
		
	/* Start interrupts and enter main loop */
	sei();
	while(1)
	{
		
	}
	
	return(0);
}

