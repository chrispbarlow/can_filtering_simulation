/*
 * readADC.c
 *
 *  Created on: 25 Mar 2013
 *      Author: chris.barlow
 */
#include "readADC.h"
#include <stdio.h>
#include "../../CAN_Exchange/CAN_Tx_global.h"


Uint16 adcValue = 0xFFFF;
Uint16 adcValue0 = 0xFFFF, adcValue1 = 0xFFFF, adcValue2 = 0xFFFF, adcValue3 = 0xFFFF, adcValue4 = 0xFFFF, adcValue5 = 0xFFFF;

void readADC_init(void)
{
	ADC_Start();
	/* Task - specific Initialisation code */
	ADC_ConfigurePort(ADCINA0, 0);				/* Configure ADCINA0 to port 0 in sequencer */
	ADC_ConfigurePort(ADCINA0, 1);				/* Configure ADCINA0 to port 0 in sequencer */
	ADC_ConfigurePort(ADCINA0, 2);				/* Configure ADCINA0 to port 0 in sequencer */
	ADC_ConfigurePort(ADCINA0, 3);				/* Configure ADCINA0 to port 0 in sequencer */
	ADC_ConfigurePort(ADCINA0, 4);				/* Configure ADCINA0 to port 0 in sequencer */
	ADC_ConfigurePort(ADCINA0, 5);				/* Configure ADCINA0 to port 0 in sequencer */

}

void readADC_update(void)
{
	static Uint16 adcPrev = 0x0000, adcDif = 0x0000, adcCounter = 1;

	ADC_UpdateState();
	if(ADC_CheckPortState(0) == ADC_PORT_PENDING)
	{
		/*Read adc port */
		adcValue0 = ADC_ReadPort(0);
		adcValue1 = ADC_ReadPort(1);
		adcValue2 = ADC_ReadPort(2);
		adcValue3 = ADC_ReadPort(3);
		adcValue4 = ADC_ReadPort(4);
		adcValue5 = ADC_ReadPort(5);

		adcValue = (adcValue0+adcValue1+adcValue2+adcValue3+adcValue4+adcValue5)/6;

		if(adcValue <= 0x000E)
		{
			adcValue = 0x0000;
		}
		else if(adcValue >= 0x0FF0)
		{
			adcValue = 0x0FFF;
		}
		else if(adcValue>adcPrev)
		{
			adcDif = adcValue-adcPrev;
		}
		else
		{
			adcDif = adcPrev-adcValue;
		}

		if((adcDif > 0x0010)||(++adcCounter == 0)||(adcValue == 0x0000)||(adcValue == 0x0FFF))
		{
			canData_Tx_7AB.dataVars.DB7_6 = adcValue;
			adcPrev = adcValue;
		}
		canData_Tx_7AB.dataVars.DB5_4 = adcValue0;
	}
	else
	{
		/*ADC still sequencing */
	}
}
