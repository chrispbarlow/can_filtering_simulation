/*
 * CAN_Rx_global.c
 *
 *  Created on: 7 Mar 2013
 *      Author: chris.barlow
 */
#include "CAN_Rx_global.h"

/* Filter shadow is necessary due to being unable to read a mailbox's ID from registry */
filterShadow_t mailBoxFilters_G[64];

updateFlags_t updateSequenceRequired_G = INIT;

canRxMessage_t CAN_RxMessages_G[64];

Uint16 numRxCANMsgs_G = 0;
Uint16 filterSize_G = 0;


