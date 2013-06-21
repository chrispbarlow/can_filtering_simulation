/*
 * CAN_Tx_global.c
 *
 *  Created on: 5 Mar 2013
 *      Author: chris.barlow
 */
#include "CAN_Tx_global.h"

union CAN_DATA_TX_7AB canData_Tx_7AB;
canTxMessage_t canMessage_Tx_7AB = {
		0x7AB,
		canData_Tx_7AB.rawData,
		8,
		10,
		0
};


union CAN_DATA_TX_651 canData_Tx_651;
canTxMessage_t canMessage_Tx_651 = {
		0x651,
		canData_Tx_651.rawData,
		8,
		25,
		2
};

/*******************************************************************************
 * CAN Tx Message array
 * Assigns messages to mailbox position (array index + CAN_TX_MBOFFSET)
 * *****************************************************************************/

canTxMessage_t* CAN_TxMessages[] = {
		&canMessage_Tx_7AB,
//		&canMessage_Tx_651
};

const unsigned int numTxCANMsgs = (sizeof(CAN_TxMessages) / sizeof(CAN_TxMessages[0]));
