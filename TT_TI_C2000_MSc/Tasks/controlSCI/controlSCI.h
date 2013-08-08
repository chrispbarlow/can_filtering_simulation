/*
 * controlSCI.h
 *
 *  Created on: 25 June 2013
 *      Author: chris.barlow
 */

#ifndef CONTROLSCI_H_
#define CONTROLSCI_H_

#define SEQ_TX_CHUNK_SIZE		(10)
#define SEQ_TX_CHUNK_SPACING	(6)

#include "DSP28x_Project.h"     // Device Headerfile and Examples Include File

void controlSCI_init(void);
void controlSCI_update(void);

void buildSequence(Uint16 listSize);

#endif /* CONTROLSCI_H_ */
