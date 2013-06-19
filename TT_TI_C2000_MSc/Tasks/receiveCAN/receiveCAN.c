/*
 * sendCAN - controls timing and transmission of CAN messages from the CAN_Messages array.
 *
 *  Created on: 11 Feb 2013
 *      Author: chris.barlow
 */
#include "receiveCAN.h"
#include <stdio.h>
#include "../../CAN_Exchange/CAN_Rx_global.h"
#include "../../CAN_Exchange/CAN_Tx_global.h"

void receiveCAN_init(void)
{
	Uint16 i, mailBox;
	printf("CAN Rx Config:\n");

	for(i=0; i<numRxCANMsgs; i++)
	{
		mailBox = i + numTxCANMsgs;

		printf("%02u: %X%X\n", mailBox, CAN_RxMessages[i]->canID);
		if(mailBox < 32)
		{
			configureMailbox(CANPORT_A, mailBox, CAN_RX, ID_STD, CAN_RxMessages[i]->canID, CAN_RxMessages[i]->canDLC);
		}
	}
}

void receiveCAN_update(void)
{
	Uint16 i, mailBox;

	for(i=0; i<numRxCANMsgs; i++)
	{
		mailBox = i + numTxCANMsgs;

		if(mailBox < 32)
		{
			if(checkMailboxState(CANPORT_A, mailBox) == RX_PENDING)
			{
				if(readRxMailbox(CANPORT_A, mailBox, CAN_RxMessages[i]->canData) == CAN_RxMessages[i]->canDLC)
				{
					if(i == 0)
					{
					printf("%X%X: %01X%01X%01X%01X%01X%01X%01X%01X %02X %02X %02X %04X %04X\n",
							canMessage_Rx_7AB.canID,
							canData_Rx_7AB.dataVars.Db0_7,
							canData_Rx_7AB.dataVars.Db0_6,
							canData_Rx_7AB.dataVars.Db0_5,
							canData_Rx_7AB.dataVars.Db0_4,
							canData_Rx_7AB.dataVars.Db0_3,
							canData_Rx_7AB.dataVars.Db0_2,
							canData_Rx_7AB.dataVars.Db0_1,
							canData_Rx_7AB.dataVars.Db0_0,

							canData_Rx_7AB.dataVars.DB1,
							canData_Rx_7AB.dataVars.DB2,
							canData_Rx_7AB.dataVars.DB3,

							canData_Rx_7AB.dataVars.DB5_4,
							canData_Rx_7AB.dataVars.DB7_6);
					}

					if(i == 1)
					{
						/* Byte 3 */
						canData_Tx_651.dataVars.vs_state 		= canData_Rx_651.dataVars.vs_state;

						/* Byte 2 */
						/* Byte 1 */
						/* Byte 0 */
						canData_Tx_651.dataVars.vs_xcount 		= canData_Rx_651.dataVars.vs_xcount;

						/* Byte 7 */
						canData_Tx_651.dataVars.VSO_TEN 		= canData_Rx_651.dataVars.VSO_TEN;
						canData_Tx_651.dataVars.VSO_WPMP 		= canData_Rx_651.dataVars.VSO_WPMP;
						canData_Tx_651.dataVars.VSO_WFAN 		= canData_Rx_651.dataVars.VSO_WFAN;
						canData_Tx_651.dataVars.VSO_PASINVSS 	= canData_Rx_651.dataVars.VSO_PASINVSS;

						/* Byte 6 */
						canData_Tx_651.dataVars.VSO_COMPINVSS 	= canData_Rx_651.dataVars.VSO_COMPINVSS;
						canData_Tx_651.dataVars.VSO_PASINVRST 	= canData_Rx_651.dataVars.VSO_PASINVRST;
						canData_Tx_651.dataVars.VSO_COMPINRST 	= canData_Rx_651.dataVars.VSO_COMPINRST;
						canData_Tx_651.dataVars.VSO_DCDC 		= canData_Rx_651.dataVars.VSO_DCDC;
						canData_Tx_651.dataVars.VSO_BRKLAMP 	= canData_Rx_651.dataVars.VSO_BRKLAMP;
						canData_Tx_651.dataVars.VSO_HVOK1 		= canData_Rx_651.dataVars.VSO_HVOK1;
						canData_Tx_651.dataVars.VSO_HVOK2 		= canData_Rx_651.dataVars.VSO_HVOK2;

						/* Byte 5 */
						canData_Tx_651.dataVars.VSI_DRVEN 		= canData_Rx_651.dataVars.VSI_DRVEN;
						canData_Tx_651.dataVars.VSI_BCEN 		= canData_Rx_651.dataVars.VSI_BCEN;
						canData_Tx_651.dataVars.VSI_GSELFWD 	= canData_Rx_651.dataVars.VSI_GSELFWD;
						canData_Tx_651.dataVars.VSI_GSELREV 	= canData_Rx_651.dataVars.VSI_GSELREV;
						canData_Tx_651.dataVars.VSI_REGENEN 	= canData_Rx_651.dataVars.VSI_REGENEN;
						canData_Tx_651.dataVars.VSI_BRAKESW 	= canData_Rx_651.dataVars.VSI_BRAKESW;

						/* Byte 4 */
						canData_Tx_651.dataVars.VSI_PASINVERR 	= canData_Rx_651.dataVars.VSI_PASINVERR;
						canData_Tx_651.dataVars.VSI_COMPINVERR 	= canData_Rx_651.dataVars.VSI_COMPINVERR;
						canData_Tx_651.dataVars.VSI_ABSACTIV 	= canData_Rx_651.dataVars.VSI_ABSACTIV;
						canData_Tx_651.dataVars.VSI_BMSCORQ 	= canData_Rx_651.dataVars.VSI_BMSCORQ;
						canData_Tx_651.dataVars.VSI_CPOW 		= canData_Rx_651.dataVars.VSI_CPOW;
					}
				}
				else
				{
					printf("RxErr\n");
				}
			}
		}
		else
		{
			printf("Mailbox overflow\n");
		}
	}
}
