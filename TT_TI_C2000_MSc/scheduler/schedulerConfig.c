/*
 * schedule.c
 *
 *  Created on: 8 Feb 2013
 *      Author: chris.barlow
 */

#include "schedulerConfig.h"

volatile task_t Tasks[] =
{
	{
		handleCAN_update,		/* function pointer */
		1,						/* period in ticks */
		125,					/* initial offset in ticks */
		IN_SCHEDULER
	},

	{
		sendCAN_update,			/* function pointer */
		10,						/* period in ticks */
		126,					/* initial offset in ticks */
		IN_SCHEDULER
	},

	{
		receiveCAN_update,		/* function pointer */
		2,						/* period in ticks */
		127,					/* initial offset in ticks */
		IN_SCHEDULER
	},

};

const unsigned int tasksInSchedule = (sizeof(Tasks) / sizeof(Tasks[0]));

void Tasks_Init(void)
{
	sendCAN_init();
	receiveCAN_init();
}
