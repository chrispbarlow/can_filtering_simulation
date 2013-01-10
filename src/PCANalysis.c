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

#define BUFFERSIZE 		(363)
#define FILTERSIZE 		(30)
#define MAX_TRACE_LINES	(0)  /* Set to zero to analyse entire trace */


#define FREEZE_TRIES 	(0)
#define MESSAGE_TIME_DELTA_MAX (100500)
#define MESSAGE_TIME_DELTA_MIN (0)

unsigned long  ID, timeDelta;
unsigned long  Task_WCET;
int noIDs;
unsigned int filterPointer;

typedef struct
{
	unsigned int canID;					/* Message ID */
	unsigned int logCounter;			/* Total number of times ID has appeared in log */
	unsigned int timeDeltaMin;			/* Minimum time difference between occurrences of ID */
	unsigned int timeDeltaMax;			/* Maximum time difference between occurrences of ID */
	unsigned long prevLoggedTime;		/* Time of previous occurrence */
	unsigned long messageJitterMax;		/* Difference between timeDeltaMin and timeDeltaMax */
	unsigned int prevCanID;				/* ID of previous message */
	unsigned int passedCount;			/* Number of times message appears in sequence */
	unsigned long frozenTime;			/* Time message order was frozen */
	unsigned int frozenTriesCount;		/* Counter for freezing */
	unsigned int deltaAvCounter;		/* Counter for mean calculation */
	unsigned long deltaAv;				/* Average mean time delta */
	unsigned long long deltaAvSum;		/* Sum used in mean calculation */
	unsigned long messageOffset;			/* First time ID occurs */
	unsigned long messageOffsetDelta;	/* Difference between offsets */
	unsigned int beforeTargetCount;		/* Number of times message arrives before target buffer */
	unsigned int afterTargetCount;		/* Number of times message arrives after target buffer */
	unsigned int onTargetCount;			/* Number of times message arrives within target buffer */
	unsigned int clashCount;			/* Number of times message clashes with processing of previous message */
	unsigned int orderPointer;			/* Pointer to ordered list */
	unsigned long projectedNextArrival;	/* Projected time for next occurrence of ID */
} CAN_message_t;

typedef struct
{
	int canID;
	unsigned long counter;
	unsigned long loggedCounter;
} logging_Sequence_t;

typedef struct
{
	int canID;
	int sequencePointer;
} filter_t;

typedef enum {TRUE, FALSE}flag_t;

logging_Sequence_t loggingSequence[BUFFERSIZE];
filter_t acceptanceFilter[FILTERSIZE];

CAN_message_t CAN1Buffer[BUFFERSIZE];
CAN_message_t OrderedMessages[BUFFERSIZE];

void getCanSequence(char *filename, FILE *log);
void getSimpleCanSequence(char *filename, FILE *log);
void checkLogability(char *filename, FILE *log, int filterSize);
void orderSequence(void);
int countSequence(void);




void getCanTiming(char *filename, FILE *log);
void getPrecisionCanTiming(char *filename, FILE *log);
void calculateAverages(void);
void csvOutput(char *filename);
int orderMessages(void);
void summaryOutput(FILE *csv);
void changeFilter(void);


int GetCAN1BufferPointer(unsigned int ID);

char *logFormat = "%4u.%06u 1  %3x             Tx%s";

