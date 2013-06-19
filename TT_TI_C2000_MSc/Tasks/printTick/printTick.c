/*
 * printTick.c
 *
 *  Created on: 8 Feb 2013
 *      Author: chris.barlow
 */
#include "printTick.h"

//extern unsigned long tickCount_G;

void printTick_update(void)
{
	printf("Tick %lu\n", tickCount_G);
}
