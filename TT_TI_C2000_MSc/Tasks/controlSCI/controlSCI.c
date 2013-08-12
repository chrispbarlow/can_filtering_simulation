/***********************************************************************************************************
 * controlSCI.c
 *
 * Controls the serial data transfer between device and desktop application via the TI SCI port
 *
 * Created on: 25 June 2013
 *      Author: chris.barlow
 * *********************************************************************************************************/

#include "../../Lib/SCI/SCI.h"
#include "controlSCI.h"
#include <stdio.h>
#include "../../CAN_Exchange/CAN_Rx_global.h"

/* SCI states */
typedef enum{WAITING,RECEIVE,SEND}SCIstate_t;

/* Raw character receive buffer */
static char rxbuffer[300];
Uint16 rxbufferSize = (sizeof(rxbuffer)/sizeof(rxbuffer[0]));

/* Position control for packet data */
enum {
	FSC_DATAPOSITION = 1,
	DUP_DATAPOSITION = 2,
	IDH_DATAPOSITION = 3,
	IDL_DATAPOSITION = 4,
	DLC_DATAPOSITION = 5,
	CYT_DATAPOSITION = 6
};

/* Temporary arrays for data unpacking */
typedef struct{
	Uint16 sequenceIndex_SCITx;
	Uint16 canID_SCITx;
} tempShadow_t;
tempShadow_t mailBoxFilterShadow_SCITx[NUM_MESSAGES_MAX];

typedef struct{
	Uint16 canID_SCIRx;
	Uint16 canDLC_SCIRx;
	Uint16 cycleTime_SCIRx;
} logging_list_t;
logging_list_t loggingList_SCIRx[NUM_MESSAGES_MAX];



/***********************************************************************************************************
 * Initialisation - called once when the device boots, before the scheduler starts.
 * *********************************************************************************************************/
void controlSCI_init(void)
{
	/* This TI function is found in the DSP2833x_Sci.c file. */
	InitSciaGpio();
	scia_fifo_init();	  	/* Initialize the SCI FIFO */
	scia_init();  			/* Initalize SCI for echoback */
}



/***********************************************************************************************************
 * Update function - called periodically from scheduler
 * *********************************************************************************************************/
