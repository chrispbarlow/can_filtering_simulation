/*
 * CAN_Rx_global.c
 *
 *  Created on: 7 Mar 2013
 *      Author: chris.barlow
 */
#include "CAN_Rx_global.h"


union CAN_DATA_RX_7AB canData_Rx_7AB;
canRxMessage_t canMessage_Rx_7AB = {
		0x7AB,
		canData_Rx_7AB.rawData,
		8
};


union CAN_DATA_RX_651 canData_Rx_651;
canRxMessage_t canMessage_Rx_651 = {
		0x651,
		canData_Rx_651.rawData,
		6
};

/*******************************************************************************
 * CAN Rx Message array
 * Assigns messages to mailbox position (array index + CAN_RX_MBOFFSET)
 * *****************************************************************************/

canRxMessage_t* CAN_RxMessages[] = {
		&canMessage_Rx_7AB,
		&canMessage_Rx_651
};

const unsigned int numRxCANMsgs = (sizeof(CAN_RxMessages) / sizeof(CAN_RxMessages[0]));
