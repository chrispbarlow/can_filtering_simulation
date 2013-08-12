/*
 * CAN_Rx_global.c
 *
 *  Created on: 7 Mar 2013
 *      Author: chris.barlow
 */
#include "CAN_Rx_global.h"

/* Filter shadow is necessary due to being unable to read a mailbox's ID from registry */
filterShadow_t mailBoxFilterShadow_G[NUM_MESSAGES_MAX];

/* The main CAN Rx buffer - holds all received data in logging order */
canRxMessage_t CAN_RxMessages_G[NUM_MESSAGES_MAX];

updateFlags_t updateSequenceRequired_G = INIT;

Uint16 numRxCANMsgs_G = 0;
Uint16 filterSize_G = 0;


