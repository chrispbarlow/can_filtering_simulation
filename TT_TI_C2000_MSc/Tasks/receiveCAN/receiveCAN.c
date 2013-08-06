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
	static Uint16 mailBox;
	Uint16 messagePointer;
	int16 newSequencePointer;
	static Uint32 totalcounter = 0;

	if(updateSequenceRequired_G == 1){
		mailBox = 0;
		updateSequenceRequired_G = 2;
	}
	else if(updateSequenceRequired_G == 2){
		filterSize_G = numRxCANMsgs_G/FILTERSIZE_RATIO;
		if((numRxCANMsgs_G%2)!=0){
			filterSize_G += 1;
		}

		updateFilter(mailBox,mailBox);

		printf("%d: %d %d\n",mailBox, CAN_RxMessages_G[mailBox].timer, CAN_RxMessages_G[mailBox].timer_reload);

		mailBox++;
		if(mailBox == filterSize_G){
			getNextSequencePointer(); /* Calling here re-initialises the sequencePointer */
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
				CAN_RxMessages_G[messagePointer].counter++;
				totalcounter++;

				newSequencePointer = getNextSequencePointer();

				/* read the CAN data into buffer (nothing done with the data, but nice to do this for realistic timing */
				readRxMailbox(CANPORT_A, mailBox, CAN_RxMessages_G[messagePointer].canData.rawData);

				/* update the filter for next required ID */
				updateFilter(mailBox, newSequencePointer);
			}
		}
	}
}

int16 getNextSequencePointer(void){
	static int16 sequencePointer = -1;
	int16 last_sequencePointer;
	boolean_t result = FALSE;

	if(updateSequenceRequired_G > 0){
		sequencePointer = (filterSize_G - 1);
	}
	else{
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
			if(CAN_RxMessages_G[sequencePointer].timer > (0-DUPLICATES_ALLOWED)){

				CAN_RxMessages_G[sequencePointer].timer--;

				/* ID ready to be inserted */
				if(CAN_RxMessages_G[sequencePointer].timer <= 0){
					result = TRUE;
				}
				else{
					result = FALSE;
				}

			}
			else{
				result = FALSE;
			}
		}
		while((result == FALSE)&&(sequencePointer != last_sequencePointer));
	}

	return sequencePointer;
}

void updateFilter(Uint16 filterPointer, int16 sequencePointer){

	Uint16 last_messagePointer;

	if(updateSequenceRequired_G == 0){
		/* Message scheduling */
		last_messagePointer = mailBoxFilters[filterPointer].messagePointer;
		CAN_RxMessages_G[last_messagePointer].timer = CAN_RxMessages_G[last_messagePointer].timer_reload;
	}
	else{
		CAN_RxMessages_G[filterPointer].timer = 0;
	}

	/* Real ID replacement */
	configureRxMailbox(CANPORT_A, filterPointer, ID_STD, CAN_RxMessages_G[sequencePointer].canID, CAN_RxMessages_G[sequencePointer].canDLC);

	/* ID replacement in shadow */
	mailBoxFilters[filterPointer].canID = CAN_RxMessages_G[sequencePointer].canID;
	mailBoxFilters[filterPointer].messagePointer = sequencePointer;
}
