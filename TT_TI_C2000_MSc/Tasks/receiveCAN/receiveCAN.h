/***********************************************************************************************************
 *  receiveCAN.h
 *  	checks the status of mailboxes. When a message is pending, the data is read
 *  	and the dynamic filter mechanism updates the mailbox to the next valid CAN ID
 *
 *  Created on: 11 Feb 2013
 *      Author: chris.barlow
 * *********************************************************************************************************/

#ifndef REVCEIVECAN_H_
#define REVCEIVECAN_H_

#include "../../Lib/CAN/CAN.h"


/***********************************************************************************************************
 * Initialisation - called once when the device boots, before the scheduler starts.
 * *********************************************************************************************************/
void receiveCAN_init(void);


/***********************************************************************************************************
 * Update function - called periodically from scheduler
 * *********************************************************************************************************/
void receiveCAN_update(void);

/***********************************************************************************************************
 * Controls the scheduling of the IDs in the filter.
 * Returns the next valid sequence index to use in the filter.
 * *********************************************************************************************************/
int16 getNextSequenceIndex(void);

/***********************************************************************************************************
 * Replaces the ID in the filter at location filterPointer, with ID from sequence at location sequencePointer.
 * Arguments:
 * 		filterIndex -- the Index of the filter mailbox to modify
 * 		sequenceIndex_replace -- The new sequence index to use in the filter
 * *********************************************************************************************************/
void updateFilter(Uint16 filterIndex, int16 sequenceIndex_replace);

#endif /* REVCEIVECAN_H_ */
