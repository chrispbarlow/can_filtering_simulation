/***********************************************************************************************************
 * CAN_Rx_global.c
 *
 * Global CAN message receive buffers and control variables
 *
 *  Created on: 7 Mar 2013
 *      Author: chris.barlow
 * *********************************************************************************************************/

#ifndef CAN_RX_GLOBAL_H_
#define CAN_RX_GLOBAL_H_

#define NUM_MAILBOXES_MAX 	(32)
#define NUM_MESSAGES_MAX 	(64)

#include <DSP2833x_Device.h>

typedef enum{INIT,RUN,RESET,UPDATE} updateFlags_t;
extern updateFlags_t updateSequenceRequired_G;

/*******************************************************************************
 * CAN Rx Message array
 * Assigns messages to logging sequence position
 * *****************************************************************************/

/* If we were doing anything with the CAN data received, the individual bytes of the message could be accessed from here */
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


typedef struct {
	Uint32 canID;			/* CAN ID */
	canData_t canData;		/* CAN Data */
	Uint16 canDLC;			/* Message byte length */
	int16 timer;			/* Used for message scheduling */
	int16 timer_reload;
	Uint32 counter;			/* Used to track message hits */
} canRxMessage_t;

extern canRxMessage_t CAN_RxMessages_G[];

/*******************************************************************************/

/*******************************************************************************
 * The filterShadow holds a copy of the current ID - mailbox mapping.
 * Necessary because we can't read the mailbox ID directly from the registry
 * *****************************************************************************/
typedef struct{
	Uint32 canID_mapped;				/* CAN ID currently being held in mailbox */
	Uint16 sequenceIndex_mapped;		/* Index of CAN_RxMessages[] mapped to mailbox */
}filterShadow_t;

extern filterShadow_t mailBoxFilterShadow_G[];

/*******************************************************************************/

/* Control values set dynamically when logging list is received */
extern Uint16 numRxCANMsgs_G;
extern Uint16 filterSize_G;

#endif /* CAN_RX_GLOBAL_H_ */