int main(void)
{
	char *CANlogFile = "Log_for_analysis2.asc";
	int i, sequenceSize;

	FILE *outputFile = fopen("output.txt", "w");

	FILE *logFile = fopen("CAN_Logging_Simple_10-01-2013_Individual_Counter.csv", "w");

	noIDs = 0;

	getSimpleCanSequence(CANlogFile, logFile);

//	orderSequence();
	sequenceSize = countSequence();

	printf("\n\n %u ID's",sequenceSize);

	printf("\n\n\nChecking logability...\r\n\n");
	fprintf(logFile,"\n\n,,Filter Size,Logged,Missed\n");
//	for(i = 0; i < sequenceSize; i++)
	{
		checkLogability(CANlogFile, logFile, 15);
	}

//	getCanSequence(CANlogFile, logFile);
//
//	getCanTiming(CANlogFile, logFile);
//
//	noIDs = orderMessages();
//
//	calculateAverages();
//
//
//	getPrecisionCanTiming(CANlogFile, logFile);
//
//	summaryOutput(outputFile);
//
//	csvOutput("CANtiming.csv");

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

			if(GetCAN1BufferPointer(ID) != -1)
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

void checkLogability(char *filename, FILE *log, int filterSize)
{
	char inputStr[200];
	char canData[200];
	int i = 0, j = 0, sequencePointer = 0, sequencePointerStart = 0, IDLogCount = 0, IDMissedCount = 0;
	flag_t IDlogged = FALSE, IDfound = FALSE;
	unsigned long timeNow_s ,timeNow_us;

	int ID;

	/* open trace file */
	FILE *bufferFile = fopen(filename, "r");


//	printf("Initial filter:\n");

	for(i = 0; i <= filterSize; i++)
	{
		acceptanceFilter[i].canID = loggingSequence[i].canID;
		acceptanceFilter[i].sequencePointer = i;
//		printf("0x%03X\n",acceptanceFilter[i]);
		sequencePointer = i;
	}

	while(fgets(inputStr, 190, bufferFile) != NULL)
	{
		IDlogged = FALSE;
		IDfound = FALSE;
		i = 0;

		/* Extract values from input string */
		unsigned int scanReturn = sscanf(inputStr, logFormat, &timeNow_s, &timeNow_us, &ID, &canData);
		if((scanReturn == 4) && (GetCAN1BufferPointer(ID) != -1))
		{
			printf("%u Checking log line: %s", filterSize, inputStr);
//			fprintf(log,"Found ID: 0x%03X ", ID);

			do
			{
				if(acceptanceFilter[i].canID == ID)
				{
					IDLogCount++;
					loggingSequence[acceptanceFilter[i].sequencePointer].loggedCounter++;

					sequencePointerStart = sequencePointer;
					do
					{
						sequencePointer++;
						if(loggingSequence[sequencePointer].canID == 0)
						{
							sequencePointer = 0;
						}

						IDfound = FALSE;

						for(j = 0; j <= filterSize; j++)
						{
							if(acceptanceFilter[j].canID == loggingSequence[sequencePointer].canID)
							{
								IDfound = TRUE;
							}

						}

					}while((sequencePointer != sequencePointerStart) && (IDfound == TRUE));

					acceptanceFilter[i].canID = loggingSequence[sequencePointer].canID;
					acceptanceFilter[i].sequencePointer = sequencePointer;

					IDlogged = TRUE;
//					fprintf(log,"Logged in %u, replaced with 0x%03X\n", i, loggingSequence[sequencePointer]);
				}

				i++;

			}while((IDlogged == FALSE) && (i <= filterSize));

			if(IDlogged == FALSE)
			{
				IDMissedCount++;
//				fprintf(log,"Missed\n");
			}
		}
	}

	printf("filterSize: %u   Logged: %u    Missed %u\n", filterSize, IDLogCount, IDMissedCount);
	fprintf(log,",,%u,%u,%u\n", filterSize, IDLogCount, IDMissedCount);

	for(i = 0; i < BUFFERSIZE; i++)
	{
		if(loggingSequence[i].canID != 0)
		{
			printf("0x%03X: %lu of %lu   Missed: %lu\n",loggingSequence[i].canID, loggingSequence[i].loggedCounter, loggingSequence[i].counter, (loggingSequence[i].counter - loggingSequence[i].loggedCounter));
		}
	}


//	i = 0;
//	while((loggingSequence[i] != 0) && (i < BUFFERSIZE))
//	{
//		printf("0x%03X\n",loggingSequence[i]);
//		i++;
//	}
//
//	printf("\n\nFilter:\n\n");
//	i = 0;
//	while((acceptanceFilter[i] != 0) && (i < filterSize))
//	{
//		printf("0x%03X\n",acceptanceFilter[i]);
//		i++;
//	}

}



void getCanSequence(char *filename, FILE *log)
{
	char inputStr[200];
	char canData[200];
	unsigned int pointer;
	unsigned long timeNow_s ,timeNow_us, timeDelta, logTimeDelta, prevLogTimeDelta, timeOrigin = 0;

	int ID, prevID;
	printf("Reading CAN log...\r\n");

	/* open trace file */
	FILE *bufferFile = fopen(filename, "r");
	timeOrigin = 0;

	/* Read trace file one line at a time until end of file */
	while(fgets(inputStr, 190, bufferFile) != NULL)
	{
		/* Extract values from input string */
		unsigned int scanReturn = sscanf(inputStr, logFormat, &timeNow_s, &timeNow_us, &ID, &canData);

//		if(inputStr[0] != 'NULL')
		{
			printf("%u Sequencing log line: %s", scanReturn, inputStr);
		}
			timeNow_us += (timeNow_s * 1000000);

			/* Find pointer for buffer array (v11X logic) */
			pointer = GetCAN1BufferPointer(ID);

			if(scanReturn == 4)
			{

				/* Don't count if repeated or not in ID list */
				if((pointer != 9999) && (ID != prevID))
				{

					/* Find timeNow origin */
					if(timeOrigin == 0)
					{
						timeOrigin = timeNow_us;
					}

					/* Find current time delta from origin */
					timeDelta = (timeNow_us - timeOrigin);

					/* Find time delta since last message */
					logTimeDelta = timeDelta - prevLogTimeDelta;

					CAN1Buffer[pointer].logCounter++;
					/* file output */
					fprintf(log,"Time: %11lu  ID: 0x%03X  Instance: %7u Time dif: %6lu",
							timeDelta,
							ID,
							CAN1Buffer[pointer].logCounter,
							logTimeDelta);

					if(CAN1Buffer[pointer].prevCanID != prevID)
					{
					/* sequence freezing logic */
						fprintf(log, "Expected after 0x%03X", CAN1Buffer[pointer].prevCanID);

						/* Keep updating previous ID until frozen */
						if(CAN1Buffer[pointer].frozenTime == 0)
						{
							CAN1Buffer[pointer].prevCanID = prevID;
							CAN1Buffer[pointer].frozenTriesCount = 0;
						}

					}
					else
					{
						if(CAN1Buffer[pointer].frozenTime == 0)
						{
							if(CAN1Buffer[pointer].frozenTriesCount == FREEZE_TRIES)
							{
								CAN1Buffer[pointer].frozenTime++;
								fprintf(log, " FFFF");
							}
						}
						else
						{
							CAN1Buffer[pointer].passedCount++;
						}

						CAN1Buffer[pointer].frozenTriesCount++;
					}

					if(CAN1Buffer[pointer].frozenTime > 0)
					{
						CAN1Buffer[pointer].frozenTime++;
					}

					fprintf(log, "\n");

					prevLogTimeDelta = timeDelta;

					prevID = ID;
				}



			}

	}

	fclose(bufferFile);

	fprintf(log, "\n\nSequencing done...\n\n");

}


void getCanTiming(char *filename, FILE *log)
{
	char inputStr[200];
	char canData[200];
	unsigned int pointer, pointer2;
	unsigned long timeNow_s ,timeNow_us, messagePeriod, logTimeDelta, prevLogTimeDelta, timeOrigin = 0;
	int ID, prevID, lastID, lastIDmem;
	printf("Reading CAN log...\r\n");

	/* open trace file */
	FILE *bufferFile = fopen(filename, "r");
	timeOrigin = 0;

	/* Read trace file one line at a time until end of file */
	while(fgets(inputStr, 190, bufferFile) != NULL)
	{
		/* Extract values from input string */
		unsigned int scanReturn = sscanf(inputStr, logFormat, &timeNow_s, &timeNow_us, &ID, &canData);

	//	if(inputStr[0] != 'NULL')
		{
			printf("%u Analysing log line: %s", scanReturn, inputStr);
		}

		timeNow_us += (timeNow_s * 1000000);


		if(scanReturn == 4)
		{
			/* Find pointer for buffer array (v11X logic) */
			pointer = GetCAN1BufferPointer(ID);

			/* Don't count if repeated or not in ID list */
			if(pointer != 9999)
			{
				pointer2 = CAN1Buffer[pointer].orderPointer;

				/* Find timeNow origin */
				if(timeOrigin == 0)
				{
					timeOrigin = timeNow_us;
				}

				/* Find current time delta from origin */
				timeDelta = (timeNow_us - timeOrigin);

				/* Find time delta since last message */
				logTimeDelta = timeDelta - prevLogTimeDelta;

				if((CAN1Buffer[pointer].frozenTime > 0) && ((CAN1Buffer[pointer].prevCanID == prevID) || (timeOrigin == 0)))
				{

					if(CAN1Buffer[pointer].canID == 0)
					{
						CAN1Buffer[pointer].canID = ID;
						lastID = ID;
						CAN1Buffer[pointer].prevLoggedTime = 0;
						messagePeriod = 0;
						CAN1Buffer[pointer].messageOffset = timeDelta;
					}
					else
					{
						/* Find time delta since last occurrence of current CAN ID */
						messagePeriod = (timeDelta - CAN1Buffer[pointer].prevLoggedTime);
					}


					/* Calculate max and min time deltas for current CAN ID (ignore large time deltas (MESSAGE_TIME_DELTA_MAX) ) */
					if((messagePeriod < MESSAGE_TIME_DELTA_MAX) && (messagePeriod > MESSAGE_TIME_DELTA_MIN))
					{

						if((CAN1Buffer[pointer].timeDeltaMin == 0) || (messagePeriod < CAN1Buffer[pointer].timeDeltaMin))
						{
							CAN1Buffer[pointer].timeDeltaMin = messagePeriod;
						}

						if((messagePeriod != timeDelta) && (messagePeriod >= CAN1Buffer[pointer].timeDeltaMax))
						{
							CAN1Buffer[pointer].timeDeltaMax = messagePeriod;
						}

						CAN1Buffer[pointer].deltaAvSum += messagePeriod;
						CAN1Buffer[pointer].deltaAvCounter++;
					}

					CAN1Buffer[pointer].prevLoggedTime = timeDelta;

				}

				/* file output */
				fprintf(log,"Time: %11lu  ID: 0x%03X  Instance: %7u  Previous Time: %11lu  Message period: %6lu  Time dif: %6lu Offset: %lu - %u ",
						timeDelta,
						ID,
						CAN1Buffer[pointer].logCounter,
						CAN1Buffer[pointer].prevLoggedTime,
						messagePeriod,
						logTimeDelta,
						CAN1Buffer[pointer].messageOffset,
						CAN1Buffer[pointer].timeDeltaMin);

				if(CAN1Buffer[pointer].prevCanID != prevID)
				{
					fprintf(log, "Expected after 0x%03X", OrderedMessages[pointer2].prevCanID);
				}

				fprintf(log, "\n");

				prevLogTimeDelta = timeDelta;
				prevID = ID;
			}



		}
	}

	fclose(bufferFile);

}

int orderMessages(void)
{
	unsigned int i, pointer, pointer2, minOffsetPrev = 0, counter;
	unsigned int minOffset = 999999;


	for (i = 0; i < BUFFERSIZE; i++)
	{
		pointer2 = 0;

		for(pointer = 1; pointer <= BUFFERSIZE; pointer++)
		{

			if(((CAN1Buffer[pointer].messageOffset < minOffset)
							&& (CAN1Buffer[pointer].messageOffset > minOffsetPrev))
					|| ((i == 0) && (CAN1Buffer[pointer].messageOffset == 0)))
			{
				if(CAN1Buffer[pointer].canID != 0)
				{
					minOffset = CAN1Buffer[pointer].messageOffset;

					OrderedMessages[i].canID = CAN1Buffer[pointer].canID;
					OrderedMessages[i].logCounter = CAN1Buffer[pointer].logCounter;
					OrderedMessages[i].timeDeltaMin = CAN1Buffer[pointer].timeDeltaMin;
					OrderedMessages[i].timeDeltaMax = CAN1Buffer[pointer].timeDeltaMax;
					OrderedMessages[i].prevLoggedTime = CAN1Buffer[pointer].prevLoggedTime;
					OrderedMessages[i].messageJitterMax = CAN1Buffer[pointer].messageJitterMax;
					OrderedMessages[i].prevCanID = CAN1Buffer[pointer].prevCanID;
					OrderedMessages[i].passedCount = CAN1Buffer[pointer].passedCount;
					OrderedMessages[i].frozenTime = CAN1Buffer[pointer].frozenTime;
					OrderedMessages[i].frozenTriesCount = CAN1Buffer[pointer].frozenTriesCount;
					OrderedMessages[i].deltaAvCounter = CAN1Buffer[pointer].deltaAvCounter;
					OrderedMessages[i].deltaAvSum = CAN1Buffer[pointer].deltaAvSum;
					OrderedMessages[i].deltaAv = CAN1Buffer[pointer].deltaAv;
					OrderedMessages[i].messageOffset = CAN1Buffer[pointer].messageOffset;
					OrderedMessages[i].messageOffsetDelta = CAN1Buffer[pointer].messageOffsetDelta;

					OrderedMessages[i].beforeTargetCount = CAN1Buffer[pointer].beforeTargetCount;
					OrderedMessages[i].afterTargetCount = CAN1Buffer[pointer].afterTargetCount;
					OrderedMessages[i].onTargetCount = CAN1Buffer[pointer].onTargetCount;

					pointer2 = GetCAN1BufferPointer(CAN1Buffer[pointer].canID);

				}
			}

		}

		if (pointer2 != 0)
		{
			printf("Buffer %u points to %u\n", pointer2, i);
			CAN1Buffer[pointer2].orderPointer = i;
		}

		OrderedMessages[i].messageOffsetDelta = (minOffset - minOffsetPrev);

		minOffsetPrev = minOffset;
		minOffset = 999999;

		if(OrderedMessages[i].canID != 0)
		{
			counter = i;
		}
	}

	return counter;
}

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
	}
}

