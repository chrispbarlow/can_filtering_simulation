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


typedef enum{NEW,WAITING,RECEIVE,SEND}SCIstate_t;
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



logging_list_t loggingList[64];

tempShadow_t filtermap[64];

void controlSCI_init(void)
{
	// This function is found in the DSP2833x_Sci.c file.
	InitSciaGpio();
	scia_fifo_init();	   // Initialize the SCI FIFO
	scia_init();  // Initalize SCI for echoback
}

void controlSCI_update(void)
{
	static SCIstate_t SCIstate = NEW;
    static Uint16 i = 0, j = 0;
    Uint32 ID1 = 0, ID2 = 0;
    char tempCharOut;
    static Uint16 pointerShift = 0;
    Uint16 sequenceNum = 0;

    switch(SCIstate){
    case NEW:
    	/* This doesn't do anything anymore */
 		SCIstate = WAITING;
		break;


    case WAITING:
      	scia_xmit('?');
    	if(SciaRegs.SCIFFRX.bit.RXFFST != 0){
    		for(i=0;i<rxbufferSize;i++){
    			rxbuffer[i] = 0;
    		}
    		i=0;
    		SCIstate = RECEIVE;
    	}
    	break;


    case RECEIVE:
    	if(SciaRegs.SCIFFRX.bit.RXFFST != 0){
    		rxbuffer[i] = SciaRegs.SCIRXBUF.all;
    	}

     	if(rxbuffer[i] != 0){

			if((i>0)&&(rxbuffer[i-1] == '~')&&(rxbuffer[i] == '}')){

				numRxCANMsgs_G = (i-2)/4;

				printf("%u\n", numRxCANMsgs_G);

				for(sequenceNum=0;sequenceNum<numRxCANMsgs_G;sequenceNum++){
					ID1 = rxbuffer[(4*sequenceNum)+1];
					ID1 <<= 8;
					ID2 = rxbuffer[(4*sequenceNum)+2];

					loggingList[sequenceNum].canID = (ID1|ID2);
					loggingList[sequenceNum].canDLC = rxbuffer[(4*sequenceNum)+3];
					loggingList[sequenceNum].cycleTime = rxbuffer[(4*sequenceNum)+4];
				}

				buildSequence(numRxCANMsgs_G);

				updateFilterRequired_G = 1;
				SCIstate = SEND;

			}
			else if(rxbuffer[0] == '{'){ /*terminal sends \0 after each character typed*/
				i++;
			}
			scia_xmit('?');
    	}
    	break;


    case SEND:
    	scia_xmit('{');
    	scia_xmit('~');
    	scia_xmit('}');
    	/* Take snapshot of filters (should prevent updates halfway through transmission)*/
    	for(i=0;i<filterSize_G;i++){
    		j = mailBoxFilters[i].messagePointer;
    		filtermap[i].mp = j;
    		filtermap[i].count = CAN_RxMessages[j].counter;
    		filtermap[i].ID = mailBoxFilters[i].canID;
    	}


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


        for(i=0;i<=10;i++){
        	j = (6*i)+pointerShift;

        	if(j<=numRxCANMsgs_G){
    			scia_xmit('{');
    			scia_xmit('S');

    			tempCharOut = (j & 0xff);
    			scia_xmit(tempCharOut);

    			tempCharOut = ((CAN_RxMessages[j].counter>>24)&0xFF);
    			scia_xmit(tempCharOut);
    			tempCharOut = ((CAN_RxMessages[j].counter>>16)&0xFF);
    			scia_xmit(tempCharOut);
    			tempCharOut = ((CAN_RxMessages[j].counter>>8)&0xFF);
    			scia_xmit(tempCharOut);
    			tempCharOut = ((CAN_RxMessages[j].counter)&0xFF);
    			scia_xmit(tempCharOut);

    			scia_xmit('~');
    			scia_xmit('}');
        	}
       }

        pointerShift++;
        if(pointerShift > 6){
    		pointerShift = 0;
    	}

        scia_xmit('?');

    	break;

    default:
    	break;
    }
}

void buildSequence(Uint16 listSize){
	Uint16 i, cycleTime_min;

 	cycleTime_min = 0xFFFF;
 	for(i=0;i<listSize;i++){
 		if(loggingList[i].cycleTime<cycleTime_min){
 			cycleTime_min = loggingList[i].cycleTime;
 		}
 	}

 	for(i=0;i<listSize;i++){
		CAN_RxMessages[i].canID = loggingList[i].canID;
		CAN_RxMessages[i].canData.rawData[0] = 0;
		CAN_RxMessages[i].canData.rawData[1] = 0;
		CAN_RxMessages[i].canDLC = loggingList[i].canDLC;
		CAN_RxMessages[i].timer_reload = loggingList[i].cycleTime/cycleTime_min;
		CAN_RxMessages[i].timer = 0;
		CAN_RxMessages[i].counter = 0;
 	}
 }
