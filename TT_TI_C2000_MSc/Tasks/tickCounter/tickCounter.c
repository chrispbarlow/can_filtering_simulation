/*
 * printTick2.c
 *
 *  Created on: 8 Feb 2013
 *      Author: chris.barlow
 */
#include "../../global.h"

extern unsigned long tickCount_G;

void tickCounter_update(void)
{
	tickCount_G++;
}
