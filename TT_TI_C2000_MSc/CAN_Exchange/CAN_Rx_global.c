/*
 * CAN_Rx_global.c
 *
 *  Created on: 7 Mar 2013
 *      Author: chris.barlow
 */
#include "CAN_Rx_global.h"

filterShadow_t mailBoxFilters[64];

updateFlags_t updateSequenceRequired_G = INIT;

/*******************************************************************************
 * CAN Rx Message array
 * Assigns messages to mailbox position (array index + CAN_RX_MBOFFSET)
 * *****************************************************************************/

canRxMessage_t CAN_RxMessages_G[64];

Uint16 numRxCANMsgs_G = 0;
Uint16 filterSize_G = 0;


