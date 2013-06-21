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
#define FILTERSIZE 				(200)
#define MAX_TRACE_LINES			(100000)  /* Set to zero to analyse entire trace */
#define LOGGING_TASK_PERIOD_us 	(1000)


#define FREEZE_TRIES 	(0)
#define MESSAGE_TIME_DELTA_MAX (100500)
#define MESSAGE_TIME_DELTA_MIN (0)

unsigned long  ID, timeDelta;
unsigned long  Task_WCET;
int noIDs;
unsigned int filterPointer;

typedef struct{
	int canID;
	unsigned int cycleTime;
} logging_list_t;

typedef struct
{
	int canID;
	unsigned long counter;
	unsigned long loggedCounter;
	int timer;
	int timer_reload;
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

logging_list_t loggingList[]={
	{ 0x187 , 10 },
	{ 0x188 , 10 },
	{ 0x189 , 10 },
	{ 0x18A , 10 },
	{ 0x18B , 10 },
	{ 0x18C , 10 },
	{ 0x18D , 10 },
	{ 0x18E , 10 },
	{ 0x207 , 10 },
	{ 0x209 , 10 },
	{ 0x20B , 10 },
	{ 0x20D , 10 },
	{ 0x287 , 10 },
	{ 0x289 , 10 },
	{ 0x28B , 10 },
	{ 0x28D , 10 },
	{ 0x307 , 10 },
	{ 0x309 , 10 },
	{ 0x30B , 10 },
	{ 0x30D , 10 },
	{ 0x385 , 10 },
	{ 0x387 , 10 },
	{ 0x389 , 10 },
	{ 0x38B , 10 },
	{ 0x38D , 10 },
	{ 0x407 , 10 },
	{ 0x409 , 10 },
	{ 0x40B , 10 },
	{ 0x40D , 10 },
	{ 0x707 , 50 },
	{ 0x709 , 50 },
	{ 0x70B , 50 },
	{ 0x70D , 50 }
};

int listSize = sizeof(loggingList)/sizeof(logging_list_t);
int sequenceSize;
void canTraceConverter(char *filename, FILE *output);
void CanSequenceMessageCounter(char *filename);
void checkLogability(char *filename, FILE *log, int filterSize, int sequenceSize);
void orderSequence(void);
int countSequence(void);
flag_t GetCAN1BufferPointer(unsigned int ID);

void buildSequence(void){
	int i, cycleTime_min;

	cycleTime_min = 0xFFFF;
	for(i=0;i<listSize;i++){
		if(loggingList[i].cycleTime<cycleTime_min){
			cycleTime_min = loggingList[i].cycleTime;
		}
	}

	for(i=0;i<BUFFERSIZE;i++){
		if(i<listSize){
			loggingSequence[i].canID = loggingList[i].canID;
			loggingSequence[i].timer_reload = loggingList[i].cycleTime/cycleTime_min;
			loggingSequence[i].timer = 1;
			printf("ID: %03X, Period: %u\n", loggingSequence[i].canID, loggingSequence[i].timer_reload);
		}
	}
}

flag_t updateFilter(unsigned int filterPointer){
	static int last_i = -1;
	int i, j;
	flag_t result = FALSE, IDfound = FALSE;

	i = last_i;
	do{
		if(i<(listSize-1)){
			i++;
		}
		else{
			i=0;
		}

		for(j = 0; j < FILTERSIZE; j++){
			if(acceptanceFilter[j].canID == loggingSequence[i].canID){
				IDfound = TRUE;
			}
			else{
				IDfound = FALSE;
			}
		}

		if(IDfound == FALSE){
			loggingSequence[i].timer--;
		}

		if(loggingSequence[i].timer<=0){
			result = TRUE;
		}

	}while((result == FALSE)&&(i != last_i));


	if(result == TRUE){
		last_i = i;

		loggingSequence[i].timer = loggingSequence[i].timer_reload;

		acceptanceFilter[filterPointer].canID = loggingSequence[i].canID;
		acceptanceFilter[filterPointer].sequencePointer = i;
		acceptanceFilter[filterPointer].loggedFlag = FALSE;
	}

	return result;
}


char *logFormat = "%4u.%06u 1  %3x             Tx%s";
char *detailedLogFormat = "%4u.%06u 1  %3x             Tx   d %1u %02X %02X %02X %02X %02X %02X %02X %02X";

int main(void){
	char *CANlogFile = "Logs/Log_for_analysis2.asc";
	int i;

	FILE *outputFile = fopen("newOutput.txt", "w");
//	canTraceConverter(CANlogFile, outputFile);


//
	FILE *logFile = fopen("CAN_Logging_new.txt", "w");


	noIDs = 0;
	buildSequence();
	CanSequenceMessageCounter(CANlogFile);
	orderSequence();

	sequenceSize = countSequence();
	printf("\n\n\n");
	printf("\n\n %u ID's\n\n",sequenceSize);

	printf("\n\n\nChecking logability...\r\n\n");
	fprintf(logFile,"\n\n,,Filter Size,Logged,Missed\n");
//	for(i = 1; i <= sequenceSize; i++)	{
//		checkLogability(CANlogFile, logFile, i, sequenceSize);
//	}

	checkLogability(CANlogFile, logFile, 16, sequenceSize);

	fclose(outputFile);
	fclose(logFile);

	return EXIT_SUCCESS;
}



void checkLogability(char *filename, FILE *log, int filterSize, int sequenceSize){
	char inputStr[200];
	char canData[200];
	int i = 0, IDLogCount = 0, IDMissedCount = 0;
	flag_t IDlogged = FALSE;
	unsigned long timeNow_s ,timeNow_us, timeOrigin, lineCounter=0;

	int ID;

	/* open trace file */
	FILE *bufferFile = fopen(filename, "r");


	for(i = 0; i < filterSize; i++)	{

		if(updateFilter(i)==TRUE){
			printf("filter ID: %03X\n", acceptanceFilter[i].canID);
		}
		else{
			printf("updateFilter FAIL\n");
		}
	}

	timeOrigin = 0;

	while((fgets(inputStr, 190, bufferFile) != NULL) && ((lineCounter < MAX_TRACE_LINES) || (MAX_TRACE_LINES == 0))){
		/* Extract values from input string */
		unsigned int scanReturn = sscanf(inputStr, logFormat, &timeNow_s, &timeNow_us, &ID, &canData);
		if(scanReturn == 4)	{ /* valid line in trace */
			lineCounter++;

			timeNow_us += (timeNow_s * 1000000);

			/* Find timeNow origin */
			if(timeOrigin == 0)	{
				timeOrigin = timeNow_us;
			}

			/* Find current time delta from origin */
			timeDelta = (timeNow_us - timeOrigin);

			printf("%u %lu\tChecking log line: %s", filterSize, timeDelta, inputStr);

			/* Logging task has run - replace logged IDs in filter */
			if(timeDelta >= LOGGING_TASK_PERIOD_us){

				for(i = 0; i < filterSize; i++){
					/* loggedFlag is set when ID is logged for first time */
					if(acceptanceFilter[i].loggedFlag == TRUE){
						if(updateFilter(i)==TRUE){
							printf("filter ID: %03X\n", acceptanceFilter[i].canID);
						}
						else{
							printf("updateFilter FAIL\n");
						}
					}
				}

				timeOrigin = timeNow_us;
			}

			/* ID is in logging list */
			if(GetCAN1BufferPointer(ID) == TRUE){
				IDlogged = FALSE;
				i = 0;

				/* look for ID in acceptance filter */
				do{
					if(acceptanceFilter[i].canID == ID){
						/* ID found, increment counters */
						IDLogCount++;
						loggingSequence[acceptanceFilter[i].sequencePointer].loggedCounter++;
						acceptanceFilter[i].loggedFlag = TRUE;

						IDlogged = TRUE;
					}

					i++;

				}while((IDlogged == FALSE) && (i < filterSize));

				/* ID not found in filter, so would be missed */
				if(IDlogged == FALSE){
					IDMissedCount++;
				}
			}
		}
	}

	printf("filterSize: %u   Logged: %u    Missed %u\n", filterSize, IDLogCount, IDMissedCount);

		for(i = 0; i < BUFFERSIZE; i++){
			if(loggingSequence[i].canID != 0){
				fprintf(log,",,0x%03X,%lu,%lu,%lu\n",loggingSequence[i].canID, loggingSequence[i].loggedCounter, (loggingSequence[i].counter - loggingSequence[i].loggedCounter), loggingSequence[i].counter);
			}
		}
		fprintf(log,"\n");

	for(i = 0; i < BUFFERSIZE; i++){
		if(loggingSequence[i].canID != 0){
			printf(",,0x%03X,%lu,%lu,%lu\n",loggingSequence[i].canID, loggingSequence[i].loggedCounter, (loggingSequence[i].counter - loggingSequence[i].loggedCounter), loggingSequence[i].counter);
		}
	}
	printf("\n");

//	fprintf(log,",,%u,%u,%u\n", filterSize, IDLogCount, IDMissedCount);

}

void canTraceConverter(char *filename, FILE *output){
	char inputStr[200];
	unsigned int canData[8];

	unsigned int i = 0, DLC, j;
	flag_t IDfound = FALSE;
	unsigned long timeNow_s ,timeNow_us, timeNow_ms, timeNow_ms_dec, lineCounter;

	unsigned int ID;
	printf("Reading CAN log...\r\n");

	/* open trace file */
	FILE *bufferFile = fopen(filename, "r");

	fprintf(output,";$FILEVERSION=1.3\n"																);
	fprintf(output,";$STARTTIME=41053.504196474\n"														);
	fprintf(output,";\n"																				);
	fprintf(output,";   C:\\Users\\SEV01\\Desktop\\BatteryCAN_151.trc\n"									);
	fprintf(output,";   Start time: 5/24/2012 12:06:02.575.3\n"											);
	fprintf(output,";-------------------------------------------------------------------------------\n"	);
	fprintf(output,";   Bus  Name  Connection    Protocol\n"											);
	fprintf(output,";   1    SEV  SEV@pcan_usb  CAN\n"													);
	fprintf(output,";-------------------------------------------------------------------------------\n"	);
	fprintf(output,";   Message Number\n"																);
	fprintf(output,";   |         Time Offset (ms)\n"													);
	fprintf(output,";   |         |       Bus\n"														);
	fprintf(output,";   |         |       |    Type\n"													);
	fprintf(output,";   |         |       |    |       ID (hex)\n"										);
	fprintf(output,";   |         |       |    |       |    Reserved\n"									);
	fprintf(output,";   |         |       |    |       |    |   Data Length Code\n"						);
	fprintf(output,";   |         |       |    |       |    |   |    Data Bytes (hex) ...\n"			);
	fprintf(output,";   |         |       |    |       |    |   |    |\n"								);
	fprintf(output,";   |         |       |    |       |    |   |    |\n"								);
	fprintf(output,";---+-- ------+------ +- --+-- ----+--- +- -+-- -+ -- -- -- -- -- -- --\n"			);


	i=1;
	while((fgets(inputStr, 190, bufferFile) != NULL) && ((lineCounter++ < MAX_TRACE_LINES) || (MAX_TRACE_LINES == 0))){
		/* Extract values from input string */
		unsigned int scanReturn = sscanf(inputStr, detailedLogFormat, &timeNow_s, &timeNow_us, &ID, &DLC, &canData[0], &canData[1], &canData[2], &canData[3], &canData[4], &canData[5], &canData[6], &canData[7]);
		if(scanReturn >= 4){
			timeNow_us += (timeNow_s * 1000000);
			timeNow_ms = timeNow_us/1000;
			timeNow_ms_dec = timeNow_us%1000;

			fprintf(output,"%6u)%10lu.%03lu 1  Rx        %04X -  %1u    ", i, timeNow_ms, timeNow_ms_dec, ID, DLC);

			for(j=0;j<DLC;j++){
				fprintf(output,"%02X ",canData[j]);
			}
			fprintf(output,"\n");

			printf("%u\n",i);
			i++;
		}
	}

	printf("Finished sequence.\n");

}

void CanSequenceMessageCounter(char *filename)
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

	while((fgets(inputStr, 190, bufferFile) != NULL) && ((lineCounter++ < MAX_TRACE_LINES) || (MAX_TRACE_LINES == 0)))
	{
		/* Extract values from input string */
		unsigned int scanReturn = sscanf(inputStr, logFormat, &timeNow_s, &timeNow_us, &ID, &canData);
		if(scanReturn == 4){
			printf("%u Counting ID's... Log line: %s", scanReturn, inputStr);

			if(GetCAN1BufferPointer(ID) == TRUE){
				i = 0;
				IDfound = FALSE;

				do{
					if(loggingSequence[i].canID == ID){
						loggingSequence[i].counter++;
					}

					if(loggingSequence[i].canID != 0){
						i++;
					}

				}while((loggingSequence[i].canID != 0) && (i < listSize) && (IDfound == FALSE));
			}
		}
	}

	printf("Finished sequence:\n");

	i = 0;

	while((loggingSequence[i].canID != 0) && (i < BUFFERSIZE))
	{
		printf("0x%03X: %u\n",loggingSequence[i].canID,loggingSequence[i].counter);
		i++;
	}
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
int countSequence(void){
	int i, counter = 0;

	printf("\n\n");

	for(i = 0; i < BUFFERSIZE; i++){
		if(loggingSequence[i].canID != 0x000){
			counter++;
//			printf("case(0x%03X)\n",loggingSequence[i].canID);
		}
	}

	return counter;
}



/* Checks if ID is in logging list */
flag_t GetCAN1BufferPointer(unsigned int ID){
	int i;
	flag_t IDfound = FALSE;

	for(i=0;i<listSize;i++){
		if(ID == loggingList[i].canID){
			IDfound = TRUE;
		}
	}
	return IDfound;
}
