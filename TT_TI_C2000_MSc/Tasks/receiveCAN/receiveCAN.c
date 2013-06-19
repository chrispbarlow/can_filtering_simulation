/*
 * sendCAN - controls timing and transmission of CAN messages from the CAN_Messages array.
 *
 *  Created on: 11 Feb 2013
 *      Author: chris.barlow
 */
#include "receiveCAN.h"
#include <stdio.h>
#include "../../CAN_Exchange/CAN_Rx_global.h"
#include "../../CAN_Exchange/CAN_Tx_global.h"

void receiveCAN_init(void){
	Uint16 i, mailBox;
	printf("CAN Rx Config:\n");

	for(i=0; i<numRxCANMsgs; i++){
		mailBox = i + numTxCANMsgs;

		printf("%02u: %X%X\n", mailBox, CAN_RxMessages[i]->canID);
		if(mailBox < 32){
			configureMailbox(CANPORT_A, mailBox, CAN_RX, ID_STD, CAN_RxMessages[i]->canID, CAN_RxMessages[i]->canDLC, i);
		}
	}
}

void receiveCAN_update(void){
	Uint16 i, mailBox;

	for(i=0; i<numRxCANMsgs; i++){
		mailBox = i + numTxCANMsgs;

		if(mailBox < 32){
			if(checkMailboxState(CANPORT_A, mailBox) == RX_PENDING){
				readRxMailbox(CANPORT_A, mailBox, CAN_RxMessages[i]->canData); /*TODO: read use sequence pointer for array index */
				/*TODO: update mailbox pointer and ID */
			}
		}
		else{
			printf("Mailbox overflow\n");
		}
	}
}
