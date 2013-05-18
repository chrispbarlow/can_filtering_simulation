/*
 ============================================================================
 Name        : PCANalysis.c
 Author      : C Barlow
 Version     :
 Copyright   : 
 Description : Performs timing analysis on a PCAN trace
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFERSIZE 				(81)
#define FILTERSIZE 				(30)
#define MAX_TRACE_LINES			(0)  /* Set to zero to analyse entire trace */
#define LOGGING_TASK_PERIOD_us 	(1000)


#define FREEZE_TRIES 	(0)
#define MESSAGE_TIME_DELTA_MAX (100500)
#define MESSAGE_TIME_DELTA_MIN (0)

unsigned long  ID, timeDelta;
unsigned long  Task_WCET;
int noIDs;
unsigned int filterPointer;


typedef struct
{
	int canID;
	unsigned long counter;
	unsigned long loggedCounter;
} logging_Sequence_t;

typedef enum {TRUE, FALSE}flag_t;

typedef struct
{
	int canID;
	int sequencePointer;
	flag_t loggedFlag;
} filter_t;


logging_Sequence_t loggingSequence[BUFFERSIZE];
filter_t acceptanceFilter[FILTERSIZE];

void getSimpleCanSequence(char *filename, FILE *log);
void checkLogability(char *filename, FILE *log, int filterSize, int sequenceSize);
void orderSequence(void);
int countSequence(void);


int GetCAN1BufferPointer(unsigned int ID);

char *logFormat = "%4u.%06u 1  %3x             Tx%s";

int main(void)
{
	char *CANlogFile = "Logs/Log_for_analysis2.asc";
	int i, sequenceSize;

	FILE *outputFile = fopen("output.txt", "w");

	FILE *logFile = fopen("CAN_Logging_Simple_18-05-2013_Timing_perID_16_500us.txt", "w");

	noIDs = 0;

	getSimpleCanSequence(CANlogFile, logFile);

//	orderSequence();
	sequenceSize = countSequence();
	printf("\n\n\n");
	printf("\n\n %u ID's",sequenceSize);

	i = 16;

	printf("\n\n\nChecking logability...\r\n\n");
	fprintf(logFile,"\n\n,,Filter Size,Logged,Missed\n");
//	for(i = 1; i <= sequenceSize; i++)
	{
		checkLogability(CANlogFile, logFile, i, sequenceSize);
	}


	fclose(outputFile);
	fclose(logFile);

	return EXIT_SUCCESS;
}


void getSimpleCanSequence(char *filename, FILE *log)
{
	char inputStr[200];
	char canData[200];
	int i = 0;
	flag_t IDfound = FALSE;
	unsigned long timeNow_s ,timeNow_us, lineCounter;

	int ID;
	printf("Reading CAN log...\r\n");

	/* open trace file */
	FILE *bufferFile = fopen(filename, "r");

	for(i = 0; i < BUFFERSIZE; i++)
	{
		loggingSequence[i].canID = 0x000;
	}

	while((fgets(inputStr, 190, bufferFile) != NULL) && ((lineCounter++ < MAX_TRACE_LINES) || (MAX_TRACE_LINES == 0)))
	{
		/* Extract values from input string */
		unsigned int scanReturn = sscanf(inputStr, logFormat, &timeNow_s, &timeNow_us, &ID, &canData);
		if(scanReturn == 4)
		{
			printf("%u Sequencing log line: %s", scanReturn, inputStr);

			if(GetCAN1BufferPointer(ID) == 1)
			{
				i = 0;
				IDfound = FALSE;

				do
				{
					if(loggingSequence[i].canID == ID)
					{
						IDfound = TRUE;
						loggingSequence[i].counter++;
					}

					if(loggingSequence[i].canID != 0)
					{
						i++;
					}

				}while((loggingSequence[i].canID != 0) && (i < BUFFERSIZE) && (IDfound == FALSE));

				if((loggingSequence[i].canID == 0) && (IDfound == FALSE))
				{
					loggingSequence[i].canID = ID;
					loggingSequence[i].counter = 1;
				}
			}
		}

	}

	printf("Finished sequence:\n");

	i = 0;

	while((loggingSequence[i].canID != 0) && (i < BUFFERSIZE))
	{
		printf("0x%03X\n",loggingSequence[i].canID);
		i++;
	}

}

