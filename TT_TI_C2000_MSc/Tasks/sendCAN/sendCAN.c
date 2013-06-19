/*
 * sendCAN - controls timing and transmission of CAN messages from the CAN_Messages array.
 *
 *  Created on: 11 Feb 2013
 *      Author: chris.barlow
 */
#include "sendCAN.h"
#include "../../CAN_Exchange/CAN_Tx_global.h"

Uint16 CANTimer;

void sendCAN_init(void)
{
	configureMailbox(CANPORT_A, 31, CAN_TX, ID_STD, CAN_TxMessages[0]->canID, CAN_TxMessages[0]->canDLC);

	CANTimer = 10;

	CANTx_clearAll();
}

void sendCAN_update(void)
{

	if((checkMailboxState(CANPORT_A, 31) == TX_FREE) && (CAN_TxMessages[0]->offset == 0))
	{
		CAN_TxMessages[0]->offset = (CAN_TxMessages[0]->period - 1);
		loadTxMailbox(CANPORT_A, 31, CAN_TxMessages[0]->canData);
		commitSendMailbox(CANPORT_A, 31);
	}
	else if(CAN_TxMessages[0]->offset > 0)
	{
		CAN_TxMessages[0]->offset--;
	}
}

void CANTx_clearAll(void)
{
	canData_Tx_7AB.dataVars.Db0_7 = 0;
	canData_Tx_7AB.dataVars.Db0_6 = 0;
	canData_Tx_7AB.dataVars.Db0_5 = 0;
	canData_Tx_7AB.dataVars.Db0_4 = 0;
	canData_Tx_7AB.dataVars.Db0_3 = 0;
	canData_Tx_7AB.dataVars.Db0_2 = 0;
	canData_Tx_7AB.dataVars.Db0_1 = 0;
	canData_Tx_7AB.dataVars.Db0_0 = 0;
	canData_Tx_7AB.dataVars.DB1 = 0;
	canData_Tx_7AB.dataVars.DB2 = 0;
	canData_Tx_7AB.dataVars.DB3 = 0;
	canData_Tx_7AB.dataVars.DB5_4 = 0;
	canData_Tx_7AB.dataVars.DB7_6 = 0;


	canData_Tx_651.dataVars.vs_state = 0;				/*B3*/

	/* Byte 2 */
	/* Byte 1 */
	/* Byte 0 */
	canData_Tx_651.dataVars.vs_xcount = 0;			/*B2,B1,B0*/

	/* Byte 7 */
	canData_Tx_651.dataVars.VSO_TEN = 0;
	canData_Tx_651.dataVars.Db7_1 = 0;					/*B7,b1 reserved*/
	canData_Tx_651.dataVars.Db7_2 = 0;					/*B7,b2 reserved*/
	canData_Tx_651.dataVars.Db7_3 = 0;					/*B7,b3 reserved*/
	canData_Tx_651.dataVars.Db7_4 = 0;					/*B7,b4 reserved*/
	canData_Tx_651.dataVars.VSO_WPMP = 0;
	canData_Tx_651.dataVars.VSO_WFAN = 0;
	canData_Tx_651.dataVars.VSO_PASINVSS = 0;

	/* Byte 6 */
	canData_Tx_651.dataVars.VSO_COMPINVSS = 0;
	canData_Tx_651.dataVars.VSO_PASINVRST = 0;
	canData_Tx_651.dataVars.VSO_COMPINRST = 0;
	canData_Tx_651.dataVars.VSO_DCDC = 0;
	canData_Tx_651.dataVars.VSO_BRKLAMP = 0;
	canData_Tx_651.dataVars.VSO_HVOK1 = 0;
	canData_Tx_651.dataVars.VSO_HVOK2 = 0;
	canData_Tx_651.dataVars.Db6_7 = 0;					/*B6,b7 reserved*/

	/* Byte 5 */
	canData_Tx_651.dataVars.VSI_DRVEN = 0;
	canData_Tx_651.dataVars.VSI_BCEN = 0;
	canData_Tx_651.dataVars.Db5_2 = 0;					/*B5,b2 reserved*/
	canData_Tx_651.dataVars.Db5_3 = 0;					/*B5,b3 reserved*/
	canData_Tx_651.dataVars.VSI_GSELFWD = 0;
	canData_Tx_651.dataVars.VSI_GSELREV = 0;
	canData_Tx_651.dataVars.VSI_REGENEN = 0;
	canData_Tx_651.dataVars.VSI_BRAKESW = 1;

	/* Byte 4 */
	canData_Tx_651.dataVars.Db4_0 = 0;					/*B4,b0 reserved*/
	canData_Tx_651.dataVars.VSI_PASINVERR = 0;
	canData_Tx_651.dataVars.VSI_COMPINVERR = 0;
	canData_Tx_651.dataVars.VSI_ABSACTIV = 0;
	canData_Tx_651.dataVars.Db4_4 = 0;					/*B4,b4 reserved*/
	canData_Tx_651.dataVars.Db4_5 = 0;					/*B4,b5 reserved*/
	canData_Tx_651.dataVars.VSI_BMSCORQ = 0;
	canData_Tx_651.dataVars.VSI_CPOW = 0;
}
