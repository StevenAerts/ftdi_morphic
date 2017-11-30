/*
	Simple example to open a maximum of 4 devices - write some data then read it back.
	Shows one method of using list devices also.
	Assumes the devices have a loopback connector on them and they also have a serial number

	To build use the following gcc statement 
	(assuming you have the d2xx library in the /usr/local/lib directory).
	gcc -o simple main.c -L. -lftd2xx -Wl,-rpath /usr/local/lib
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/time.h>
#include "../ftc.h"

jmp_buf ftcException;
FT_STATUS ftcStatus;

#define BUF_SIZE 0x10
#define MAX_DEVICES		5

static int verbosity = 2;

static void verbosity_printf(int _verbosity, const char* format, ...)
{
	if(_verbosity <= verbosity) 
	{
		va_list argptr;
		va_start(argptr, format);
		vfprintf(stderr, format, argptr);
		va_end(argptr);
	}
}

static void verbosity_dumpBuffer(int _verbosity, const unsigned char *buffer, int elements, const char *title)
{
	int j;
	if(title==NULL) title="";
	verbosity_printf(_verbosity, "%s [", title);
	for (j = 0; j < elements; j++)
	{
		if (j > 0)
			verbosity_printf(_verbosity, ", ");
		verbosity_printf(_verbosity, "0x%02X", (unsigned int)buffer[j]);
	}
	verbosity_printf(_verbosity, "]\n");
}


static DWORD readBytes(FT_HANDLE ftHandle, unsigned char* pcBufRead, DWORD dwRxSize)
{
	
	DWORD dwReceived = FTC_Read(ftHandle, pcBufRead, dwRxSize);
	verbosity_dumpBuffer(2, pcBufRead, dwReceived, "FTC_Read");
	return dwReceived;
}

static DWORD readAll(FT_HANDLE ftHandle, unsigned char** ppcBufRead)
{
	DWORD dwRxSize = FTC_GetQueueStatus(ftHandle);
	if(dwRxSize == 0) return 0;
	
	*ppcBufRead = realloc(*ppcBufRead, (size_t)(dwRxSize));
	if(*ppcBufRead == NULL) 
		FTC_THROW(FTC_OTHER);
	
	return readBytes(ftHandle, *ppcBufRead, dwRxSize);
}

static void skipUntilLastToken(FT_HANDLE ftHandle, unsigned char token, int timeoutms)
{
	clock_t tStart = clock();
	unsigned char *pcBufRead=NULL;
	DWORD dwRxSize;
	
	for(;;) {
		dwRxSize = readAll(ftHandle, &pcBufRead);
		if(dwRxSize == 0) { 
			sleep(0);
		} else if(pcBufRead[dwRxSize-1] == token) {
			free(pcBufRead);
			return;
		}
		if(clock() - tStart > (timeoutms*1000)) {
			verbosity_printf(1, "Error: Timeout skipping to token 0x%02X\n", token);
			FTC_THROW(FTC_OTHER);
		}
	}
}

static void clearRxBuffer(FT_HANDLE ftHandle)
{
	verbosity_printf(2, "Clearing RX buffer\n");
	unsigned char *pcBufRead=NULL;
	readAll(ftHandle, &pcBufRead);
	if(pcBufRead != NULL) free(pcBufRead);
}

static void writeBytes(FT_HANDLE ftHandle, const unsigned char *data, DWORD dwBytesToWrite) 
{
	verbosity_dumpBuffer(2, data, dwBytesToWrite, "FTC_Write");
	FTC_Write(ftHandle, (LPVOID)data, dwBytesToWrite);
}
static void writeByte(FT_HANDLE ftHandle, unsigned char data) 
{
	writeBytes(ftHandle, &data, 1);
}

static void syncMpsse(FT_HANDLE ftHandle)
{
	clearRxBuffer(ftHandle);
	const unsigned char data1[] = {
		0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 
		0xAA, 0xAA, 0xAA, 0xAA, 0xAA
	};
	writeBytes(ftHandle, data1, sizeof(data1));
	skipUntilLastToken(ftHandle, 0xAA, 1000);
	const unsigned char data2[] = {
		0xAB
	};
	writeBytes(ftHandle, data2, sizeof(data2));
	skipUntilLastToken(ftHandle, 0xAB, 1000);
}

static int checkDone(FT_HANDLE ftHandle) 
{
	const unsigned char data[] = {
		0x81, 0x87 // Direction, send imm
	};
	writeBytes(ftHandle, data, sizeof(data));
	clock_t tStart = clock();
	DWORD dwRxSize;
	do {
		dwRxSize = FTC_GetQueueStatus(ftHandle);
		if(clock() - tStart > (1000*1000)) {
			verbosity_printf(1, "Error: Timeout \n");
			FTC_THROW(FTC_OTHER);
		}
	} while(dwRxSize == 0);
	unsigned char *pcBufRead=NULL;
	dwRxSize = readAll(ftHandle, &pcBufRead);
	unsigned char nstat = pcBufRead[0];
	return nstat & 0x10;
}
		

static void programStartConfig(FT_HANDLE ftHandle) 
{
	const unsigned char pulseNCONFIG[] = {
		0x80, 0x06, 0x87, // Data bits, value, direction
		0x80, 0x02, 0x87, // Data bits, value, direction
		0x80, 0x06, 0x87  // Data bits, value, direction
	};
	writeBytes(ftHandle, pulseNCONFIG, sizeof(pulseNCONFIG));
	checkDone(ftHandle);
}

static void program(FT_HANDLE ftHandle, const char *fname) 
{
	int i;
	// Open rbf file
	FILE *pFile = fopen(fname, "rb");
	if(pFile == NULL) FTC_THROW(FTC_OTHER);
	
	// Reset device
	FTC_ResetDevice(ftHandle);
	FTC_ResetDevice(ftHandle);
	
	// Set bit mode
	FTC_SetBitMode(ftHandle, 0, FT_BITMODE_MPSSE);
	
	syncMpsse(ftHandle);

	const unsigned char mpsseConfig[] = {
		0x86, 0x00, 0x00, // 6MHz clk
		0x80, 0X06, 0x87  // Data bits, value & direction 
	};
	writeBytes(ftHandle, mpsseConfig, sizeof(mpsseConfig));
	programStartConfig(ftHandle);
}


int main(int argc, char **argv)
{
	int ftcFunc = 0;
    //if ( (ftcFunc = setjmp(ftcException)) == 0 )// try{

	char *optProgramFile = NULL;
	int optNodata = 0;
	
	//unsigned char 	cBufWrite[BUF_SIZE];
	//unsigned char cBufRead[BUF_SIZE*2];
	char * 	pcBufLD[MAX_DEVICES + 1];
	char 	cBufLD[MAX_DEVICES][64];
	//DWORD	dwRxSize = 0;
	//DWORD 	dwBytesWritten, dwBytesRead=0;
	FT_STATUS	ftStatus;
	FT_HANDLE	ftHandle;//[MAX_DEVICES];
	int	iNumDevs = 0;
	int	i, j;
	int iSelectedProg = -1;
	int iSelectedMain = -1;
	//int	iDevicesOpen = 0;	
    //int queueChecks = 0;
    //long int timeout = 5; // seconds
    //struct timeval  startTime;
    //unsigned char bitmode=0;
    
	int c;
    //int digit_optind = 0;

	while (1) {
        //int this_option_optind = optind ? optind : 1;
        int option_index = 0;
        static struct option long_options[] = {
            {"program", required_argument, 0,  0 },
            {"nodata",  no_argument, 	   0,  0 },
//            {"append",  no_argument,       0,  0 },
//            {"delete",  required_argument, 0,  0 },
//            {"verbose", no_argument,       0,  0 },
//            {"create",  required_argument, 0, 'c'},
//            {"file",    required_argument, 0,  0 },
            {0,         0,                 0,  0 }
        };
		//c = getopt_long(argc, argv, ":abc:d:012", long_options, &option_index);
		c = getopt_long(argc, argv, ":v:", long_options, &option_index);
		if (c == -1) break;
		switch (c) {
        case 0:
			verbosity_printf(2, "option %s\n", long_options[option_index].name);
			switch(option_index) {
				case 0:
					if (optarg == NULL) {
						verbosity_printf(1, "Error: option %s requires a file name argument", long_options[option_index].name);
						return 1;
					} 
					optProgramFile = optarg;
					break;
				case 1:
					optNodata = 1;
					break;
				default:
					verbosity_printf(1, "Error: option %s not handled", long_options[option_index].name);
			}
            break;
		case 'v':
			verbosity=atoi(optarg);
			verbosity_printf(2, "option v with arg %s\n", optarg);
			break;
			
/*		case '0':
        case '1':
        case '2':
            if (digit_optind != 0 && digit_optind != this_option_optind)
              printf("digits occur in two different argv-elements.\n");
            digit_optind = this_option_optind;
            printf("option %c\n", c);
            break;

       case 'a':
            printf("option a\n");
            break;

       case 'b':
            printf("option b\n");
            break;

       case 'c':
            printf("option c with value '%s'\n", optarg);
            break;

       case 'd':
            printf("option d with value '%s'\n", optarg);
            break;
*/
       case '?':
            break;

       default:
            verbosity_printf(1, "Error: Invalid command line option\n", c);
            return 1;
        }
    }

	if (optind < argc) {
        printf("non-option ARGV-elements: ");
        while (optind < argc)
            printf("%s ", argv[optind++]);
        printf("\n");
    }
	
