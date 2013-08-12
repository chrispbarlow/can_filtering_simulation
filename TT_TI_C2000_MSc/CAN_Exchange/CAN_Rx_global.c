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


/* The global sequence update state */
updateFlags_t updateSequenceRequired_G = INIT;

/* Global control variables */
Uint16 numRxCANMsgs_G = 0;
Uint16 filterSize_G = 0;


