/*
 * ADC.c
 *
 *	ADC library optimised for TT scheduler
 *
 *  Created on: 22 Mar 2013
 *      Author: chris.barlow
 *
 */

#include "ADC.h"

typedef struct {
	adcPortStates_t portState;
	volatile Uint16 * value;
} adcPorts_t;

adcPorts_t adcPorts[16]={
		{ ADC_PORT_UNUSED, &AdcRegs.ADCRESULT0 	},
		{ ADC_PORT_UNUSED, &AdcRegs.ADCRESULT1 	},
		{ ADC_PORT_UNUSED, &AdcRegs.ADCRESULT2 	},
		{ ADC_PORT_UNUSED, &AdcRegs.ADCRESULT3 	},
		{ ADC_PORT_UNUSED, &AdcRegs.ADCRESULT4 	},
		{ ADC_PORT_UNUSED, &AdcRegs.ADCRESULT5 	},
		{ ADC_PORT_UNUSED, &AdcRegs.ADCRESULT6 	},
		{ ADC_PORT_UNUSED, &AdcRegs.ADCRESULT7 	},
		{ ADC_PORT_UNUSED, &AdcRegs.ADCRESULT8 	},
		{ ADC_PORT_UNUSED, &AdcRegs.ADCRESULT9 	},
		{ ADC_PORT_UNUSED, &AdcRegs.ADCRESULT10 },
		{ ADC_PORT_UNUSED, &AdcRegs.ADCRESULT11 },
		{ ADC_PORT_UNUSED, &AdcRegs.ADCRESULT12 },
		{ ADC_PORT_UNUSED, &AdcRegs.ADCRESULT13 },
		{ ADC_PORT_UNUSED, &AdcRegs.ADCRESULT14 },
		{ ADC_PORT_UNUSED, &AdcRegs.ADCRESULT15 }
};

adcStates_t portState_Master = ADC_OFF;
static Uint16 numberOfPortsInUse = 0;


void ADC_Init(void)
{
    extern void DSP28x_usDelay(Uint32 Count);


    /*	IMPORTANT*
		The ADC_cal function, which  copies the ADC calibration values from TI reserved
		OTP into the ADCREFSEL and ADCOFFTRIM registers, occurs automatically in the
		Boot ROM. If the boot ROM code is bypassed during the debug process, the
		following function MUST be called for the ADC to function according
		to specification. The clocks to the ADC MUST be enabled before calling this
		function.
		See the device data manual and/or the ADC Reference
		Manual for more information.
	*/
	    EALLOW;
		SysCtrlRegs.PCLKCR0.bit.ADCENCLK = 1;
		ADC_cal();
		EDIS;




}

void ADC_Start(void)
{
    /* 	To powerup the ADC the ADCENCLK bit should be set first to enable
		clocks, followed by powering up the bandgap, reference circuitry, and ADC core.
		Before the first conversion is performed a 5ms delay must be observed
		after power up to give all analog circuits time to power up and settle

		Please note that for the delay function below to operate correctly the
		CPU_RATE define statement in the DSP2833x_Examples.h file must
		contain the correct CPU clock period in nanoseconds.
	*/
	if(portState_Master == ADC_OFF)
	{
		AdcRegs.ADCTRL3.all = 0x00E0;  					/* 	Power up bandgap/reference/ADC circuits		*/

		AdcRegs.ADCTRL1.bit.ACQ_PS = ADC_SHCLK;			/* 	Acquisition window size. This bit field controls the width of SOC pulse, which, in turn,
															determines for what time duration the sampling switch is closed. The width of SOC pulse
															is ADCTRL1[11:8] + 1 times the ADCLK period.
														*/
		AdcRegs.ADCTRL3.bit.ADCCLKPS = ADC_CKPS;		/*	Core clock divider. 28x peripheral clock, HSPCLK, is divided by 2*ADCCLKPS[3-0],
															except when ADCCLKPS[3-0] is 0000, in which case HSPCLK is directly passed on. The
															divided clock is further divided by ADCTRL1[7]+1 to generate the core clock, ADCLK.
														*/
		AdcRegs.ADCTRL1.bit.SEQ_CASC 		= 0x1;      /* 	1 = Cascaded mode 		*/
		AdcRegs.ADCTRL1.bit.SUSMOD 			= 0x3;		/* 	1 = Continuous run mode */
		AdcRegs.ADCTRL1.bit.CONT_RUN 		= 0x0;		/* 	1 = Continuous run mode */

		AdcRegs.ADCTRL2.bit.INT_ENA_SEQ1 	= 0x0;		/* 	Disable all interrupts and input signals */
		AdcRegs.ADCTRL2.bit.EPWM_SOCB_SEQ 	= 0x0;
		AdcRegs.ADCTRL2.bit.EPWM_SOCA_SEQ1 	= 0x0;
		AdcRegs.ADCTRL2.bit.EXT_SOC_SEQ1 	= 0x0;

		AdcRegs.ADCTRL2.bit.INT_ENA_SEQ2 	= 0x0;
		AdcRegs.ADCTRL2.bit.EPWM_SOCB_SEQ2 	= 0x0;

		portState_Master = ADC_DISABLED;
	}
}

/*
 * Link adcPort to a portNumber (0 - 15)
 * Ports are sampled in order from 0 to 15.
 * Physical ports can be mapped to more than one portNumber, allowing for multiple samples
 *
 * Returns port configured or -1 if portNumber was out of range (> 15) or -2 if all possible ports have been configured.
 */