FTC_TRY(ftcFunc)
{
	for(i = 0; i < MAX_DEVICES; i++) {
		pcBufLD[i] = cBufLD[i];
	}
	pcBufLD[MAX_DEVICES] = NULL;

	// List devices
	ftStatus = FT_ListDevices(pcBufLD, &iNumDevs, FT_LIST_ALL | FT_OPEN_BY_SERIAL_NUMBER);
	if(ftStatus != FT_OK) {
		verbosity_printf(1, "Error: FT_ListDevices(%d)\n", (int)ftStatus);
		return 1;
	}
	if(iNumDevs < 1) {
		verbosity_printf(1, "Error: No device found\n");
		return 1;
	}

	// Select device
	for(i = 0; ( (i <MAX_DEVICES) && (i < iNumDevs) ); i++) {
		char * selectStr = "NOT selected";
		j = strlen(cBufLD[i]);
		if((iSelectedMain == -1) && (j > 2) && (cBufLD[i][j-1] == 'B')) {
			iSelectedMain = i;
			selectStr = "selected as main data port";
		} else if((iSelectedProg == -1) && (j > 2) && (cBufLD[i][j-1] == 'A')) {
			iSelectedProg = i;
			selectStr = "selected for programming (if needed)";
		}
		verbosity_printf(2, "Device %d Serial Number - %s %s\n", i, cBufLD[i], selectStr);
	}

	// Program device if requested
	if(optProgramFile != NULL) {
		if(iSelectedProg == -1) {
			verbosity_printf(1, "Error: No device selected for programming\n");
			return 1;
		}
		ftStatus = FT_OpenEx(cBufLD[iSelectedProg], FT_OPEN_BY_SERIAL_NUMBER, &ftHandle);
		if(ftStatus != FT_OK){
			verbosity_printf(1, "Error FT_OpenEx(%d), device %d\n", (int)ftStatus, iSelectedProg);
			verbosity_printf(2, "Use lsmod to check if ftdi_sio (and usbserial) are present.\n");
			verbosity_printf(2, "If so, unload them using rmmod, as they conflict with ftd2xx.\n");
			return 1;
		}
		verbosity_printf(2, "Opened device %s for programming\n", cBufLD[iSelectedProg]);
		program(ftHandle, optProgramFile);
		verbosity_printf(2, "Programming completed\n", cBufLD[iSelectedProg]);
	}
	
	if(optNodata)
		return 0;
	
	if(iSelectedMain == -1) {
		verbosity_printf(1, "Error: No device selected as main data port\n");
		return 1;
	}
	
	// Open device
	ftStatus = FT_OpenEx(cBufLD[iSelectedMain], FT_OPEN_BY_SERIAL_NUMBER, &ftHandle);
	if(ftStatus != FT_OK){
		verbosity_printf(1, "Error FT_OpenEx(%d), device %d\n", (int)ftStatus, iSelectedMain);
		verbosity_printf(2, "Use lsmod to check if ftdi_sio (and usbserial) are present.\n");
		verbosity_printf(2, "If so, unload them using rmmod, as they conflict with ftd2xx.\n");
		return 1;
	}
	verbosity_printf(2, "Opened device %s\n", cBufLD[iSelectedMain]);
	
	/*if(ftStatus = FT_GetBitMode(ftHandle, &bitmode) != FT_OK)
	{
		verbosity_printf(1, "FT_GetBitMode failed (error %d).\n", (int)ftStatus);
		goto closeDev;
	}
	verbosity_printf(2, "Bit mode: 0x%02X\n", bitmode);*/
	
	// Set bit mode
	//ftStatus = FT_SetBitMode(ftHandle, 0, FT_BITMODE_ASYNC_BITBANG);
	//ftStatus = FT_SetBitMode(ftHandle, 0, FT_BITMODE_SYNC_FIFO);
	ftStatus = FT_SetBitMode(ftHandle, 0, FT_BITMODE_MPSSE);
	if (ftStatus != FT_OK) 
	{
		verbosity_printf(1, "FT_SetBitMode failed (error %d).\n", (int)ftStatus);
		goto closeDev;
	}
	syncMpsse(ftHandle);
	/*
	// Clear read buffer if any
	ftStatus = FT_GetQueueStatus(ftHandle, &dwRxSize);
	if (ftStatus != FT_OK)
	{
		verbosity_printf(1, "FT_GetQueueStatus failed (%d).\n", (int)ftStatus);
		goto closeDev;
		
	}
	verbosity_printf(2, "Emptying read buffer (%d bytes)\n", dwRxSize);
	while(dwRxSize > 0)
	{
		ftStatus = FT_Read(ftHandle, cBufRead, MIN(dwRxSize, BUF_SIZE*2), &dwBytesRead);
		if (ftStatus != FT_OK) {
			verbosity_printf(1, "Error FT_Read(%d)\n", (int)ftStatus);
			goto closeDev;
		} 
		dwRxSize -= dwBytesRead;
	}
	
	// Fill write buffer
	for(j = 0; j < BUF_SIZE; j++) {
		cBufWrite[j] = 0xAA;
	}
	cBufWrite[BUF_SIZE-1] = 0xAB;
	
	// Write buffer
	verbosity_printf(2, "Calling FT_Write with this write-buffer:\n");
	if(verbosity >= 2) dumpBuffer(cBufWrite, BUF_SIZE);
	ftStatus = FT_Write(ftHandle, cBufWrite, BUF_SIZE, &dwBytesWritten);
	if (ftStatus != FT_OK) {
		verbosity_printf(1, "Error FT_Write(%d)\n", (int)ftStatus);
		goto closeDev;
	} else if (dwBytesWritten != (DWORD)BUF_SIZE) {
		verbosity_printf(1, "FT_Write only wrote %d (of %d) bytes\n", (int)dwBytesWritten, BUF_SIZE);
		goto closeDev;
	}
	
	// Read buffer
	dwRxSize = 0;
	while(dwRxSize < 2*BUF_SIZE) {
		ftStatus = FT_GetQueueStatus(ftHandle, &dwRxSize);
		if (ftStatus != FT_OK)
		{
			verbosity_printf(1, "FT_GetQueueStatus failed (%d).\n", (int)ftStatus);
			goto closeDev;
		}
		verbosity_printf(2, "FT_GetQueueStatus: %d\n", dwRxSize);
		sleep(1);
	}
	verbosity_printf(2, "Calling FT_Read with this read-buffer:\n");
	if(verbosity >= 2) dumpBuffer(cBufRead, dwRxSize);
	ftStatus = FT_Read(ftHandle, cBufRead, dwRxSize, &dwBytesRead);
	if (ftStatus != FT_OK) {
		verbosity_printf(1, "Error FT_Read(%d)\n", (int)ftStatus);
		goto closeDev;
	} else if (dwBytesRead != dwRxSize) {
		verbosity_printf(1, "FT_Read only read %d (of %d) bytes\n", (int)dwBytesRead, (int)dwRxSize);
		goto closeDev;
	}
	verbosity_printf(2, "FT_Read read %d bytes.  Read-buffer is now:\n", (int)dwBytesRead);
	if(verbosity >= 2) dumpBuffer(cBufRead, (int)dwBytesRead);
	*/

closeDev:
	FT_Close(ftHandle);
	verbosity_printf(2, "Closed device %s\n", cBufLD[iSelectedMain]);

} // } --> end of try{
CATCH
{
	switch(ftcFunc)
	{
	/*case FTC_OPEN:
		verbosity_printf(1, "Error %d in function FTC_OPEN\n", ftcStatus);
		break;*/
	default: 
		verbosity_printf(1, "Error %s(%d) in function %s(%d)\n", 
			FTC_STATUS_STR[ftcStatus], ftcStatus,
			FTC_FUNCTION_STR[ftcFunc], ftcFunc  
			);
	}
} // } --> end of catch(i){
	/*if(pcBufRead)
		free(pcBufRead);*/
	return 0;
}
