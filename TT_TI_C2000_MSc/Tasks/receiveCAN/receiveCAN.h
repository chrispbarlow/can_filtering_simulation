/*
*   receiveCAN checks the status of mailboxes. When a message is pending, the data is read
 *  and the dynamic filter mechanism updates the mailbox to the next valid CAN ID
 *  Created on: 11 Feb 2013
 *      Author: chris.barlow
 */

#ifndef REVCEIVECAN_H_
#define REVCEIVECAN_H_

#include "../../Lib/CAN/CAN.h"


void receiveCAN_init(void);

/* receiveCAN checks the status of mailboxes. When a message is pending, the data is read
 * and the dynamic filter mechanism updates the mailbox to the next valid CAN ID 			*/
void receiveCAN_update(void);

/* getNextSequencePointer controls the scheduling of the IDs in the filter and returns the next valid ID */
int16 getNextSequencePointer(void);

/* Replaces the ID in the filter at location filterPointer, with ID from sequence at location sequencePointer */
void updateFilter(Uint16 filterPointer, int16 sequencePointer);

#endif /* REVCEIVECAN_H_ */
