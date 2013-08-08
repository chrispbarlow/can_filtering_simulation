/*
*   receiveCAN checks the status of mailboxes. When a message is pending, the data is read
 *  and the dynamic filter mechanism updates the mailbox to the next valid CAN ID
 *  Created on: 11 Feb 2013
 *      Author: chris.barlow
 */

#include "receiveCAN.h"
#include <stdio.h>
#include "../../CAN_Exchange/CAN_Rx_global.h"
#include "../../CAN_Exchange/CAN_Tx_global.h"

#define DUPLICATES_ALLOWED 	(1)

typedef enum{FALSE, TRUE}boolean_t;

/* Init function called once when device boots */
void receiveCAN_init(void){
	/* mailboxes are configured in _update when first logging list is received from desktop app */
	updateSequenceRequired_G = INIT;
}

/* update function called periodically from TT scheduler */
void receiveCAN_update(void){
	static Uint16 mailBox;
	Uint16 messagePointer;
	int16 newSequencePointer;

	/* updateSequenceRequired_G controls the sequence update mechanism when a new logging list is transmitted to the device */
	switch(updateSequenceRequired_G){
	/* controlSCI will initiate RESET when new logging list is received */
	case RESET:
		mailBox = 0;
		updateSequenceRequired_G = UPDATE;
		break;

	/* Set up mailboxes for initial filter conditions */
	case UPDATE:

		/* Direct copy of first filterSize_G IDs in the sequence */
		updateFilter(mailBox,mailBox);
		printf("%d: %d %d\n",mailBox, CAN_RxMessages_G[mailBox].timer, CAN_RxMessages_G[mailBox].timer_reload);

		mailBox++;
		if(mailBox == filterSize_G){
			getNextSequencePointer(); /* Calling here re-initialises the sequencePointer */
			updateSequenceRequired_G = RUN;
		}
		break;

	/* Checking for CAN messages and updating filters - normal running conditions */
	case RUN:
		/* look through mailboxes for pending messages */
		for(mailBox=0; mailBox<filterSize_G; mailBox++){
			if(checkMailboxState(CANPORT_A, mailBox) == RX_PENDING){

				/* Find message pointer from mailbox shadow */
				messagePointer = mailBoxFilters_G[mailBox].messagePointer;

				/* Count message hits */
				CAN_RxMessages_G[messagePointer].counter++;

				newSequencePointer = getNextSequencePointer();

				/* read the CAN data into buffer (nothing done with the data, but nice to do this for realistic timing */
				readRxMailbox(CANPORT_A, mailBox, CAN_RxMessages_G[messagePointer].canData.rawData);

				/* update the filter for next required ID */
				updateFilter(mailBox, newSequencePointer);
			}
		}
		break;

	/* Do nothing until first RESET */
	default:
	case INIT:
		break;
	}
}

/* getNextSequencePointer controls the scheduling of the IDs in the filter and returns the next valid ID */
int16 getNextSequencePointer(void){
	static int16 sequencePointer = -1;
	int16 last_sequencePointer;
	boolean_t result = FALSE;


	if(updateSequenceRequired_G != RUN){
		/* Reset sequencePointer to continue sequence after loading mailbox */
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

/* Replaces the ID in the filter at location filterPointer, with ID from sequence at location sequencePointer */
void updateFilter(Uint16 filterPointer, int16 sequencePointer){
	Uint16 last_messagePointer;

	if(updateSequenceRequired_G == RUN){
		/* Message scheduling */
		last_messagePointer = mailBoxFilters_G[filterPointer].messagePointer;
		CAN_RxMessages_G[last_messagePointer].timer = CAN_RxMessages_G[last_messagePointer].timer_reload;
	}
	else{
		/* Used during mailbox reload - replicates the timer mechanism for first use of ID */
		CAN_RxMessages_G[filterPointer].timer = 0;
	}

	/* Real ID replacement */
	configureRxMailbox(CANPORT_A, filterPointer, ID_STD, CAN_RxMessages_G[sequencePointer].canID, CAN_RxMessages_G[sequencePointer].canDLC);

	/* ID replacement in shadow */
	mailBoxFilters_G[filterPointer].canID = CAN_RxMessages_G[sequencePointer].canID;
	mailBoxFilters_G[filterPointer].messagePointer = sequencePointer;
}