int countSequence(void)
{
	unsigned int i, counter;

	printf("\n\n");

	for(i = 0; i < BUFFERSIZE; i++)
	{
		if(loggingSequence[i].canID != 0x000)
		{
			counter++;
			printf("0x%03X: %lu\n",loggingSequence[i].canID, loggingSequence[i].counter);
		}
	}

	return counter;
}


void calculateAverages(void)
{
	unsigned int pointer = 0;
	unsigned long deltaDiv = 0;

	for(pointer = 0; pointer < BUFFERSIZE; pointer++)
	{
		if(OrderedMessages[pointer].canID != 0)
		{
				deltaDiv = 0;

				/* Compensate for blocks of missed data */
				if((OrderedMessages[pointer].deltaAvCounter - deltaDiv)  > 0)
				{
					OrderedMessages[pointer].deltaAvCounter += deltaDiv;
					OrderedMessages[pointer].deltaAv = (OrderedMessages[pointer].deltaAvSum / OrderedMessages[pointer].deltaAvCounter);
				}
				else
				{
					OrderedMessages[pointer].deltaAv = 0;
				}


				OrderedMessages[pointer].messageJitterMax = (OrderedMessages[pointer].timeDeltaMax - OrderedMessages[pointer].timeDeltaMin);

				OrderedMessages[pointer].prevLoggedTime = 0;
		}
	}
}

