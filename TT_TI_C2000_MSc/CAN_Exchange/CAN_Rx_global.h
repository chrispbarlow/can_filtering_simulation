/*
 * CAN_Rx_global.h
 *
 *  Created on: 4 Mar 2013
 *      Author: chris.barlow
 */

#ifndef CAN_RX_GLOBAL_H_
#define CAN_RX_GLOBAL_H_

#include <DSP2833x_Device.h>

typedef struct {
	Uint32 canID;			/* CAN ID */
	Uint32* canData;		/* CAN Data */
	Uint16 canDLC;			/* Message byte length */
} canRxMessage_t;

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
struct CAN_DATA_VARS_RX_7AB{

	/*byte variable examples */
	Uint16 DB3:8;
	Uint16 DB2:8;
	Uint16 DB1:8;

	/*bit variable examples */
	Uint16 Db0_0:1;
	Uint16 Db0_1:1;
	Uint16 Db0_2:1;
	Uint16 Db0_3:1;
	Uint16 Db0_4:1;
	Uint16 Db0_5:1;
	Uint16 Db0_6:1;
	Uint16 Db0_7:1;

	/* 16 bit variable examples */
	Uint16 DB7_6;						/* Low byte: DB7, High byte: DB6 */
	Uint16 DB5_4;
};

/* Join CAN message with raw data array (2 x 32bit array) */
union CAN_DATA_RX_7AB{
	Uint32 rawData[2];
	struct CAN_DATA_VARS_RX_7AB dataVars;
};
extern union CAN_DATA_RX_7AB canData_Rx_7AB;

/* Assign CAN ID to message */
extern canRxMessage_t canMessage_Rx_7AB;


/*******************************************************************************/
/* Rx Message ID 0x651 */
/*******************************************************************************/

/* Specify variables for CAN Message */
struct CAN_DATA_VARS_RX_651{

	/* Byte 3 */
	Uint32 vs_state:8;				/*B3*/

	/* Byte 2 */
	/* Byte 1 */
	/* Byte 0 */
	Uint32 vs_xcount:24;			/*B2,B1,B0*/

	/* Byte 7 */
	Uint16 VSO_TEN:1;
	Uint16 Db7_1:1;					/*B7,b1 reserved*/
	Uint16 Db7_2:1;					/*B7,b2 reserved*/
	Uint16 Db7_3:1;					/*B7,b3 reserved*/
	Uint16 Db7_4:1;					/*B7,b4 reserved*/
	Uint16 VSO_WPMP:1;
	Uint16 VSO_WFAN:1;
	Uint16 VSO_PASINVSS:1;

	/* Byte 6 */
	Uint16 VSO_COMPINVSS:1;
	Uint16 VSO_PASINVRST:1;
	Uint16 VSO_COMPINRST:1;
	Uint16 VSO_DCDC:1;
	Uint16 VSO_BRKLAMP:1;
	Uint16 VSO_HVOK1:1;
	Uint16 VSO_HVOK2:1;
	Uint16 Db6_7:1;					/*B6,b7 reserved*/

	/* Byte 5 */
	Uint16 VSI_DRVEN:1;
	Uint16 VSI_BCEN:1;
	Uint16 Db5_2:1;					/*B5,b2 reserved*/
	Uint16 Db5_3:1;					/*B5,b3 reserved*/
	Uint16 VSI_GSELFWD:1;
	Uint16 VSI_GSELREV:1;
	Uint16 VSI_REGENEN:1;
	Uint16 VSI_BRAKESW:1;

	/* Byte 4 */
	Uint16 Db4_0:1;					/*B4,b0 reserved*/
	Uint16 VSI_PASINVERR:1;
	Uint16 VSI_COMPINVERR:1;
	Uint16 VSI_ABSACTIV:1;
	Uint16 Db4_4:1;					/*B4,b4 reserved*/
	Uint16 Db4_5:1;					/*B4,b5 reserved*/
	Uint16 VSI_BMSCORQ:1;
	Uint16 VSI_CPOW:1;
};

/* Join CAN message with raw data array (2 x 32bit array) */
union CAN_DATA_RX_651{
	Uint32 rawData[2];
	struct CAN_DATA_VARS_RX_651 dataVars;
};
extern union CAN_DATA_RX_651 canData_Rx_651;

/* Assign CAN ID to message */
extern canRxMessage_t canMessage_Rx_651;



/*******************************************************************************/


/*******************************************************************************
 * CAN Rx Message array
 * Assigns messages to mailbox position (array index + CAN_RX_MBOFFSET)
 * *****************************************************************************/

extern canRxMessage_t* CAN_RxMessages[];


extern const unsigned int numRxCANMsgs;

#endif /* CAN_RX_GLOBAL_H_ */
