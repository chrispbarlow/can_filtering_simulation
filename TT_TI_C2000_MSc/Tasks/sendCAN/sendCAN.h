/*
 * newTask.h
 *
 *  Created on: 11 Feb 2013
 *      Author: chris.barlow
 */

#ifndef SENDCAN_H_
#define SENDCAN_H_

#include "../../Lib/CAN/CAN.h"

void sendCAN_init(void);
void sendCAN_update(void);
void CANTx_clearAll(void);

#endif /* SENDCAN_H_ */
