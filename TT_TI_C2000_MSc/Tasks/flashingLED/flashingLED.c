/*
 * flashingLED.c
 *
 *  Created on: 06 Mar 2013
 *      Author: chris.barlow
 */
#include "flashingLED.h"
#include "../../global.h"
#include "../../CAN_Exchange/CAN_Rx_Global.h"

void flashingLED_init(void)
{
	EALLOW;
	/* Enable an GPIO output on GPIO2, set it high */
	GpioCtrlRegs.GPAPUD.bit.GPIO2 = 0;   		/* Enable pullup on GPIO2 	*/
	GpioDataRegs.GPASET.bit.GPIO2 = TRUE;   	/* Load output latch 		*/
	GpioCtrlRegs.GPAMUX1.bit.GPIO2 = 0;  		/* GPIO2 = GPIO2 			*/
	GpioCtrlRegs.GPADIR.bit.GPIO2 = 1;   		/* GPIO2 = output			*/
	EDIS;

}

void flashingLED_update(void)
{
	GpioDataRegs.GPATOGGLE.bit.GPIO2 = TRUE;
}
