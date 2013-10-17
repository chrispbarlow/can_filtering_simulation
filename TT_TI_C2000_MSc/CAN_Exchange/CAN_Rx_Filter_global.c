/***********************************************************************************************************
 * CAN_Rx_global.c
 *
 * Global CAN message receive buffers and control variables
 *
 *  Created on: 7 Mar 2013
 *      Author: chris.barlow
 * *********************************************************************************************************/

#include "CAN_Rx_Filter_global.h"

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

typedef struct{
	Uint16 filterStart;
	Uint16 filterEnd;
	Uint16 sequenceStart;
	Uint16 sequenceEnd;
	Uint16 sequenceIndex;
} filterSegment_t;

filterSegment_t segments[2];


/***********************************************************************************************************
 * Copies sequence details from temporary buffers to global message sequence array.
 * Since we don't know where in the sequence we will start, the schedule timer for all messages is set to 1.
 * *********************************************************************************************************/
void buildSequence(Uint16 listSize){
	Uint16 i, cycleTime_min, newReload, remainder = 0;

	segments[0].filterStart = 0;
	segments[0].filterEnd = 10;
	segments[0].sequenceStart = 0;
	segments[0].sequenceEnd = 21;

	segments[1].filterStart = 11;
	segments[1].filterEnd = 31;
	segments[1].sequenceStart = 22;
	segments[1].sequenceEnd = 81;

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

 	numRxCANMsgs_G = listSize;
 }



/***********************************************************************************************************
 * Controls the scheduling of the IDs in the filter.
 * *********************************************************************************************************/
int16 getNextSequenceIndex(Uint16 segment){
	int16 sequenceIndex_next = -1;
	boolean_t searchResult = FALSE;


	if(updateSequenceRequired_G != RUN){
		/* Reset sequencePointer to continue sequence after loading mailbox */
		segments[segment].sequenceIndex = (segments[segment].sequenceEnd-1);
		sequenceIndex_next = segments[segment].sequenceIndex;
	}
	else{
		/* Find next required CAN ID in sequence */
		sequenceIndex_next = segments[segment].sequenceIndex;
		do{
			/* Wrap search */
			if(sequenceIndex_next < segments[segment].sequenceEnd){
				sequenceIndex_next++;
			}
			else{
				sequenceIndex_next = segments[segment].sequenceStart;
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
		while((searchResult == FALSE)&&(sequenceIndex_next != segments[segment].sequenceIndex));

		segments[segment].sequenceIndex = sequenceIndex_next;
	}

	return sequenceIndex_next;
}


Uint16 findSegment(Uint16 mailbox){
	Uint16 segmentNumber = 0, i;

	for(i = 0; i < sizeof(segments)/sizeof(segments[0]); i++){
		if((mailbox >= segments[i].filterStart) && (mailbox <= segments[i].filterEnd)){
			segmentNumber = i;
		}
	}

	return segmentNumber;
}

/***********************************************************************************************************
 * Replaces the ID in the filter at location filterPointer, with ID from sequence at location sequencePointer.
 * *********************************************************************************************************/
void updateFilter(Uint16 filterIndex, int16 sequenceIndex_replace){
	Uint16 sequenceIndex_old;

	if(updateSequenceRequired_G == RUN){
		/* Message scheduling */
		sequenceIndex_old = mailBoxFilterShadow_G[filterIndex].sequenceIndex_mapped;

//		if(CAN_RxMessages_G[sequenceIndex_old].timer >= 0){														/* Duplication restriction DOES NOT help when filter is segmented */
			CAN_RxMessages_G[sequenceIndex_old].timer = CAN_RxMessages_G[sequenceIndex_old].timer_reload;
//		}
//		else{
//			CAN_RxMessages_G[sequenceIndex_old].timer++;
//		}
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