void checkLogability(char *filename, FILE *log, int filterSize, int sequenceSize)
{
	char inputStr[200];
	char canData[200];
	int i = 0, j = 0, sequencePointer = 0, sequencePointerStart = 0, IDLogCount = 0, IDMissedCount = 0;
	flag_t IDlogged = FALSE, IDfound = FALSE;
	unsigned long timeNow_s ,timeNow_us, timeOrigin;

	int ID;

	/* open trace file */
	FILE *bufferFile = fopen(filename, "r");


	for(i = 0; i < filterSize; i++)
	{
		acceptanceFilter[i].canID = loggingSequence[i].canID;
		acceptanceFilter[i].sequencePointer = i;
		acceptanceFilter[i].loggedFlag = FALSE;
		sequencePointer = i;
	}

	timeOrigin = 0;

	while(fgets(inputStr, 190, bufferFile) != NULL)
	{

		/* Extract values from input string */
		unsigned int scanReturn = sscanf(inputStr, logFormat, &timeNow_s, &timeNow_us, &ID, &canData);
		if(scanReturn == 4) /* valid line in trace */
		{

			timeNow_us += (timeNow_s * 1000000);

			/* Find timeNow origin */
			if(timeOrigin == 0)
			{
				timeOrigin = timeNow_us;
			}

			/* Find current time delta from origin */
			timeDelta = (timeNow_us - timeOrigin);

			printf("%u %lu\tChecking log line: %s", filterSize, timeDelta, inputStr);

			/* Logging task has run - replace logged IDs in filter */
			if(timeDelta >= LOGGING_TASK_PERIOD_us)
			{

				for(i = 0; i < filterSize; i++)
				{
					/* loggedFlag is set when ID is logged for first time */
					if(acceptanceFilter[i].loggedFlag == TRUE)
					{
						/* look through logging sequence for next ID not contained in acceptance filter */
						sequencePointerStart = sequencePointer;
						do
						{
							sequencePointer++;
							if(sequencePointer >= sequenceSize)
							{
								sequencePointer = 0;
							}

							IDfound = FALSE;

							for(j = 0; j < filterSize; j++)
							{
								if(acceptanceFilter[j].canID == loggingSequence[sequencePointer].canID)
								{
									IDfound = TRUE;
								}
							}

						}while((sequencePointer != sequencePointerStart) && (IDfound == TRUE));

						/* ID found not already in acceptance filter - replace Id in filter */
						if(IDfound == FALSE)
						{
							acceptanceFilter[i].canID = loggingSequence[sequencePointer].canID;
							acceptanceFilter[i].sequencePointer = sequencePointer;
							acceptanceFilter[i].loggedFlag = FALSE;
						}
					}
				}

				timeOrigin = timeNow_us;
			}

			/* ID is in logging list */
			if(GetCAN1BufferPointer(ID) == 1)
			{
				IDlogged = FALSE;
				i = 0;

				/* look for ID in acceptance filter */
				do
				{
					if(acceptanceFilter[i].canID == ID)
					{
						/* ID found, increment counters */
						IDLogCount++;
						loggingSequence[acceptanceFilter[i].sequencePointer].loggedCounter++;
						acceptanceFilter[i].loggedFlag = TRUE;

						IDlogged = TRUE;
					}

					i++;

				}while((IDlogged == FALSE) && (i < filterSize));

				/* ID not found in filter, so would be missed */
				if(IDlogged == FALSE)
				{
					IDMissedCount++;
				}
			}
		}
	}

	printf("filterSize: %u   Logged: %u    Missed %u\n", filterSize, IDLogCount, IDMissedCount);

//	for(i = 0; i < BUFFERSIZE; i++)
//	{
//		if(loggingSequence[i].canID != 0)
//		{
//			fprintf(log,",,0x%03X,%lu,%lu,%lu\n",loggingSequence[i].canID, loggingSequence[i].loggedCounter, (loggingSequence[i].counter - loggingSequence[i].loggedCounter), loggingSequence[i].counter);
//		}
//	}
//	fprintf(log,"\n");

	fprintf(log,",,%u,%u,%u\n", filterSize, IDLogCount, IDMissedCount);

}

/* Orders sequence by CAN ID */
void orderSequence(void)
{
	unsigned int i, j, minIDPrev = 0, minID = 0xFFFF, minIDPointer;
	logging_Sequence_t orderedSequence[BUFFERSIZE];

	for (i = 0; i < BUFFERSIZE; i++)
	{
		for(j = 0; j < BUFFERSIZE; j++)
		{
			if((loggingSequence[j].canID < minID) && (loggingSequence[j].canID > minIDPrev))
			{
				minID = loggingSequence[j].canID;
				minIDPointer = j;
			}
		}

		orderedSequence[i].canID = minID;
		orderedSequence[i].counter = loggingSequence[minIDPointer].counter;
		minIDPrev = minID;
		minID = 0xFFFF;
	}

	printf("\n\n");

	for(i = 0; i < BUFFERSIZE; i++)
	{
		if(orderedSequence[i].canID == 0xFFFF)
		{
			loggingSequence[i].canID = 0x000;
			loggingSequence[i].counter = 0;
		}
		else
		{
			loggingSequence[i].canID = orderedSequence[i].canID;
			loggingSequence[i].counter = orderedSequence[i].counter;

		}
		printf("0x%X\n",loggingSequence[i].canID);
	}
}

/* Counts number if ID's in sequence */
int countSequence(void)
{
	int i, counter = 0;

	printf("\n\n");

	for(i = 0; i < BUFFERSIZE; i++)
	{
		if(loggingSequence[i].canID != 0x000)
		{
			counter++;
			printf("case(0x%03X)\n",loggingSequence[i].canID);
		}
	}

	return counter;
}



/* Checks if ID is in logging list */
int GetCAN1BufferPointer(unsigned int ID)
{
	switch(ID)
	{
		case(0x187):
		case(0x188):
		case(0x189):
		case(0x18A):
		case(0x18B):
		case(0x18C):
		case(0x18D):
		case(0x18E):
		case(0x207):
		case(0x209):
		case(0x20B):
		case(0x20D):
		case(0x287):
		case(0x289):
		case(0x28B):
		case(0x28D):
		case(0x307):
		case(0x309):
		case(0x30B):
		case(0x30D):
		case(0x385):
		case(0x387):
		case(0x389):
		case(0x38B):
		case(0x38D):
		case(0x407):
		case(0x409):
		case(0x40B):
		case(0x40D):
		case(0x707):
		case(0x709):
		case(0x70B):
		case(0x70D):
			return 1;
			break;
		default:
			return -1;
			break;
	}

	return -1;
}