void getPrecisionCanTiming(char *filename, FILE *log)
{
	char inputStr[200];
	char canData[200];
	unsigned int pointer, pointer2;
	unsigned long timeNow_s ,timeNow_us, messagePeriod, logTimeDelta, prevLogTimeDelta = 0, timeOrigin = 0;
	int ID, prevID, timeoutDelta;
	printf("Reading CAN log...\r\n");

	unsigned long targetBuffer_lower = 50;
	unsigned long targetBuffer_upper = 50;
	unsigned long processBuffer = 200;

	/* open trace file */
	FILE *bufferFile = fopen(filename, "r");

	timeOrigin = 0;

	filterPointer = 0;

	/* Read trace file one line at a time until end of file */
	while(fgets(inputStr, 190, bufferFile) != NULL)
	{
		/* Extract values from input string */
		unsigned int scanReturn = sscanf(inputStr, logFormat, &timeNow_s, &timeNow_us, &ID, &canData);

	//	if(inputStr[0] != NULL)
		{
//			printf("%u Further Analysing log line: %s", scanReturn, inputStr);
		}

		timeNow_us += (timeNow_s * 1000000);

		/* Find pointer for buffer array (v11X logic) */
		pointer = GetCAN1BufferPointer(ID);

		if(scanReturn == 4)
		{
			/* Don't count if repeated or not in ID list */
			if(pointer != 9999)
			{
				pointer2 = CAN1Buffer[pointer].orderPointer;

				/* Find timeNow origin */
				if(timeOrigin == 0)
				{
					timeOrigin = timeNow_us;
				}

				/* Find current time delta from origin */
				timeDelta = (timeNow_us - timeOrigin);

//				if(timeOrigin != 0)
				/* Find time delta since last message */
				{
					logTimeDelta = (timeDelta - prevLogTimeDelta);

				}
//				else
				{
//					logTimeDelta = processBuffer;
//					printf("logTimeDelta3 = %lu\n", logTimeDelta);
				}


				/* file output */
//				fprintf(log,"Time: %11lu  ID: 0x%03X  Instance: %7u  ",
//									timeDelta,
//									ID,
//									CAN1Buffer[pointer].logCounter);

				if((logTimeDelta > (OrderedMessages[filterPointer].messageOffsetDelta + targetBuffer_upper)) || ((filterPointer != 0) && (logTimeDelta > 3000)))
				{
					printf("  Timed out\n");
					OrderedMessages[filterPointer].afterTargetCount++;
					changeFilter();
					prevLogTimeDelta = timeDelta;
				}
//



				if(pointer2 == filterPointer)		/* ID matches filter */
				{
					/* Find time delta since last occurrence of current CAN ID */
//					messagePeriod = (timeDelta - OrderedMessages[pointer2].prevLoggedTime);


					if((logTimeDelta >= processBuffer) || (timeOrigin == 0)) /* Message doesn't clash with processing of previous message */
					{
//						changeFilter();

//						if(messagePeriod < (OrderedMessages[pointer2].deltaAv - targetBuffer_lower))
//						{
//							OrderedMessages[pointer2].beforeTargetCount++;
//							fprintf(log, " << ");
////							prevLogTimeDelta = timeDelta;
//							printf("Early   ");
////							changeFilter();
//
//						}
//						else if(messagePeriod > (OrderedMessages[pointer2].deltaAv + targetBuffer_upper))
//						{
//							OrderedMessages[pointer2].afterTargetCount++;
//							printf("Late   ");
//							fprintf(log, " >> ");
//						}
//						else
//						{
//							OrderedMessages[pointer2].onTargetCount++;
//							fprintf(log, " ^^ ");
////							prevLogTimeDelta = timeDelta;
//							printf("On Time   ");
////							changeFilter();
//						}
						OrderedMessages[pointer2].onTargetCount++;

						printf("Found");

					}
					else
					{
						OrderedMessages[pointer2].clashCount++;
						printf("Clashed");
					}

					changeFilter();


					OrderedMessages[pointer2].prevLoggedTime = timeDelta;
					prevLogTimeDelta = timeDelta;

				}
				else
				{
					printf(" - 0x%03X ", OrderedMessages[pointer2].canID);
					printf("  logTimeDelta = %lu\n", logTimeDelta);

//					if((logTimeDelta >= (OrderedMessages[filterPointer].messageOffsetDelta + targetBuffer_upper)))
//					{
//						printf("  Timed out.\n");
//						OrderedMessages[filterPointer].afterTargetCount++;
//						changeFilter();
//						prevLogTimeDelta = timeDelta;
//					}

				}


				fprintf(log, "\n");

				prevID = ID;

			}

		}
	}

	fclose(bufferFile);

}

