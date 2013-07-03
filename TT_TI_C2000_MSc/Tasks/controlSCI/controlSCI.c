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


typedef enum{NEW,WAITING,SEND}SCIstate_t;
static char rxbuffer[200];
Uint16 rxbufferSize = (sizeof(rxbuffer)/sizeof(rxbuffer[0]));



void controlSCI_init(void)
{
	char *msg;

	// This function is found in the DSP2833x_Sci.c file.
	InitSciaGpio();
	scia_fifo_init();	   // Initialize the SCI FIFO
	scia_init();  // Initalize SCI for echoback

//	msg = "\r\n\n\nHello World!\0";
//	scia_msg(msg);
//
//	msg = "\r\nAll your CAN are tested with this\n\0";
//	scia_msg(msg);
}

void controlSCI_update(void)
{
	static SCIstate_t SCIstate = NEW;
    char *msg = 0;
    static Uint16 i = 0, j = 0;
    static Uint16 LoopCount = 0;
    Uint16 sret = 0;
    Uint16 ID1 = 0, ID2 = 0, ID3 = 0;
    char tempCharOut;

//    switch(SCIstate){
//    case NEW:
//    	msg = "\r\n\n\nEnter something: \0";
//		scia_msg(msg);
//		SCIstate = WAITING;
//		break;
//    case WAITING:
//    	if(SciaRegs.SCIFFRX.bit.RXFFST != 0){
//    		for(i=0;i<rxbufferSize;i++){
//    			rxbuffer[i] = 0;
//    		}
//    		i=0;
//    		SCIstate = SEND;
//    	}
//    	break;
//    case SEND:
//    	if(SciaRegs.SCIFFRX.bit.RXFFST != 0){
//    		rxbuffer[i] = SciaRegs.SCIRXBUF.all;
//    	}
//
//    	if((rxbuffer[i] == '\n')||(i>=rxbufferSize)){
//
//    		msg = "   Received: \0";
//    		scia_msg(msg);
//    		rxbuffer[i+1] = '\0'; /*scia_message() uses \0 to mark end of string*/
//    		scia_msg(rxbuffer);
//
//    		sret = sscanf(rxbuffer,"%03X,%03X,%03X",&ID1, &ID2, &ID3);
//
//			sprintf(rxbuffer,"%u 1: %03X  \0",sret, ID1);
//			scia_msg(rxbuffer);
//
//			sprintf(rxbuffer,"2: %03X  \0",ID2);
//			scia_msg(rxbuffer);
//
//			sprintf(rxbuffer,"3: %03X  \0",ID3);
//			scia_msg(rxbuffer);
//
//
//    		if(SciaRegs.SCIFFRX.bit.RXFFST == 0){
//    			SCIstate = NEW;
//
//    			i = 0;
//    		}
//    	}
//    	else if(rxbuffer[i] != '\0'){ /*terminal sends \0 after each character typed*/
//    		i++;
//    	}
//    	break;
//    default:
//    	break;
//    }
//    tempCharOut = (char)(i & 0xff);
//    msg[0] = tempCharOut;
//    msg[1] = (',');
//    tempCharOut = (char)((CAN_RxMessages[i].canID>>8)&0xFF);
//    msg[2] = tempCharOut;
//    tempCharOut = (char)(CAN_RxMessages[i].canID&0xFF);
//    msg[3] = tempCharOut;
//    msg[4] = (',');
//    tempCharOut = (char)((CAN_RxMessages[i].counter>>8)&0xFF);
//    msg[5] = tempCharOut;
//    tempCharOut = (char)(CAN_RxMessages[i].counter&0xFF);
//    msg[6] = tempCharOut;
//    msg[7] = ('\n');
//    msg[8] = ('\0');

	scia_xmit('~');
	scia_xmit('~');
	scia_xmit('~');

    for(i=0;i<16;i++){
		j = mailBoxFilters[i].messagePointer;

    	scia_xmit('M');

		tempCharOut = (i & 0xff);
		scia_xmit(tempCharOut);

		scia_xmit(',');

		tempCharOut = ((j)&0xFF);
		scia_xmit(tempCharOut);

		scia_xmit('~');

		scia_xmit('S');

		tempCharOut = (j & 0xff);
		scia_xmit(tempCharOut);

		scia_xmit(',');

		tempCharOut = ((CAN_RxMessages[j].counter>>8)&0xFF);
		scia_xmit(tempCharOut);
		tempCharOut = ((CAN_RxMessages[j].counter)&0xFF);
		scia_xmit(tempCharOut);

		scia_xmit('~');

   }

//    for(i=0;i<numRxCANMsgs;i++){
//		if(CAN_RxMessages[i].counter>0){
//			sprintf(msg,"{%03X}\n\0",(Uint16)CAN_RxMessages[i].canID);
//			scia_msg(msg);
//			CAN_RxMessages[i].counter--;
//		}
//    }
}
