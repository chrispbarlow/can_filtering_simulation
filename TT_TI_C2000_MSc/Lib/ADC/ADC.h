/*
 * ADC.h
 *
 *	ADC library optimised for TT scheduler
 *
 *  Created on: 22 Mar 2013
 *      Author: chris.barlow
 */

#ifndef ADC_H_
#define ADC_H_

#include "DSP28x_Project.h"     // Device Headerfile and Examples Include File
#include "DSP2833x_Device.h"     // DSP2833x Headerfile Include File

// ADC start parameters
#if (CPU_FRQ_150MHZ)     // Default - 150 MHz SYSCLKOUT
  #define ADC_MODCLK 0x3 // HSPCLK = SYSCLKOUT/2*ADC_MODCLK2 = 150/(2*3)   = 25.0 MHz
#endif
#if (CPU_FRQ_100MHZ)
  #define ADC_MODCLK 0x2 // HSPCLK = SYSCLKOUT/2*ADC_MODCLK2 = 100/(2*2)   = 25.0 MHz
#endif

#define ADC_CKPS   0x01   // ADC module clock = HSPCLK/2*ADC_CKPS   = 25.0MHz/(1*2) = 12.5MHz
#define ADC_SHCLK  0x0f   // S/H width in ADC module periods                        = 16 ADC clocks
#define ZOFFSET    0x00  // Average Zero offset
#define BUF_SIZE   2048  // Sample buffer size
#define ADC_usDELAY  5000L

#define ADCINA0	(0x0)
#define ADCINA1	(0x1)
#define ADCINA2	(0x2)
#define ADCINA3	(0x3)
#define ADCINA4	(0x4)
#define ADCINA5	(0x5)
#define ADCINA6	(0x6)
#define ADCINA7	(0x7)
#define ADCINB0	(0x8)
#define ADCINB1	(0x9)
#define ADCINB2	(0xA)
#define ADCINB3	(0xB)
#define ADCINB4	(0xC)

typedef enum {ADC_PORT_UNUSED, ADC_PORT_IDLE, ADC_PORT_PENDING} adcPortStates_t;
typedef enum {ADC_OFF, ADC_DISABLED, ADC_IDLE, ADC_BUSY} adcStates_t;

void ADC_Init(void);
void ADC_Start(void);
int16 ADC_ConfigurePort(Uint16 adcPort, int16 portNumber);
adcStates_t ADC_UpdateState();
adcPortStates_t ADC_CheckPortState(Uint16 portNumber);
Uint16 ADC_ReadPort(Uint16 portNumber);
void ADC_ResetPorts(void);


#endif /* ADC_H_ */