void changeFilter(void)
{
	if(filterPointer < noIDs)
	{
		filterPointer++;
	}
	else
	{
		filterPointer = 0;
	}
	printf("\nlooking for 0x%03X", OrderedMessages[filterPointer].canID);

}

void summaryOutput(FILE *output)
{
	char outputStr[1000];
	unsigned int pointer = 0;
	unsigned long deltaDiv = 0;
	double percentCaptured;
	int percentCapturedInt;
	sprintf(outputStr, 						"\n\n\n\n| CAN ID |  Total | Times      | Times    | Times on Target | Times before Target | Times Clashed |  Captured | Times after Target | Max. Jitter | Av. Period | Max. Period | Min. Period |  Offset [diff] |\n");
	sprintf(outputStr + strlen(outputStr), 	        "|        | (logs) | frozen for | caught   |          (logs) |              (logs) |        (logs) | (percent) |             (logs) |        (us) |       (us) |        (us) |        (us) |           (us) |\n");
	sprintf(outputStr + strlen(outputStr), 	        "|========|========|============|==========|=================|=====================|===============|===========|====================|=============|============|=============|=============|================|\n");

	printf("%u\n", strlen(outputStr));

	fprintf(output,outputStr);
	printf(outputStr);

	percentCapturedInt = 0;

	for(pointer = 0; pointer < BUFFERSIZE; pointer++)
	{
		if(OrderedMessages[pointer].canID != 0)
		{

			percentCaptured = (((double)(OrderedMessages[pointer].onTargetCount) + (double)(OrderedMessages[pointer].beforeTargetCount)) / (double)(OrderedMessages[pointer].logCounter)) * 100;
			percentCapturedInt = (int)(percentCaptured * 100) / 100;

			sprintf(outputStr, "|  0x%03X | %6u | %10lu | %8u | %15u | %19u | %13u | %4s%03.03f | %18u | %11lu | %10lu | %11u | %11u | %5lu [+%5lu] | %lu %u\n",
					OrderedMessages[pointer].canID,
					OrderedMessages[pointer].logCounter,
					OrderedMessages[pointer].frozenTime,
					(OrderedMessages[pointer].onTargetCount + OrderedMessages[pointer].beforeTargetCount),
					OrderedMessages[pointer].onTargetCount,
					OrderedMessages[pointer].beforeTargetCount,
					OrderedMessages[pointer].clashCount,
					"",
					percentCaptured,
					OrderedMessages[pointer].afterTargetCount,
					OrderedMessages[pointer].messageJitterMax,
					OrderedMessages[pointer].deltaAv,
					OrderedMessages[pointer].timeDeltaMax,
					OrderedMessages[pointer].timeDeltaMin,
					OrderedMessages[pointer].messageOffset,
					OrderedMessages[pointer].messageOffsetDelta,
					deltaDiv,
					OrderedMessages[pointer].deltaAvCounter);

			fprintf(output, outputStr);
			printf(outputStr);

		}
	}
	unsigned int minutes, seconds, decimals;

	decimals = timeDelta % 1000000;
	seconds = ((timeDelta / 1000000) % 60);
	minutes = ((timeDelta / 1000000) / 60);

	printf("\n%u IDs. Total logging time: %u min, %u.%u sec\n", noIDs, minutes, seconds, decimals);
	fprintf(output, "\nTotal logging time: %u min, %u.%u sec\n", minutes, seconds, decimals);

}