int16 ADC_ConfigurePort(Uint16 adcPort, int16 portNumber)
{

	if(numberOfPortsInUse < 16)
	{
		switch(portNumber)
		{
		case(0):
			AdcRegs.ADCCHSELSEQ1.bit.CONV00 = adcPort&(0xF);
			break;
		case(1):
			AdcRegs.ADCCHSELSEQ1.bit.CONV01 = adcPort&(0xF);
			break;
		case(2):
			AdcRegs.ADCCHSELSEQ1.bit.CONV02 = adcPort&(0xF);
			break;
		case(3):
			AdcRegs.ADCCHSELSEQ1.bit.CONV03 = adcPort&(0xF);
			break;
		case(4):
			AdcRegs.ADCCHSELSEQ2.bit.CONV04 = adcPort&(0xF);
			break;
		case(5):
			AdcRegs.ADCCHSELSEQ2.bit.CONV05 = adcPort&(0xF);
			break;
		case(6):
			AdcRegs.ADCCHSELSEQ2.bit.CONV06 = adcPort&(0xF);
			break;
		case(7):
			AdcRegs.ADCCHSELSEQ2.bit.CONV07 = adcPort&(0xF);
			break;
		case(8):
			AdcRegs.ADCCHSELSEQ3.bit.CONV08 = adcPort&(0xF);
			break;
		case(9):
			AdcRegs.ADCCHSELSEQ3.bit.CONV09 = adcPort&(0xF);
			break;
		case(10):
			AdcRegs.ADCCHSELSEQ3.bit.CONV10 = adcPort&(0xF);
			break;
		case(11):
			AdcRegs.ADCCHSELSEQ3.bit.CONV11 = adcPort&(0xF);
			break;
		case(12):
			AdcRegs.ADCCHSELSEQ4.bit.CONV12 = adcPort&(0xF);
			break;
		case(13):
			AdcRegs.ADCCHSELSEQ4.bit.CONV13 = adcPort&(0xF);
			break;
		case(14):
			AdcRegs.ADCCHSELSEQ4.bit.CONV14 = adcPort&(0xF);
			break;
		case(15):
			AdcRegs.ADCCHSELSEQ4.bit.CONV15 = adcPort&(0xF);
			break;
		default:
			portNumber = -1;
			break;
		}
	}
	else
	{
		portNumber = -2;
	}

	if(portNumber >= 0)
	{
		AdcRegs.ADCMAXCONV.bit.MAX_CONV1 = (numberOfPortsInUse & 0x000F);
		numberOfPortsInUse++;
		adcPorts[portNumber].portState = ADC_PORT_IDLE;
		portState_Master = ADC_IDLE;
	}

	return portNumber;
}


/* Updates ADC state machine.
 * Call periodically at least as often as ADC ports are read.
 * Returns latest state.
 */
adcStates_t ADC_UpdateState(void)
{
	Uint16 idleCount = 0, i = 0;

	switch(portState_Master)
	{

	/* ADC_IDLE only used the first time the ADC is used */
	case ADC_IDLE:
		/* Check all expected ports are have been read */
		for(i = 0; i < 16; i++)
		{
			if(adcPorts[i].portState == ADC_PORT_IDLE)
			{
				idleCount++;
			}
		}

		if(idleCount == numberOfPortsInUse)
		{
			AdcRegs.ADCTRL2.bit.RST_SEQ1 = 1; 	/* Starts autosequencer */
			AdcRegs.ADCTRL2.bit.SOC_SEQ1 = 1; 	/* Starts autosequencer */
			portState_Master = ADC_BUSY;
		}
		break;

	case ADC_BUSY:
		if(AdcRegs.ADCST.bit.SEQ1_BSY == 0)		/* Autosequencer finished */
		{
			for(i = 0; i < 16; i++)
			{
				if(adcPorts[i].portState != ADC_PORT_UNUSED)
				{
					adcPorts[i].portState = ADC_PORT_PENDING;
				}
			}

			portState_Master = ADC_IDLE;
		}

		break;

	case ADC_OFF:
	case ADC_DISABLED:
	default:
		break;
	}

	return portState_Master;
}

adcPortStates_t ADC_CheckPortState(Uint16 portNumber)
{
	adcPortStates_t portState = ADC_PORT_UNUSED;

	if(portNumber < 16)
	{
		portState = adcPorts[portNumber].portState;
	}

	return portState;
}

/* Reads specified ADC port. Returns 0xFFFF if port could not be read */
Uint16 ADC_ReadPort(Uint16 portNumber)
{
	Uint16 adcValue = 0xFFFF;

	if(adcPorts[portNumber].portState == ADC_PORT_PENDING)
	{
		adcValue = (*adcPorts[portNumber].value)>>4;
		adcPorts[portNumber].portState = ADC_PORT_IDLE;
	}

	return adcValue;
}

/* Resets state of all used ports to ADC_READ, forcing new autosequence to start */
void ADC_ResetPorts(void)
{
	Uint16 i;

	for(i = 0; i < 16; i++)
	{
		if(adcPorts[i].portState != ADC_PORT_UNUSED)
		{
			adcPorts[i].portState = ADC_PORT_IDLE;
		}
	}
}
