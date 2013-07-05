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
static char rxbuffer[200];
Uint16 rxbufferSize = (sizeof(rxbuffer)/sizeof(rxbuffer[0]));

typedef struct{
	Uint16 mp;
	Uint16 ID;
	Uint32 count;
} tempShadow_t;

tempShadow_t filtermap[FILTERSIZE];

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

				numRxCANMsgs = (i-2)/2;

				printf("%u\n", numRxCANMsgs);

				for(sequenceNum=0;sequenceNum<numRxCANMsgs;sequenceNum++){
					ID1 = rxbuffer[(2*sequenceNum)+1];
					ID2 = rxbuffer[(2*sequenceNum)+2];

					ID1 <<= 8;

					CAN_RxMessages[sequenceNum].canID = ID1|ID2;

					CAN_RxMessages[sequenceNum].counter = 0;
				}
				updateFilterRequired = 1;
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
    	for(i=0;i<FILTERSIZE;i++){
    		j = mailBoxFilters[i].messagePointer;
    		filtermap[i].mp = j;
    		filtermap[i].count = CAN_RxMessages[j].counter;
    		filtermap[i].ID = mailBoxFilters[i].canID;
    	}


    	scia_xmit('{');
    	scia_xmit('M');

        for(i=0;i<FILTERSIZE;i++){
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


        for(i=0;i<=5;i++){
        	j = (6*i)+pointerShift;

        	if(j<33){
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

    	break;

    default:
    	break;
    }





//    for(i=0;i<numRxCANMsgs;i++){
//		if(CAN_RxMessages[i].counter>0){
//			sprintf(msg,"{%03X}\n\0",(Uint16)CAN_RxMessages[i].canID);
//			scia_msg(msg);
//			CAN_RxMessages[i].counter--;
//		}
//    }
}
