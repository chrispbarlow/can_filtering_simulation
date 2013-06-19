/*
 * printTick2.c
 *
 *  Created on: 8 Feb 2013
 *      Author: chris.barlow
 */

#include "printTick2.h"
//extern unsigned long tickCount_G;

void printTick2_update(void)
{
	printf("Tock %lu\n", tickCount_G);
}
