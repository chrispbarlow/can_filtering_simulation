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

#define DUPLICATES_ALLOWED 	(1)
#define FILTERSIZE_RATIO	(2)

typedef enum{FALSE, TRUE}boolean_t;

void receiveCAN_init(void){

	/* mailboxes are configured when first logging list is received from desktop app */
}

void receiveCAN_update(void){
	Uint16 mailBox, messagePointer;
	static Uint32 totalcounter = 0;

	if(updateSequenceRequired_G == 1){

		filterSize_G = numRxCANMsgs_G/FILTERSIZE_RATIO;

		if((numRxCANMsgs_G%2)!=0){
			filterSize_G += 1;
		}

		for(mailBox=0; mailBox < filterSize_G; mailBox++){
			updateFilter(mailBox);
			updateSequenceRequired_G = 0;
		}
	}
	else{
		/* look through mailboxes for pending messages */
		for(mailBox=0; mailBox<filterSize_G; mailBox++){
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
}

void updateFilter(unsigned int filterPointer){
	static int16 sequencePointer = -1;
	int16 last_sequencePointer, last_messagePointer;
	boolean_t result = FALSE;

	if(updateSequenceRequired_G == 1){
		sequencePointer = -1;
	}
	/* Find next required CAN ID in sequence */
	last_sequencePointer = sequencePointer;
	do{
		/* Wrap search */
		if(sequencePointer<(numRxCANMsgs_G-1)){
			sequencePointer++;
		}
		else{
			sequencePointer=0;
		}

		/* ID not already in mailbox, decrement 'schedule' timer (timer set to -1 whilst ID is in mailbox) */
		if(CAN_RxMessages[sequencePointer].timer >= (0-DUPLICATES_ALLOWED)){
			CAN_RxMessages[sequencePointer].timer--;
		}

		/* ID ready to be inserted */
		if((CAN_RxMessages[sequencePointer].timer >= (0-DUPLICATES_ALLOWED))&&(CAN_RxMessages[sequencePointer].timer <= 0)){
			result = TRUE;
		}
	}
	while((result == FALSE)&&(sequencePointer != last_sequencePointer));


	/* New ID found for mailbox */
	if(result == TRUE){

		/* Message scheduling */
		last_messagePointer = mailBoxFilters[filterPointer].messagePointer;
		CAN_RxMessages[last_messagePointer].timer = CAN_RxMessages[last_messagePointer].timer_reload;

		/* Real ID replacement */
		configureRxMailbox(CANPORT_A, filterPointer, ID_STD, CAN_RxMessages[sequencePointer].canID, CAN_RxMessages[sequencePointer].canDLC);

		/* ID replacement in shadow */
		mailBoxFilters[filterPointer].canID = CAN_RxMessages[sequencePointer].canID;
		mailBoxFilters[filterPointer].messagePointer = sequencePointer;

	}

}