void csvOutput(char *filename)
{
	char csvStr[400];
	unsigned int pointer = 0;

	/* open trace file */
	FILE *csv = fopen(filename, "w");

	if(csv == NULL)
	{
		printf("\nUnable to access csv file\n");
	}
	else
	{
		printf("\nWriting to csv file...\n");

		sprintf(csvStr, 						"\n\n\n\n,,CAN ID,Total,Times,Times in,Times on Target,Times before Target,Times after Target,Max. Jitter,Av. Period,Max. Period,Min. Period,Offset,Dif\n");
		sprintf(csvStr + strlen(csvStr), 	        ",,,(logs),frozen for,sequence,(logs),(logs),(logs),(us),(us),(us),(us),(us),(us)\n");

		fprintf(csv,csvStr);
		for(pointer = 0; pointer < BUFFERSIZE; pointer++)
		{
			if(OrderedMessages[pointer].canID != 0)
			{

				OrderedMessages[pointer].messageJitterMax = (OrderedMessages[pointer].timeDeltaMax - OrderedMessages[pointer].timeDeltaMin);

				sprintf(csvStr, ",,0x%03X,%u,%lu,%u,%u,%u,%u,%lu,%lu,%u,%u,%lu,%lu\n",
						OrderedMessages[pointer].canID,
						OrderedMessages[pointer].logCounter,
						OrderedMessages[pointer].frozenTime,
						(OrderedMessages[pointer].onTargetCount + OrderedMessages[pointer].beforeTargetCount),
						OrderedMessages[pointer].onTargetCount,
						OrderedMessages[pointer].beforeTargetCount,
						OrderedMessages[pointer].afterTargetCount,
						OrderedMessages[pointer].messageJitterMax,
						OrderedMessages[pointer].deltaAv,
						OrderedMessages[pointer].timeDeltaMax,
						OrderedMessages[pointer].timeDeltaMin,
						OrderedMessages[pointer].messageOffset,
						OrderedMessages[pointer].messageOffsetDelta);

				fprintf(csv, csvStr);
			}
		}

		fclose(csv);
	}
}














