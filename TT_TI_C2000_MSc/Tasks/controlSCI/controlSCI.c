/*
 * controlSCI.c
 *
 *  Created on: 25 June 2013
 *      Author: chris.barlow
 */
#include "../../Lib/SCI/SCI.h"
#include "controlSCI.h"
#include <stdio.h>
#include "../../CAN_Exchange/CAN_Rx_global.h"


typedef enum{WAITING,RECEIVE,SEND}SCIstate_t;
static char rxbuffer[300];
Uint16 rxbufferSize = (sizeof(rxbuffer)/sizeof(rxbuffer[0]));

typedef struct{
	Uint16 mp;
	Uint16 ID;
	Uint32 count;
} tempShadow_t;

typedef struct{
	Uint16 canID;
	Uint16 canDLC;
	Uint16 cycleTime;
} logging_list_t;

enum {
	IDH_DATAPOSITION = 1,
	IDL_DATAPOSITION = 2,
	DLC_DATAPOSITION = 3,
	CYT_DATAPOSITION = 4
};


logging_list_t loggingList[64];

tempShadow_t filtermap[64];

void controlSCI_init(void)
{
	/* This TI function is found in the DSP2833x_Sci.c file. */
	InitSciaGpio();
	scia_fifo_init();	  	/* Initialize the SCI FIFO */
	scia_init();  			/* Initalize SCI for echoback */
}

void controlSCI_update(void)
{
	static SCIstate_t SCIstate = WAITING;
    static Uint16 i = 0, j = 0;
    Uint32 IDH = 0, IDL = 0;
    char tempCharOut;
    static Uint16 pointerShift = 0;
    Uint16 sequenceNum = 0;


    /*
     * state machine controls whether the device is transmitting or receiving logging list information
     * will always receive until first logging list is received
     * */
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

         	/* *
         	 * Data packet looks like this:
         	 * 		 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14
         	 * 		"{ a a A X b b B Y c  c  C  Z  ~  }" where:
         	 *  	aa is two byte CAN ID
         	 * 		A is the CAN data length
         	 *  	X is the CAN message cycle time
         	 *  	etc
         	 * */
			if((i>0)&&(rxbuffer[i-1] == '~')&&(rxbuffer[i] == '}')){

				/* In above eg, i = 14 at end of packet, numRxCANMsgs_G = 3 */
				numRxCANMsgs_G = (i-2)/4;

				/* Unpackaging logging list info from data packet */
				for(sequenceNum=0;sequenceNum<numRxCANMsgs_G;sequenceNum++){
					IDH = rxbuffer[(4*sequenceNum)+IDH_DATAPOSITION];
					IDH <<= 8;
					IDL = rxbuffer[(4*sequenceNum)+IDL_DATAPOSITION];

					loggingList[sequenceNum].canID = (IDH|IDL);
					loggingList[sequenceNum].canID &= 0x7FF;
					loggingList[sequenceNum].canDLC = rxbuffer[(4*sequenceNum)+DLC_DATAPOSITION];
					loggingList[sequenceNum].cycleTime = rxbuffer[(4*sequenceNum)+CYT_DATAPOSITION];
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
				j = mailBoxFilters[i].messagePointer;
				filtermap[i].mp = j;
				filtermap[i].count = CAN_RxMessages_G[j].counter;
				filtermap[i].ID = mailBoxFilters[i].canID;
			}

			/* Transmit mailbox data */
			scia_xmit('{');
			scia_xmit('M');

			for(i=0;i<filterSize_G;i++){
				j = filtermap[i].mp;

				tempCharOut = ((filtermap[i].ID>>8) & 0xFF);
				scia_xmit(tempCharOut);

				tempCharOut = (filtermap[i].ID & 0xFF);
				scia_xmit(tempCharOut);

				tempCharOut = (j & 0xFF);
				scia_xmit(tempCharOut);
		   }

			scia_xmit('~');
			scia_xmit('}');

			/* Transmit message counts */
			for(i=0;i<=10;i++){
				/* Due to the large amount of data for the message counts
				 * Data is transmitted as max 10 values, 6 apart, offset by pointerShift*/
				j = (6*i)+pointerShift;

				if(j<=numRxCANMsgs_G){
					scia_xmit('{');
					scia_xmit('S');
					/* Need to tell the desktop app the array index for this value */
					tempCharOut = (j & 0xff);
					scia_xmit(tempCharOut);

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
			pointerShift++;
			if(pointerShift > 6){
				pointerShift = 0;
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

/* Iterates through logging sequence (CAN_Rx_global.c) and updates / resets values */
void buildSequence(Uint16 listSize){
	Uint16 i, cycleTime_min, newReload, remainder = 0;

	/* Finds the minimum cycle time in the logging list */
 	cycleTime_min = 0xFFFF;
 	for(i=0;i<listSize;i++){
 		if(loggingList[i].cycleTime<cycleTime_min){
 			cycleTime_min = loggingList[i].cycleTime;
 		}
 	}

 	for(i=0;i<listSize;i++){
		CAN_RxMessages_G[i].canID = loggingList[i].canID;
		CAN_RxMessages_G[i].canData.rawData[0] = 0;
		CAN_RxMessages_G[i].canData.rawData[1] = 0;
		CAN_RxMessages_G[i].canDLC = loggingList[i].canDLC;

		/* timer_reload set proportionally to weight the filter in favour of more frequent IDs */
		newReload = loggingList[i].cycleTime/cycleTime_min;
		/* Rounding logic */
		remainder = loggingList[i].cycleTime%cycleTime_min;
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
