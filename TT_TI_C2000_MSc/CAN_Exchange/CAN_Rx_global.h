/*
 * CAN_Rx_global.h
 *
 *  Created on: 4 Mar 2013
 *      Author: chris.barlow
 */

#ifndef CAN_RX_GLOBAL_H_
#define CAN_RX_GLOBAL_H_


#include <DSP2833x_Device.h>

typedef enum{RUN,RESET,UPDATE} updateFlags_t;
extern updateFlags_t updateSequenceRequired_G;


/******************************************
 * The structures below define the CAN variables and their position in their respective CAN message
 *
 * CAN message is transmitted (and viewed in CAN trace) as B0 B1 B2 B3 B4 B5 B6 B7
 *
 * Byte variables defined in the order:
 *  	B3:8;
 *  	B2:8;
 *  	B1:8;
 *  	B0:8;
 *
 *  	B7:8;
 *  	B6:8;
 *  	B5:8;
 *  	B4:8;
 *
 * bit variables for each Byte defined in the order:
 * 		b0:1;
 * 		b1:1;
 * 		b2:1;
 * 		b3:1;
 * 		b4:1;
 * 		b5:1;
 * 		b6:1;
 * 		b7:1;
 *******************************************************************************/

/* CAN Rx Messages */

/*******************************************************************************/
/* Rx Message ID 0x7AB (template)*/
/*******************************************************************************/

/* Specify variables for CAN Message */
struct CAN_DATA_VARS_RX{

	/*byte variable examples */
	Uint16 DB3:8;
	Uint16 DB2:8;
	Uint16 DB1:8;
	Uint16 Db0:8;

	Uint16 DB7:8;
	Uint16 DB6:8;
	Uint16 DB5:8;
	Uint16 Db4:8;
};

/* Join CAN message with raw data array (2 x 32bit array) */
typedef union {
	Uint32 rawData[2];
	struct CAN_DATA_VARS_RX dataBytes;
}canData_t;


/*******************************************************************************/


/*******************************************************************************
 * CAN Rx Message array
 * Assigns messages to mailbox position (array index + CAN_RX_MBOFFSET)
 * *****************************************************************************/
typedef struct {
	Uint32 canID;			/* CAN ID */
	canData_t canData;		/* CAN Data */
	Uint16 canDLC;			/* Message byte length */
	int16 timer;
	int16 timer_reload;
	Uint32 counter;
} canRxMessage_t;

extern canRxMessage_t CAN_RxMessages_G[];

typedef struct{
	Uint32 canID;
	Uint16 messagePointer;
}filterShadow_t;

extern filterShadow_t mailBoxFilters[];


extern Uint16 numRxCANMsgs_G;
extern Uint16 filterSize_G;
#endif /* CAN_RX_GLOBAL_H_ */