int GetCAN1BufferPointer(unsigned int ID)
{
	switch(ID)
	{
		case(0x0C6):	return 1;		break;
		case(0x0CC):	return 2;		break;
		case(0x0D2):	return 3;		break;
		case(0x0C0):	return 4;		break;
		case(0x0C1):	return 5;		break;
		case(0x0C7):	return 6;		break;
		case(0x0CD):	return 7;		break;
		case(0x0D3):	return 8;		break;
		case(0x0C2):	return 9;		break;
		case(0x0C8):	return 10;		break;
		case(0x0CE):	return 11;		break;
		case(0x0D4):	return 12;		break;
		case(0x0C4):	return 13;		break;
		case(0x0CA):	return 14;		break;
		case(0x0D0):	return 15;		break;
		case(0x0D6):	return 16;		break;
		case(0x440):	return 17;		break;
		case(0x442):	return 18;		break;
		case(0x444):	return 19;		break;
		case(0x446):	return 20;		break;
		case(0x618):	return 21;		break;
		case(0x608):	return 22;		break;
		case(0x5F8):	return 23;		break;
		case(0x5E8):	return 24;		break;
		case(0x610):	return 25;		break;
		case(0x600):	return 26;		break;
		case(0x5F0):	return 27;		break;
		case(0x5E0):	return 28;		break;
		case(0x180):	return 29;		break;
		case(0x181):	return 30;		break;
		case(0x182):	return 31;		break;
		case(0x183):	return 32;		break;
		case(0x350):	return 33;		break;
		case(0x351):	return 34;		break;
		case(0x352):	return 35;		break;
		case(0x353):	return 36;		break;
		case(0x354):	return 37;		break;
		case(0x355):	return 38;		break;
		case(0x356):	return 39;		break;
		case(0x357):	return 40;		break;
		case(0x358):	return 41;		break;
		case(0x359):	return 42;		break;
		case(0x35A):	return 43;		break;
		case(0x35B):	return 44;		break;
		case(0x35C):	return 45;		break;
		case(0x35D):	return 46;		break;
		case(0x35E):	return 47;		break;
		case(0x35F):	return 48;		break;
		case(0x360):	return 49;		break;
		case(0x361):	return 50;		break;
		case(0x362):	return 51;		break;
		case(0x363):	return 52;		break;
		case(0x364):	return 53;		break;
		case(0x365):	return 54;		break;
		case(0x366):	return 55;		break;
		case(0x367):	return 56;		break;
		case(0x368):	return 57;		break;
		case(0x369):	return 58;		break;
		case(0x36A):	return 59;		break;
		case(0x36B):	return 60;		break;
		case(0x36C):	return 61;		break;
		case(0x36D):	return 62;		break;
		case(0x36E):	return 63;		break;
		case(0x36F):	return 64;		break;
		case(0x370):	return 65;		break;
		case(0x371):	return 66;		break;
		case(0x372):	return 67;		break;
		case(0x373):	return 68;		break;
		case(0x374):	return 69;		break;
		case(0x375):	return 70;		break;
		case(0x376):	return 71;		break;
		case(0x377):	return 72;		break;
		case(0x378):	return 73;		break;
		case(0x379):	return 74;		break;
		case(0x37A):	return 75;		break;
		case(0x37B):	return 76;		break;
		case(0x37C):	return 77;		break;
		case(0x37D):	return 78;		break;
		case(0x380):	return 79;		break;
		case(0x381):	return 80;		break;
		case(0x382):	return 81;		break;
		case(0x383):	return 82;		break;
		case(0x384):	return 83;		break;
		case(0x385):	return 84;		break;
		case(0x386):	return 85;		break;
		case(0x387):	return 86;		break;
		case(0x388):	return 87;		break;
		case(0x389):	return 88;		break;
		case(0x38A):	return 89;		break;
		case(0x38B):	return 90;		break;
		case(0x38C):	return 91;		break;
		case(0x38D):	return 92;		break;
		case(0x38E):	return 93;		break;
		case(0x38F):	return 94;		break;
		case(0x390):	return 95;		break;
		case(0x391):	return 96;		break;
		case(0x392):	return 97;		break;
		case(0x393):	return 98;		break;
		case(0x394):	return 99;		break;
		case(0x395):	return 100;		break;
		case(0x396):	return 101;		break;
		case(0x397):	return 102;		break;
		case(0x398):	return 103;		break;
		case(0x399):	return 104;		break;
		case(0x39A):	return 105;		break;
		case(0x39B):	return 106;		break;
		case(0x39C):	return 107;		break;
		case(0x39D):	return 108;		break;
		case(0x39E):	return 109;		break;
		case(0x39F):	return 110;		break;
		case(0x3A0):	return 111;		break;
		case(0x3A1):	return 112;		break;
		case(0x3A2):	return 113;		break;
		case(0x3A3):	return 114;		break;
		case(0x3A4):	return 115;		break;
		case(0x3A5):	return 116;		break;
		case(0x3A6):	return 117;		break;
		case(0x3A7):	return 118;		break;
		case(0x3A8):	return 119;		break;
		case(0x3A9):	return 120;		break;
		case(0x3AA):	return 121;		break;
		case(0x3AB):	return 122;		break;
		case(0x3AC):	return 123;		break;
		case(0x3AD):	return 124;		break;
		case(0x3B0):	return 125;		break;
		case(0x3B1):	return 126;		break;
		case(0x3B2):	return 127;		break;
		case(0x3B3):	return 128;		break;
		case(0x3B4):	return 129;		break;
		case(0x3B5):	return 130;		break;
		case(0x3B6):	return 131;		break;
		case(0x3B7):	return 132;		break;
		case(0x3B8):	return 133;		break;
		case(0x3B9):	return 134;		break;
		case(0x3BA):	return 135;		break;
		case(0x3BB):	return 136;		break;
		case(0x3BC):	return 137;		break;
		case(0x3BD):	return 138;		break;
		case(0x3BE):	return 139;		break;
		case(0x3BF):	return 140;		break;
		case(0x3C0):	return 141;		break;
		case(0x3C1):	return 142;		break;
		case(0x3C2):	return 143;		break;
		case(0x3C3):	return 144;		break;
		case(0x3C4):	return 145;		break;
		case(0x3C5):	return 146;		break;
		case(0x3C6):	return 147;		break;
		case(0x3C7):	return 148;		break;
		case(0x3C8):	return 149;		break;
		case(0x3C9):	return 150;		break;
		case(0x3CA):	return 151;		break;
		case(0x3CB):	return 152;		break;
		case(0x3CC):	return 153;		break;
		case(0x3CD):	return 154;		break;
		case(0x3CE):	return 155;		break;
		case(0x3CF):	return 156;		break;
		case(0x3D0):	return 157;		break;
		case(0x3D1):	return 158;		break;
		case(0x3D2):	return 159;		break;
		case(0x3D3):	return 160;		break;
		case(0x3D4):	return 161;		break;
		case(0x3D5):	return 162;		break;
		case(0x3D6):	return 163;		break;
		case(0x3D7):	return 164;		break;
		case(0x3D8):	return 165;		break;
		case(0x3D9):	return 166;		break;
		case(0x3DA):	return 167;		break;
		case(0x3DB):	return 168;		break;
		case(0x3DC):	return 169;		break;
		case(0x3DD):	return 170;		break;
		case(0x3E0):	return 171;		break;
		case(0x3E1):	return 172;		break;
		case(0x3E2):	return 173;		break;
		case(0x3E3):	return 174;		break;
		case(0x3E4):	return 175;		break;
		case(0x3E5):	return 176;		break;
		case(0x3E6):	return 177;		break;
		case(0x3E7):	return 178;		break;
		case(0x3E8):	return 179;		break;
		case(0x3E9):	return 180;		break;
		case(0x3EA):	return 181;		break;
		case(0x3EB):	return 182;		break;
		case(0x3EC):	return 183;		break;
		case(0x3ED):	return 184;		break;
		case(0x3EE):	return 185;		break;
		case(0x3EF):	return 186;		break;
		case(0x3F0):	return 187;		break;
		case(0x3F1):	return 188;		break;
		case(0x3F2):	return 189;		break;
		case(0x3F3):	return 190;		break;
		case(0x3F4):	return 191;		break;
		case(0x3F5):	return 192;		break;
		case(0x3F6):	return 193;		break;
		case(0x3F7):	return 194;		break;
		case(0x3F8):	return 195;		break;
		case(0x3F9):	return 196;		break;
		case(0x3FA):	return 197;		break;
		case(0x3FB):	return 198;		break;
		case(0x3FC):	return 199;		break;
		case(0x3FD):	return 200;		break;
		case(0x3FE):	return 201;		break;
		case(0x3FF):	return 202;		break;
		case(0x400):	return 203;		break;
		case(0x401):	return 204;		break;
		case(0x402):	return 205;		break;
		case(0x403):	return 206;		break;
		case(0x404):	return 207;		break;
		case(0x405):	return 208;		break;
		case(0x406):	return 209;		break;
		case(0x407):	return 210;		break;
		case(0x408):	return 211;		break;
		case(0x409):	return 212;		break;
		case(0x40A):	return 213;		break;
		case(0x40B):	return 214;		break;
		case(0x40C):	return 215;		break;
		case(0x40D):	return 216;		break;
		case(0x46A):	return 217;		break;
		case(0x46B):	return 218;		break;
		case(0x46C):	return 219;		break;
		case(0x46D):	return 220;		break;
		case(0x46E):	return 221;		break;
		case(0x46F):	return 222;		break;
		case(0x470):	return 223;		break;
		case(0x471):	return 224;		break;
		case(0x472):	return 225;		break;
		case(0x473):	return 226;		break;
		case(0x474):	return 227;		break;
		case(0x475):	return 228;		break;
		case(0x476):	return 229;		break;
		case(0x477):	return 230;		break;
		case(0x478):	return 231;		break;
		case(0x479):	return 232;		break;
		case(0x47A):	return 233;		break;
		case(0x47B):	return 234;		break;
		case(0x47C):	return 235;		break;
		case(0x47D):	return 236;		break;
		case(0x47E):	return 237;		break;
		case(0x47F):	return 238;		break;
		case(0x480):	return 239;		break;
		case(0x481):	return 240;		break;
		case(0x56A):	return 241;		break;
		case(0x56B):	return 242;		break;
		case(0x66A):	return 243;		break;
		case(0x66C):	return 244;		break;
		case(0x66E):	return 245;		break;
		case(0x670):	return 246;		break;
		case(0x66B):	return 247;		break;
		case(0x66D):	return 248;		break;
		case(0x66F):	return 249;		break;
		case(0x671):	return 250;		break;
		case(0x76A):	return 251;		break;
		case(0x76B):	return 252;		break;
		case(0x76C):	return 253;		break;
		case(0x76D):	return 254;		break;
		case(0x76E):	return 255;		break;
		case(0x76F):	return 256;		break;
		case(0x770):	return 257;		break;
		case(0x771):	return 258;		break;
		case(0x772):	return 259;		break;
		case(0x773):	return 260;		break;
		case(0x774):	return 261;		break;
		case(0x775):	return 262;		break;
		case(0x776):	return 263;		break;
		case(0x777):	return 264;		break;
		case(0x778):	return 265;		break;
		case(0x779):	return 266;		break;
		case(0x77A):	return 267;		break;
		case(0x77B):	return 268;		break;
		case(0x77C):	return 269;		break;
		case(0x77D):	return 270;		break;
		case(0x77E):	return 271;		break;
		case(0x77F):	return 272;		break;
		case(0x780):	return 273;		break;
		case(0x781):	return 274;		break;
		case(0x06A):	return 275;		break;
		case(0x06B):	return 276;		break;
		case(0x06C):	return 277;		break;
		case(0x06D):	return 278;		break;
		case(0x06E):	return 279;		break;
		case(0x06F):	return 280;		break;
		case(0x070):	return 281;		break;
		case(0x071):	return 282;		break;
		case(0x072):	return 283;		break;
		case(0x073):	return 284;		break;
		case(0x074):	return 285;		break;
		case(0x075):	return 286;		break;
		case(0x16A):	return 287;		break;
		case(0x16B):	return 288;		break;
		case(0x26A):	return 289;		break;
		case(0x26B):	return 290;		break;
		case(0x26C):	return 291;		break;
		case(0x26D):	return 292;		break;
		case(0x26E):	return 293;		break;
		case(0x26F):	return 294;		break;
		case(0x272):	return 295;		break;
		case(0x271):	return 296;		break;
		case(0x273):	return 297;		break;
		case(0x274):	return 298;		break;
		case(0x275):	return 299;		break;
		case(0x67A):	return 300;		break;
		case(0x67B):	return 301;		break;
		case(0x67C):	return 302;		break;
		case(0x67D):	return 303;		break;
		case(0x67E):	return 304;		break;
		case(0x67F):	return 305;		break;
		case(0x680):	return 306;		break;
		case(0x681):	return 307;		break;
		case(0x682):	return 308;		break;
		case(0x683):	return 309;		break;
		case(0x684):	return 310;		break;
		case(0x685):	return 311;		break;
		case(0x686):	return 312;		break;
		case(0x687):	return 313;		break;
		case(0x688):	return 314;		break;
		case(0x689):	return 315;		break;
		case(0x68A):	return 316;		break;
		case(0x68B):	return 317;		break;
		case(0x68C):	return 318;		break;
		case(0x68D):	return 319;		break;
		case(0x68E):	return 320;		break;
		case(0x68F):	return 321;		break;
		case(0x690):	return 322;		break;
		case(0x691):	return 323;		break;
		case(0x5F1):	return 324;		break;
		case(0x5F2):	return 325;		break;
		case(0x5F3):	return 326;		break;
		case(0x5F4):	return 327;		break;
		case(0x5F5):	return 328;		break;
		case(0x601):	return 329;		break;
		case(0x602):	return 330;		break;
		case(0x603):	return 331;		break;
		case(0x604):	return 332;		break;
		case(0x605):	return 333;		break;
		case(0x611):	return 334;		break;
		case(0x612):	return 335;		break;
		case(0x613):	return 336;		break;
		case(0x614):	return 337;		break;
		case(0x615):	return 338;		break;
		//The below were added for Smiths Battery
		case(0x707):	return 339;		break;
		case(0x188):	return 340;		break;
		case(0x307):	return 341;		break;
		case(0x287):	return 342;		break;
		case(0x207):	return 343;		break;
		case(0x187):	return 344;		break;
		case(0x709):	return 345;		break;
		case(0x18A):	return 346;		break;
		case(0x309):	return 347;		break;
		case(0x289):	return 348;		break;
		case(0x209):	return 349;		break;
		case(0x189):	return 350;		break;
		case(0x70B):	return 351;		break;
		case(0x18C):	return 352;		break;
		case(0x30B):	return 353;		break;
		case(0x28B):	return 354;		break;
		case(0x20B):	return 355;		break;
		case(0x18B):	return 356;		break;
		case(0x70D):	return 357;		break;
		case(0x18E):	return 358;		break;
		case(0x30D):	return 359;		break;
		case(0x28D):	return 360;		break;
		case(0x20D):	return 361;		break;
		case(0x18D):	return 362;		break;
		default:		return -1;	break;
	}

	return -1;
}
