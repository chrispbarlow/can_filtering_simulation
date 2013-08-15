/***********************************************************************************************************
 * CAN_Rx_global.c
 *
 * Global CAN message receive buffers and control variables
 *
 *  Created on: 7 Mar 2013
 *      Author: chris.barlow
 * *********************************************************************************************************/

#include "CAN_Rx_global.h"

/* Filter shadow is necessary due to being unable to read a mailbox's ID from registry */
filterShadow_t mailBoxFilterShadow_G[NUM_MESSAGES_MAX];

/* The main CAN Rx buffer - holds all received data in logging sequence order */
canRxMessage_t CAN_RxMessages_G[NUM_MESSAGES_MAX];

/* The logging list - list of CAN IDs transmitted to device in logging sequence order */
logging_list_t loggingList_G[NUM_MESSAGES_MAX];

/* The global sequence update state */
updateFlags_t updateSequenceRequired_G = INIT;

/* Global control variables */
Uint16 numRxCANMsgs_G = 0;
Uint16 filterSize_G = 0;

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
 * Copies sequence details from temporary buffers to global message sequence array.
 * Since we don't know where in the sequence we will start, the schedule timer for all messages is set to 1.
 * *********************************************************************************************************/
void buildSequence(Uint16 listSize){
	Uint16 i, cycleTime_min, newReload, remainder = 0;

	/* Finds the minimum cycle time in the logging list */
 	cycleTime_min = 0xFFFF;
 	for(i=0;i<listSize;i++){
 		if(loggingList_G[i].cycleTime_LLRx < cycleTime_min){
 			cycleTime_min = loggingList_G[i].cycleTime_LLRx;
 		}
 	}

 	for(i=0;i<listSize;i++){
		CAN_RxMessages_G[i].canID = loggingList_G[i].canID_LLRx;
		CAN_RxMessages_G[i].canData.rawData[0] = 0;
		CAN_RxMessages_G[i].canData.rawData[1] = 0;
		CAN_RxMessages_G[i].canDLC = loggingList_G[i].canDLC_LLRx;

		/* timer_reload set proportionally to weight the filter in favour of more frequent IDs */
		newReload = (loggingList_G[i].cycleTime_LLRx / cycleTime_min);
		/* Rounding logic */
		remainder = (loggingList_G[i].cycleTime_LLRx % cycleTime_min);
		if((remainder > 0)&&(remainder >= (cycleTime_min/2))){
			CAN_RxMessages_G[i].timer_reload = (newReload + 1);
		}
		else{
			CAN_RxMessages_G[i].timer_reload = newReload;
		}

		/* Force all timers to 1 for first iteration - level playing field */
		CAN_RxMessages_G[i].timer = 1;
		CAN_RxMessages_G[i].counter = 0;
 	}
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
