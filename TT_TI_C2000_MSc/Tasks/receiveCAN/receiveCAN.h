/*
 * newTask.h
 *
 *  Created on: 11 Feb 2013
 *      Author: chris.barlow
 */

#ifndef REVCEIVECAN_H_
#define REVCEIVECAN_H_

#include "../../Lib/CAN/CAN.h"


void receiveCAN_init(void);
void receiveCAN_update(void);
void updateFilter(unsigned int filterPointer);

#endif /* REVCEIVECAN_H_ */
