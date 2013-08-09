/*
 * CAN_Rx_global.h
 *
 *  Created on: 4 Mar 2013
 *      Author: chris.barlow
 */

#ifndef CAN_RX_GLOBAL_H_
#define CAN_RX_GLOBAL_H_

#define NUM_MAILBOXES_MAX (32)

#include <DSP2833x_Device.h>

typedef enum{INIT,RUN,RESET,UPDATE} updateFlags_t;
extern updateFlags_t updateSequenceRequired_G;


/* Specify Byte Variables for CAN Message */
struct CAN_DATA_VARS_RX{

	/*byte variable examples */
	Uint16 DB3:8;
	Uint16 DB2:8;
	Uint16 DB1:8;
	Uint16 DB0:8;

	Uint16 DB7:8;
	Uint16 DB6:8;
	Uint16 DB5:8;
	Uint16 DB4:8;
};

/* Map CAN message Bytes with raw data array (2 x 32bit array) */
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

/* The filterShadow holds a copy of the current ID - mailbox mapping.
 * Necessary because we can't read the mailbox ID directly from the registry
 */

typedef struct{
	Uint32 canID;
	Uint16 messagePointer;
}filterShadow_t;

extern filterShadow_t mailBoxFilters_G[];


extern Uint16 numRxCANMsgs_G;
extern Uint16 filterSize_G;
#endif /* CAN_RX_GLOBAL_H_ */
