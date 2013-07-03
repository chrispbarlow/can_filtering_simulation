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

#define FILTERSIZE (16)

typedef enum{FALSE, TRUE}boolean_t;



void receiveCAN_init(void){
	Uint16 mailBox;
	printf("CAN Rx Config:\n");

	for(mailBox=0; mailBox<FILTERSIZE; mailBox++){
		updateFilter(mailBox);
		printf("0x%03X\n", (Uint16)CAN_RxMessages[mailBox].canID);
	}
}

void receiveCAN_update(void){
	Uint16 mailBox, messagePointer;
	static Uint32 totalcounter = 0;

	/* look through mailboxes for pending messages */
	for(mailBox=0; mailBox<FILTERSIZE; mailBox++){
		if(checkMailboxState(CANPORT_A, mailBox) == RX_PENDING){

			/* Find message pointer from mailbox shadow */
			messagePointer = mailBoxFilters[mailBox].messagePointer;

			/* Count message hits */
			CAN_RxMessages[messagePointer].counter++;
			totalcounter++;

			/* read the CAN data into buffer (nothing done with the data, but nice to do this for realistic timing */
			readRxMailbox(CANPORT_A, mailBox, CAN_RxMessages[messagePointer].canData.rawData);

			/* update the filter for next required ID */
			updateFilter(mailBox);
		}
	}
}

void updateFilter(unsigned int filterPointer){
	static int16 last_sequencePointer = -1;
	int16 sequencePointer;
	char j;
	boolean_t IDfound = FALSE, result = FALSE;

	/* Find next required CAN ID in sequence */
	sequencePointer = last_sequencePointer;
	do{
		/* Wrap search */
		if(sequencePointer<(numRxCANMsgs-1)){
			sequencePointer++;
		}
		else{
			sequencePointer=0;
		}

		/* Check there is no mailbox already configured for this ID */
		for(j = 0; j < FILTERSIZE; j++){
			if(mailBoxFilters[j].canID == CAN_RxMessages[sequencePointer].canID){
				IDfound = TRUE;
			}
			else{
				IDfound = FALSE;
			}
		}

		/* ID not already in mailbox, decrement 'schedule' timer */
		if(IDfound == FALSE){
			CAN_RxMessages[sequencePointer].timer--;
		}

		/* ID ready to be inserted */
		if(CAN_RxMessages[sequencePointer].timer<=0){
			result = TRUE;
		}

	}while((result == FALSE)&&(sequencePointer != last_sequencePointer));


	/* New ID found for mailbox */
	if(result == TRUE){
		last_sequencePointer = sequencePointer;

		/* Message scheduling */
		CAN_RxMessages[sequencePointer].timer = CAN_RxMessages[sequencePointer].timer_reload;

		/* ID replacement in shadow */
		mailBoxFilters[filterPointer].canID = CAN_RxMessages[sequencePointer].canID;
		mailBoxFilters[filterPointer].messagePointer = sequencePointer;

		/* Real ID replacement */
		configureRxMailbox(CANPORT_A, filterPointer, ID_STD, CAN_RxMessages[sequencePointer].canID, CAN_RxMessages[sequencePointer].canDLC);
	}
}
