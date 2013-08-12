/***********************************************************************************************************
 *  receiveCAN.c
 *  	checks the status of mailboxes. When a message is pending, the data is read
 *  	and the dynamic filter mechanism updates the mailbox to the next valid CAN ID
 *
 *  Created on: 19 Jun 2013
 *      Author: chris.barlow
 * *********************************************************************************************************/

#include "../../global.h"
#include "receiveCAN.h"
#include <stdio.h>
#include "../../CAN_Exchange/CAN_Rx_global.h"
#include "../../CAN_Exchange/CAN_Tx_global.h"

#define DUPLICATES_LIMIT 	(1)		/* Controls the number of duplicates for each ID allowed to be added to filter between arrivals of that ID */
#define FILTERSIZE_RATIO	(2)

/***********************************************************************************************************
 * Initialisation - called once when the device boots, before the scheduler starts.
 * *********************************************************************************************************/
void receiveCAN_init(void){
	/* mailboxes are configured in _update when first logging list is received from desktop app */
	updateSequenceRequired_G = INIT;
}

/***********************************************************************************************************
 * Update function - called periodically from scheduler
 * *********************************************************************************************************/
void receiveCAN_update(void){
	static Uint16 mailBox = 0;
	Uint16 sequenceIndex_received;
	int16 sequenceIndex_new;

	/* updateSequenceRequired_G controls the sequence update mechanism when a new logging list is transmitted to the device */
	switch(updateSequenceRequired_G){
	/* Do nothing until first logging list arrival (RESET)*/
	default:
	case INIT:
		break;

	/* controlSCI will initiate RESET when new logging list is received */
	case RESET:

		/* Ensure all mailboxes are disabled */
		for(mailBox = 0; mailBox < NUM_MAILBOXES_MAX; mailBox++){
			disableMailbox(CANPORT_A, mailBox);
		}

		mailBox = 0;
		if(filterSize_G == 0){	/* Dynamic filterSize calculation if 0 is received from configuration app */
			filterSize_G = numRxCANMsgs_G/FILTERSIZE_RATIO;
			if((numRxCANMsgs_G%2)!=0){
				filterSize_G += 1;
			}
		}
		updateSequenceRequired_G = UPDATE;
		break;

	/* Set up mailboxes for initial filter conditions */
	case UPDATE:

		/* Direct copy of first filterSize_G IDs in the sequence */
		updateFilter(mailBox,mailBox);
		printf("%d: %d %d\n",mailBox, CAN_RxMessages_G[mailBox].timer, CAN_RxMessages_G[mailBox].timer_reload);

		/* Initialising one mailBox per tick ensures all mailboxes are initialised before moving to RUN (mainly so that we can printf some debug info) */
		mailBox++;
		if(mailBox == filterSize_G){
			getNextSequenceIndex(); /* Calling here re-initialises the sequencePointer */
			updateSequenceRequired_G = RUN;
		}
		break;

	/* Checking for CAN messages and updating filters - normal running conditions */
	case RUN:
		/* look through mailboxes for pending messages */
		for(mailBox=0; mailBox<filterSize_G; mailBox++){
			if(checkMailboxState(CANPORT_A, mailBox) == RX_PENDING){

				disableMailbox(CANPORT_A, mailBox);

				/* Find message pointer from mailbox shadow */
				sequenceIndex_received = mailBoxFilterShadow_G[mailBox].sequenceIndex_mapped;

				/* read the CAN data into buffer (Nothing is done with the data, but nice to do this for realistic timing) */
				readRxMailbox(CANPORT_A, mailBox, CAN_RxMessages_G[sequenceIndex_received].canData.rawData);

				/* ID scheduling and duplication control */
				sequenceIndex_new = getNextSequenceIndex();

				/* update the filter for next required ID  */
				updateFilter(mailBox, sequenceIndex_new);	/* Mailbox is re-enabled in configureRxMailbox() - this is done last to help prevent new message arrivals causing erroneous hits mid-way through process*/

				/* Count message hits */
				CAN_RxMessages_G[sequenceIndex_received].counter++;
			}
		}
		break;
	}
}

/***********************************************************************************************************
 * Controls the scheduling of the IDs in the filter.
 * *********************************************************************************************************/
int16 getNextSequenceIndex(void){
	static int16 sequenceIndex_next = -1;
	int16 sequenceIndex_last;
	boolean_t searchResult = FALSE;


	if(updateSequenceRequired_G != RUN){
		/* Reset sequencePointer to continue sequence after loading mailbox */
		sequenceIndex_next = (filterSize_G - 1);
	}
	else{
		/* Find next required CAN ID in sequence */
		sequenceIndex_last = sequenceIndex_next;
		do{
			/* Wrap search */
			if(sequenceIndex_next<(numRxCANMsgs_G-1)){
				sequenceIndex_next++;
			}
			else{
				sequenceIndex_next = 0;
			}

			/* ID not already in mailbox, decrement 'schedule' timer (timer sits between -DUPLICATES ALLOWED and 0 while ID is in one or more mailboxes) */
			if(CAN_RxMessages_G[sequenceIndex_next].timer > (0-DUPLICATES_LIMIT)){
				CAN_RxMessages_G[sequenceIndex_next].timer--;

				/* ID ready to be inserted */
				if(CAN_RxMessages_G[sequenceIndex_next].timer <= 0){
					searchResult = TRUE;
				}
				else{
					searchResult = FALSE;	/* ET balancing */
				}
			}
			else{
				searchResult = FALSE;		/* ET balancing */
			}
		}	/* Search will abort if all messages have been checked */
		while((searchResult == FALSE)&&(sequenceIndex_next != sequenceIndex_last));
	}

	return sequenceIndex_next;
}

/***********************************************************************************************************
 * Replaces the ID in the filter at location filterPointer, with ID from sequence at location sequencePointer.
 * *********************************************************************************************************/
void updateFilter(Uint16 filterIndex, int16 sequenceIndex_replace){
	Uint16 sequenceIndex_old;

	if(updateSequenceRequired_G == RUN){
		/* Message scheduling */
		sequenceIndex_old = mailBoxFilterShadow_G[filterIndex].sequenceIndex_mapped;
		CAN_RxMessages_G[sequenceIndex_old].timer = CAN_RxMessages_G[sequenceIndex_old].timer_reload;
	}
	else{
		/* Used during mailbox reload - replicates the timer mechanism for first use of ID */
		CAN_RxMessages_G[filterIndex].timer = 0;
	}

	/* ID replacement in shadow */
	mailBoxFilterShadow_G[filterIndex].canID_mapped = CAN_RxMessages_G[sequenceIndex_replace].canID;
	mailBoxFilterShadow_G[filterIndex].sequenceIndex_mapped = sequenceIndex_replace;

	/* Real ID replacement - also re-enables mailbox*/
	configureRxMailbox(CANPORT_A, filterIndex, ID_STD, CAN_RxMessages_G[sequenceIndex_replace].canID, CAN_RxMessages_G[sequenceIndex_replace].canDLC);
}
