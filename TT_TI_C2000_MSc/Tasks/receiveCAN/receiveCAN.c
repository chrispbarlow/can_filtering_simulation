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

typedef enum{FALSE, TRUE}boolean_t;


filterShadow_t mailBoxFilters[32];

void receiveCAN_init(void){
	Uint16 mailBox;
	printf("CAN Rx Config:\n");

	for(mailBox=0; mailBox<31; mailBox++){
		updateFilter(mailBox);
	}
}

void receiveCAN_update(void){
	Uint16 mailBox, messagePointer;
	static Uint32 heartbeat = 0;

	for(mailBox=0; mailBox<31; mailBox++){
		if(checkMailboxState(CANPORT_A, mailBox) == RX_PENDING){

			messagePointer = mailBoxFilters[mailBox].messagePointer;
			CAN_RxMessages[messagePointer].counter++;
			readRxMailbox(CANPORT_A, mailBox, CAN_RxMessages[messagePointer].canData.rawData); /*TODO: read use sequence pointer for array index */
	//		printf("0x%03X\n", (Uint16)CAN_RxMessages[messagePointer].canID);

			/*TODO: update mailBox pointer and ID */
			updateFilter(mailBox);
		}
	}
	heartbeat++;
	CAN_TxMessages[0]->canData[0] = CAN_RxMessages[0].counter;
	CAN_TxMessages[0]->canData[1] = heartbeat;
}

void updateFilter(unsigned int filterPointer){
	static int16 last_i = -1;
	int16 i;
	char j;
	boolean_t IDfound = FALSE, result = FALSE;

	i = last_i;
	do{
		if(i<(numRxCANMsgs-1)){
			i++;
		}
		else{
			i=0;
		}

		for(j = 0; j < 31; j++){
			if(mailBoxFilters[j].canID == CAN_RxMessages[i].canID){
				IDfound = TRUE;
			}
			else{
				IDfound = FALSE;
			}
		}

		if(IDfound == FALSE){
			CAN_RxMessages[i].timer--;
		}

		if(CAN_RxMessages[i].timer<=0){
			result = TRUE;
		}

	}while((result == FALSE)&&(i != last_i));


	if(result == TRUE){
		last_i = i;

		CAN_RxMessages[i].timer = CAN_RxMessages[i].timer_reload;

		mailBoxFilters[filterPointer].canID = CAN_RxMessages[i].canID;
		mailBoxFilters[filterPointer].messagePointer = i;

		configureMailbox(CANPORT_A, filterPointer, CAN_RX, ID_STD, CAN_RxMessages[i].canID, CAN_RxMessages[i].canDLC);
		//printf("%02u: 0x%03X\n", filterPointer, (Uint16)mailBoxFilters[filterPointer].canID);

	}
}