void controlSCI_update(void)
{
	static SCIstate_t SCIstate = WAITING;
    static Uint16 i = 0, j = 0;
    Uint32 IDH = 0, IDL = 0;
    char tempCharOut;
    static Uint16 indexShift = 0;
    Uint16 sequenceNum = 0;


    /* state machine controls whether the device is transmitting or receiving logging list information
     * will always receive until first logging list is received */
    switch(SCIstate){

    case WAITING:
      	/* First character received induces RECEIVE state */
    	if(SciaRegs.SCIFFRX.bit.RXFFST != 0){
    		for(i=0;i<rxbufferSize;i++){
    			rxbuffer[i] = 0;
    		}
    		i=0;
    		SCIstate = RECEIVE;
    	}
    	break;

    case RECEIVE:

    	/* Checks SCI receive flag for new character */
    	if(SciaRegs.SCIFFRX.bit.RXFFST != 0){
    		rxbuffer[i] = SciaRegs.SCIRXBUF.all;

     		/* "???" sent by desktop app indicates that a reset is required (someone pressed the 'R' key) */
         	if((rxbuffer[i] == '?')&&(rxbuffer[i-1] == '?')&&(rxbuffer[i-2] == '?')){
         		SCIstate = WAITING;
         	}

         	/* Received data packet looks like this:
         	 * 		index:  0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16
         	 * 		chars:  { f d a a A X b b B  Y  c  c  C  Z  ~  }
         	 * Where:
         	 * 		f is the filter size control constant
         	 * 		d is the duplication control constant
         	 *  	aa is two byte CAN ID
         	 * 		A is the CAN data length
         	 *  	X is the CAN message cycle time
         	 *  	etc
         	 * */
			if((i>0)&&(rxbuffer[i-1] == '~')&&(rxbuffer[i] == '}')){

				/* In above eg, i = 16 at end of packet, numRxCANMsgs_G = 3 */
				numRxCANMsgs_G = (i-4)/4;

				/* Safeguard against mailbox overload */
				if(rxbuffer[FSC_DATAPOSITION] <= NUM_MAILBOXES_MAX){
					filterSize_G = rxbuffer[FSC_DATAPOSITION];
				}
				else{
					filterSize_G = 32;
				}

				/* Unpackaging logging list info from data packet */
				for(sequenceNum=0;sequenceNum<numRxCANMsgs_G;sequenceNum++){
					IDH = rxbuffer[(4*sequenceNum)+IDH_DATAPOSITION];
					IDH <<= 8;
					IDL = rxbuffer[(4*sequenceNum)+IDL_DATAPOSITION];

					loggingList_SCIRx[sequenceNum].canID_SCIRx = (IDH|IDL);
					loggingList_SCIRx[sequenceNum].canID_SCIRx &= 0x7FF;
					loggingList_SCIRx[sequenceNum].canDLC_SCIRx = rxbuffer[(4*sequenceNum)+DLC_DATAPOSITION];
					loggingList_SCIRx[sequenceNum].cycleTime_SCIRx = rxbuffer[(4*sequenceNum)+CYT_DATAPOSITION];
				}

				/* Initialise sequence */
				buildSequence(numRxCANMsgs_G);

				/* flag tells receiveCAN to update the logging sequence */
				updateSequenceRequired_G = RESET;

				SCIstate = SEND;
			}
			else if(rxbuffer[0] == '{'){
				/* data packet reception still in progress */
				i++;

				/* Reset state if buffer overflows - can happen if data loss occurs */
				if(i >= (sizeof(rxbuffer)/sizeof(rxbuffer[0]))){
					SCIstate = WAITING;
				}
			}
    	}
    	break;


    case SEND:
		/* check for reset request from desktop app */
    	if(SciaRegs.SCIFFRX.bit.RXFFST != 0){
    		rxbuffer[0] = SciaRegs.SCIRXBUF.all;
    	}

     	if(rxbuffer[0] == '?'){
     		SCIstate = WAITING;
     	}
     	else{
			/* Take snapshot of filters (should prevent updates halfway through transmission)*/
			for(i=0;i<filterSize_G;i++){
				j = mailBoxFilterShadow_G[i].sequenceIndex_mapped;
				mailBoxFilterShadow_SCITx[i].sequenceIndex_SCITx = j;
				mailBoxFilterShadow_SCITx[i].canID_SCITx = mailBoxFilterShadow_G[i].canID_mapped;
			}

			/* Transmit mailbox data
			 *
			 * Data packet looks like this:
			 *    index:  0 1 2 3 4 5 6 7
			 *    chars:  { M A a a X ~ }
			 * Where:
			 *    A is the sequence location mapped to mailbox
			 *    aa is two byte CAN ID
			 *    X mailbox location
			 *    This is fixed length.
			 * */
			scia_xmit('{');
			scia_xmit('M');

			for(i=0;i<filterSize_G;i++){
				j = mailBoxFilterShadow_SCITx[i].sequenceIndex_SCITx;

				tempCharOut = ((mailBoxFilterShadow_SCITx[i].canID_SCITx>>8) & 0xFF);
				scia_xmit(tempCharOut);

				tempCharOut = (mailBoxFilterShadow_SCITx[i].canID_SCITx & 0xFF);
				scia_xmit(tempCharOut);

				tempCharOut = (j & 0xFF);
				scia_xmit(tempCharOut);
		    }

			scia_xmit('~');
			scia_xmit('}');

			/* Transmit message counts
			 * Due to the large amount of data for the message counts
			 * Data is transmitted as max 10 values, 6 apart, offset by pointerShift
			 *
			 * Data packet looks like this:
			 *     index:  0 1 2 3 4 5 6 7 8
			 *     chars:  { S A a a a a ~ }
			 * Where:
		 	 *    A is the sequence location
		 	 *    aaaa is four byte hit count for the sequence location
		 	 *
			 *    This is fixed length.
			 * */
			for(i=0;i<=SEQ_TX_CHUNK_SIZE;i++){

				j = (i*SEQ_TX_CHUNK_SPACING)+indexShift;

				if(j<=numRxCANMsgs_G){
					scia_xmit('{');
					scia_xmit('S');
					/* Need to tell the desktop app the array index for this value */
					tempCharOut = (j & 0xff);
					scia_xmit(tempCharOut);

					/* Send the 32-bit counter value */
					tempCharOut = ((CAN_RxMessages_G[j].counter>>24)&0xFF);
					scia_xmit(tempCharOut);
					tempCharOut = ((CAN_RxMessages_G[j].counter>>16)&0xFF);
					scia_xmit(tempCharOut);
					tempCharOut = ((CAN_RxMessages_G[j].counter>>8)&0xFF);
					scia_xmit(tempCharOut);
					tempCharOut = ((CAN_RxMessages_G[j].counter)&0xFF);
					scia_xmit(tempCharOut);

					scia_xmit('~');
					scia_xmit('}');
				}

		   }

			/* Increment pointerShift to inter-space next set of values next time */
			indexShift++;
			if(indexShift > 6){
				indexShift = 0;
			}

			/* Instruct desktop app to refresh screen */
			scia_xmit('{');
			scia_xmit('~');
			scia_xmit('}');
    	}
    	break;

    default:
    	break;
    }

    /* ? symbol acts as a handshake request with the desktop app */
	scia_xmit('?');

}



/***********************************************************************************************************
 * Copies sequence details from temporary buffers to global message sequence array.
 * Since we don't know where in the sequence we will start, the schedule timer for all messages is set to 1.
 * *********************************************************************************************************/
void buildSequence(Uint16 listSize){
	Uint16 i, cycleTime_min, newReload, remainder = 0;

	/* Finds the minimum cycle time in the logging list */
 	cycleTime_min = 0xFFFF;
 	for(i=0;i<listSize;i++){
 		if(loggingList_SCIRx[i].cycleTime_SCIRx < cycleTime_min){
 			cycleTime_min = loggingList_SCIRx[i].cycleTime_SCIRx;
 		}
 	}

 	for(i=0;i<listSize;i++){
		CAN_RxMessages_G[i].canID = loggingList_SCIRx[i].canID_SCIRx;
		CAN_RxMessages_G[i].canData.rawData[0] = 0;
		CAN_RxMessages_G[i].canData.rawData[1] = 0;
		CAN_RxMessages_G[i].canDLC = loggingList_SCIRx[i].canDLC_SCIRx;

		/* timer_reload set proportionally to weight the filter in favour of more frequent IDs */
		newReload = (loggingList_SCIRx[i].cycleTime_SCIRx / cycleTime_min);
		/* Rounding logic */
		remainder = (loggingList_SCIRx[i].cycleTime_SCIRx % cycleTime_min);
		if((remainder > 0)&&(remainder >= (cycleTime_min/2))){
			CAN_RxMessages_G[i].timer_reload = (newReload + 1);
		}
		else{
			CAN_RxMessages_G[i].timer_reload = newReload;
		}

		/* Force all timers to 1 for first iteration - level playing field */
		CAN_RxMessages_G[i].timer = 1;
		CAN_RxMessages_G[i].counter = 0;
 	}
 }
