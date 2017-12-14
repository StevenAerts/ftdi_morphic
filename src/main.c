#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/time.h>
#include "ftc.h"

#define MAX_DEVICES 5

// defined extern in ftc.h
jmp_buf ftcException;
FT_STATUS ftcStatus;

// Application functions
enum {
	APP_READ_ALL = FTC_USER_START,
	APP_SKIP_UNTIL_LAST_TOKEN,
	APP_WAIT_FOR_DATA,
	APP_PROGRAM,
	APP_PARSE_OPTIONS, 
	APP_MAIN, 
	APP_TEST,
	APP_GET_PROGRAM_STATUS_BITS,
	APP_OTHER_FUNCTION
};
// Application error codes
enum {
	APP_TIMEOUT = FTC_USER_START,
	APP_INSUFFICIENT_RESOURCES,
	APP_FILE_OPEN_FAILED,
	APP_PROGRAM_BAD_STATUS,
	APP_PROGRAM_NOT_DONE,
	APP_OPTION_NOT_HANDLED,
	APP_OPTION_INVALID,
	APP_OPTION_PROGRAM_NO_FILENAME,
	APP_NO_DEVICE_FOUND,
	APP_NO_PROGRAMMING_DEVICE,
	APP_NO_DATA_DEVICE,
	APP_TEST_LOOPBACK_FAILED,
	APP_TEST_LOOPBACKINVERT_FAILED,
	APP_TEST_LOOPBACKINVERTN_FAILED,
	APP_TEST_UNEXPECTED_RESPONSE_DATA,
	APP_OTHER_ERROR
};
// Application function strings
const char* USER_FUNCTION_STR[] = {
	"APP_READ_ALL",
	"APP_SKIP_UNTIL_LAST_TOKEN",
	"APP_WAIT_FOR_DATA",
	"APP_PROGRAM",
	"APP_PARSE_OPTIONS", 
	"APP_MAIN", 
	"APP_TEST",
	"APP_GET_PROGRAM_STATUS_BITS",
	"APP_OTHER"
};
// Application error code strings
const char* USER_STATUS_STR[] = {
	"APP_TIMEOUT",
	"APP_INSUFFICIENT_RESOURCES",
	"APP_FILE_OPEN_FAILED",
	"APP_PROGRAM_BAD_STATUS",
	"APP_PROGRAM_NOT_DONE",
	"APP_OPTION_NOT_HANDLED",
	"APP_OPTION_INVALID",
	"APP_OPTION_PROGRAM_NO_FILENAME",
	"APP_NO_DEVICE_FOUND",
	"APP_NO_PROGRAMMING_DEVICE",
	"APP_NO_DATA_DEVICE",
	"APP_TEST_LOOPBACK_FAILED",
	"APP_TEST_LOOPBACKINVERT_FAILED",
	"APP_TEST_LOOPBACKINVERTN_FAILED",
	"APP_TEST_UNEXPECTED_RESPONSE_DATA"
	"APP_OTHER"
};

struct {
	char* programFileName;
	int noData;
	int noSync;
	int test;
	int verbosity;
} options = {
	.programFileName = NULL,
	.noData = 0,
	.noSync = 0,
	.test = 0,
	.verbosity = 2
};

// Print functions with verbosity - to stderr since stdout is used for streaming
static void verbosity_printf(int verbosity, const char* format, ...)
{
	if(verbosity <= options.verbosity) 
	{
		va_list argptr;
		va_start(argptr, format);
		vfprintf(stderr, format, argptr);
		va_end(argptr);
	}
}

static void verbosity_dumpBuffer(int verbosity, const unsigned char *buffer, int elements, const char *title)
{
	if(title==NULL) title="";
	verbosity_printf(verbosity, "%s [", title);
	for (int j = 0; j < elements; j++)
	{
		if (j > 0)
			verbosity_printf(verbosity, ", ");
		verbosity_printf(verbosity, "0x%02X", (unsigned int)buffer[j]);
	}
	verbosity_printf(verbosity, "]\n");
}

// Read functions

static DWORD readBytes(FT_HANDLE ftHandle, unsigned char* pcBufRead, DWORD dwRxSize)
{
	verbosity_printf(2, "Reading %d bytes\n", dwRxSize);
	DWORD dwReceived = FTC_Read(ftHandle, pcBufRead, dwRxSize);
	verbosity_dumpBuffer(3, pcBufRead, dwReceived, "FTC_Read");
	return dwReceived;
}

static DWORD readAll(FT_HANDLE ftHandle, unsigned char** ppcBufRead)
{
	DWORD dwRxSize = FTC_GetQueueStatus(ftHandle);
	if(dwRxSize == 0) return 0;
	
	*ppcBufRead = realloc(*ppcBufRead, (size_t)(dwRxSize));
	if(*ppcBufRead == NULL)
		FTC_THROWE(APP_READ_ALL, APP_INSUFFICIENT_RESOURCES);
	
	return readBytes(ftHandle, *ppcBufRead, dwRxSize);
}

static int clearRxBuffer(FT_HANDLE ftHandle)
{
	verbosity_printf(2, "Clearing RX buffer\n");
	unsigned char *pcBufRead=NULL;
	int n = readAll(ftHandle, &pcBufRead);
	if(pcBufRead != NULL) free(pcBufRead);
	return n;
}

static void waitForData(FT_HANDLE ftHandle, int timeoutMs) 
{
	verbosity_printf(2, "Waiting for data ");
	clock_t tStart = clock();
	DWORD dwRxSize;
	do {
		dwRxSize = FTC_GetQueueStatus(ftHandle);
		if(dwRxSize == 0) {
			verbosity_printf(2, ".");
			sleep(0);
			if(clock() - tStart > (timeoutMs*1000))
				FTC_THROWE(APP_WAIT_FOR_DATA, APP_TIMEOUT);
		}
	} while(dwRxSize == 0);
	verbosity_printf(2, "\n");
}

static void skipUntilLastToken(FT_HANDLE ftHandle, unsigned char token, int timeoutMs)
{
	verbosity_printf(2, "Skipping to token 0x%02X\n", token);
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
		if(clock() - tStart > (timeoutMs*1000))
			FTC_THROWE(APP_SKIP_UNTIL_LAST_TOKEN, APP_TIMEOUT);
	}
}

// Write functions

static void writeBytes(FT_HANDLE ftHandle, const unsigned char *data, DWORD dwBytesToWrite) 
{
	verbosity_printf(2, "Writing %d bytes\n", dwBytesToWrite);
	verbosity_dumpBuffer(3, data, dwBytesToWrite, "FTC_Write");
	FTC_Write(ftHandle, (LPVOID)data, dwBytesToWrite);
}
static void writeByte(FT_HANDLE ftHandle, unsigned char data) 
{
	writeBytes(ftHandle, &data, 1);
}

// MPSSE synchronization

static void syncMpsse(FT_HANDLE ftHandle)
{
	verbosity_printf(2, "Synchronizing MPSSE\n");
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

// Altera programming

#define PROGRAM_NSTATUS(v)  	(((v)>>3)&1)
#define PROGRAM_CONFDONE(v) 	(((v)>>4)&1)

static unsigned char getProgramStatusBits(FT_HANDLE ftHandle) 
{
	clearRxBuffer(ftHandle);
	const unsigned char data[] = {
		0x81, 0x87 // Direction, send imm
	};
	writeBytes(ftHandle, data, sizeof(data));
	waitForData(ftHandle, 1000);
	unsigned char *pcBufRead=NULL;
	readAll(ftHandle, &pcBufRead);
	verbosity_printf(2, "Program status bits NSTATUS=%d CONFDONE=%d\n",
		PROGRAM_NSTATUS(pcBufRead[0]), PROGRAM_CONFDONE(pcBufRead[0]));
	return pcBufRead[0];
}
		
static void program(FT_HANDLE ftHandle, const char *fname) 
{
	verbosity_printf(2, "Programming file %s\n", fname);
	// Open rbf file
	FILE *pFile = fopen(fname, "rb");
	if(pFile == NULL) 
		FTC_THROWE(APP_PROGRAM, APP_FILE_OPEN_FAILED);
	
	// Reset device
	FTC_ResetDevice(ftHandle);
	FTC_ResetDevice(ftHandle);
	
	// Set bit mode
	FTC_SetBitMode(ftHandle, 0, FT_BITMODE_MPSSE);
	syncMpsse(ftHandle);

	unsigned char programStatusBits;

	const unsigned char cfginit[] = {
		0x86, 0x00, 0x00,	// Mpsse config, 6MHz clk
		0x80, 0X06, 0x87,	// Data bits, value & direction 
		0x80, 0x02, 0x87,	// NCONFIG pulse LOW
		0x80, 0x06, 0x87	// NCONFIG HIGH
	};
	writeBytes(ftHandle, cfginit, sizeof(cfginit));

	programStatusBits = getProgramStatusBits(ftHandle);
	
	if(PROGRAM_CONFDONE(programStatusBits) == 0) for(;;) {
		unsigned char buffer[256+3];
		int iBytesRead = fread(buffer+3, 1, 256, pFile);
		if(iBytesRead > 0)
		{
			buffer[0] = 0x19;
			buffer[1] = iBytesRead-1;
			buffer[2] = 0;
			writeBytes(ftHandle, buffer, iBytesRead+3);
		}
		programStatusBits = getProgramStatusBits(ftHandle);
		if(iBytesRead == 0) break;
		if(PROGRAM_CONFDONE(programStatusBits) == 1) break;
		if(PROGRAM_NSTATUS(programStatusBits) == 0) break;
	}
	fclose(pFile);
	if(PROGRAM_NSTATUS(programStatusBits)==0)
		FTC_THROWE(APP_PROGRAM, APP_PROGRAM_BAD_STATUS);

	const unsigned char completeProg[] = {
		0x19, 0x01, 0x00,	// clk data out on -ve edge LSB, 2 bytes
		0x06, 0x06
	};
	writeBytes(ftHandle, completeProg, sizeof(completeProg));

	if(PROGRAM_CONFDONE(programStatusBits) == 1)
		verbosity_printf(2, "Programming passed\n");
	else
		FTC_THROWE(APP_PROGRAM, APP_PROGRAM_NOT_DONE);
}

static void checkProgrammed(FT_HANDLE ftHandle) 
{
	// Reset device
	FTC_ResetDevice(ftHandle);
	FTC_ResetDevice(ftHandle);
	
	// Set bit mode
	FTC_SetBitMode(ftHandle, 0, FT_BITMODE_MPSSE);
	syncMpsse(ftHandle);

	unsigned char programStatusBits;

	const unsigned char cfginit[] = {
		0x86, 0x00, 0x00,	// Mpsse config, 6MHz clk
		0x80, 0X06, 0x87,	// Data bits, value & direction 
	};
	writeBytes(ftHandle, cfginit, sizeof(cfginit));

	programStatusBits = getProgramStatusBits(ftHandle);
	
	if(PROGRAM_NSTATUS(programStatusBits)==0)
		FTC_THROWE(APP_PROGRAM, APP_PROGRAM_BAD_STATUS);

	/*if(PROGRAM_CONFDONE(programStatusBits) == 1)
		verbosity_printf(2, "Device was programmed\n");
	else
		FTC_THROWE(APP_PROGRAM, APP_PROGRAM_NOT_DONE);*/
}

static int testdiff(const unsigned char *bufferA, const unsigned char *bufferB, int invertfrom, int total)
{
	if(invertfrom < 0) invertfrom = total;
	for (int i = 0; i < invertfrom; i++)
		if(bufferA[i] != bufferB[i]) return 1; 
	for (int j = invertfrom; j < total; j++) {
		unsigned char b = ~bufferB[j];
		if(bufferA[j] != b) return 1; 
	}
	return 0;
}
static void test(FT_HANDLE ftHandle)
{
	unsigned char loopback4[] = {
		0xA8+4, 0, 1, 2, 3
	}, loopback4_result[sizeof(loopback4)];
	
	writeBytes(ftHandle, loopback4, sizeof(loopback4));
	readBytes(ftHandle, loopback4_result, sizeof(loopback4));
	
	if(testdiff(loopback4, loopback4_result, -1, sizeof(loopback4)))
		FTC_THROWE(APP_TEST, APP_TEST_LOOPBACK_FAILED);

	unsigned char loopbackInvert5[] = {
		0xB8+5, 0, 1, 2, 3, 4
	}, loopbackInvert5_result[sizeof(loopbackInvert5)];
	
	writeBytes(ftHandle, loopbackInvert5, sizeof(loopbackInvert5));
	readBytes(ftHandle, loopbackInvert5_result, sizeof(loopbackInvert5));
	
	if(testdiff(loopbackInvert5, loopbackInvert5_result,1,sizeof(loopbackInvert5)))
		FTC_THROWE(APP_TEST, APP_TEST_LOOPBACKINVERT_FAILED);

	unsigned char loopbackInvertN8[] = {
		0xB8+6, 8, 0, 1, 2, 3, 4, 5, 6, 7
	}, loopbackInvertN8_result[sizeof(loopbackInvertN8)];
	
	writeBytes(ftHandle, loopbackInvertN8, sizeof(loopbackInvertN8));
	readBytes(ftHandle, loopbackInvertN8_result, sizeof(loopbackInvertN8));
	
	if(testdiff(loopbackInvertN8, loopbackInvertN8_result,2,sizeof(loopbackInvertN8)))
		FTC_THROWE(APP_TEST, APP_TEST_LOOPBACKINVERTN_FAILED);

	if(clearRxBuffer(ftHandle) !=0)
		FTC_THROWE(APP_TEST, APP_TEST_UNEXPECTED_RESPONSE_DATA);
	
	unsigned char commands[] = {
		0x00, 
		0x10,
		0x21, 	0,
		0x42, 	0, 1, 
		0x83, 	0, 1, 2,
		0x34, 	0, 1, 2, 3,
		0x75, 	0, 1, 2, 3, 4,
		0x96,8, 0, 1, 2, 3, 4, 5, 6, 7
	};
	writeBytes(ftHandle, commands, sizeof(commands));
	
	if(clearRxBuffer(ftHandle) !=0)
		FTC_THROWE(APP_TEST, APP_TEST_UNEXPECTED_RESPONSE_DATA);

	writeBytes(ftHandle, loopback4, sizeof(loopback4));
	readBytes(ftHandle, loopback4_result, sizeof(loopback4));
	
	if(testdiff(loopback4, loopback4_result, -1, sizeof(loopback4)))
		FTC_THROWE(APP_TEST, APP_TEST_LOOPBACK_FAILED);
		
	unsigned char ledcmd3leds[] = {
		0x86, 3*3, 
		0x80, 0x00, 0x00,
		0x00, 0x80, 0x00,
		0x00, 0x00, 0x80
	};
	
	writeBytes(ftHandle, ledcmd3leds, sizeof(ledcmd3leds));
	
	if(clearRxBuffer(ftHandle) !=0)
		FTC_THROWE(APP_TEST, APP_TEST_UNEXPECTED_RESPONSE_DATA);

}


void parseOptions(int argc, char **argv)
{
	while (1) {
        int option_index = 0;
        static struct option long_options[] = {
            {"program", required_argument, 0,  0 },
            {"nodata",  no_argument, 	   0,  0 },
            {"test",    no_argument,	   0,  0 },
            {"nosync",  no_argument, 	   0,  0 },
            {0,         0,                 0,  0 }
        };
		int c = getopt_long(argc, argv, ":v:", long_options, &option_index);
		if (c == -1) break;
		switch (c) {
        case 0:
			switch(option_index) {
				case 0:
					if (optarg == NULL) 
						FTC_THROWE(APP_PARSE_OPTIONS, APP_OPTION_PROGRAM_NO_FILENAME);
					options.programFileName = optarg;
					break;
				case 1:
					options.noData = 1;
					break;
				case 2:
					options.test = 1;
					break;
				case 3:
					options.noSync = 1;
					break;
				default:
					FTC_THROWE(APP_PARSE_OPTIONS, APP_OPTION_NOT_HANDLED);
			}
            break;
		case 'v':
			options.verbosity=atoi(optarg);
			break;
       case '?':
            break;
       default:
			FTC_THROWE(APP_PARSE_OPTIONS, APP_OPTION_INVALID);
        }
    }

	/*if (optind < argc) {
        verbosity_printf(2, "non-option ARGV-elements: ");
        while (optind < argc) verbosity_printf(2, "%s ", argv[optind++]);
        verbosity_printf(2, "\n");
    }*/

}
// Main

int main(int argc, char **argv)
{
	int ftcFunc = 0;
	
	char * 	pcBufLD[MAX_DEVICES + 1];
	char 	cBufLD[MAX_DEVICES][64];
	FT_HANDLE	ftHandleProg, ftHandleMain;
	int	iNumDevs = 0;
	int	i, j;
	int iSelectedProg = -1;
	int iSelectedMain = -1;
	
FTC_TRY(ftcFunc)
{
	parseOptions(argc, argv);

	for(i = 0; i < MAX_DEVICES; i++) pcBufLD[i] = cBufLD[i];
	pcBufLD[MAX_DEVICES] = NULL;

	// Add custom VID/PID
	// FTC_SetVIDPID(0x0403, 0x9842);

	// List devices
	FTC_ListDevices(pcBufLD, &iNumDevs, FT_LIST_ALL | FT_OPEN_BY_SERIAL_NUMBER);
	if(iNumDevs < 1) FTC_THROWE(APP_MAIN, APP_NO_DEVICE_FOUND);

	// Select device
	for(i = 0; ( (i <MAX_DEVICES) && (i < iNumDevs) ); i++) {
		char * selectStr = "NOT selected";
		j = strlen(cBufLD[i]);
		if((iSelectedMain == -1) && (j > 2) && (cBufLD[i][j-1] == 'A')) {
			iSelectedMain = i;
			selectStr = "selected as main data port";
		} else if((iSelectedProg == -1) && (j > 2) && (cBufLD[i][j-1] == 'B')) {
			iSelectedProg = i;
			selectStr = "selected for programming (if needed)";
		}
		verbosity_printf(2, "Device %d Serial Number - %s %s\n", i, cBufLD[i], selectStr);
	}

	// Open devices
	if(iSelectedProg == -1) 
		FTC_THROWE(APP_MAIN, APP_NO_PROGRAMMING_DEVICE);

	ftHandleProg = FTC_OpenEx(cBufLD[iSelectedProg], FT_OPEN_BY_SERIAL_NUMBER);
	verbosity_printf(2, "Opened device %s for programming\n", cBufLD[iSelectedProg]);

	if(iSelectedMain == -1) 
		FTC_THROWE(APP_MAIN, APP_NO_DATA_DEVICE);

	ftHandleMain = FTC_OpenEx(cBufLD[iSelectedMain], FT_OPEN_BY_SERIAL_NUMBER);
	verbosity_printf(2, "Opened device %s for data\n", cBufLD[iSelectedMain]);

	FTC_ResetDevice(ftHandleProg);
	FTC_ResetDevice(ftHandleMain);
	FTC_SetBitMode(ftHandleMain, 0, FT_BITMODE_MPSSE);
	FTC_SetBitMode(ftHandleProg, 0, FT_BITMODE_MPSSE);
	
	if(options.programFileName != NULL) {
		program(ftHandleProg, options.programFileName);
	} else {
		checkProgrammed(ftHandleProg);		
	}
	
	if(options.noData)
		return 0;
	
	FTC_ResetDevice(ftHandleProg);
	FTC_ResetDevice(ftHandleMain);
	FTC_SetBitMode(ftHandleMain, 0, FT_BITMODE_SYNC_FIFO);
	FTC_SetFlowControl(ftHandleMain, FT_FLOW_RTS_CTS, 0, 0);

	if(!options.noSync) {
		unsigned char syncMain[] = {
			0xA8, 	0xA8,	0xA8,	0xA8, 	0xA8,
			0xA8, 	0xA8,	0xA8,	0xA8, 	0xA8,
			0xB8
		};
		writeBytes(ftHandleMain, syncMain, sizeof(syncMain));
		readBytes(ftHandleMain, syncMain, sizeof(syncMain));
	}
	if(options.test) 
		test(ftHandleMain);

	FTC_Close(ftHandleMain);
	verbosity_printf(2, "Closed device %s\n", cBufLD[iSelectedMain]);
	FTC_Close(ftHandleProg);
	verbosity_printf(2, "Closed device %s\n", cBufLD[iSelectedProg]);

} // } --> end of try{
CATCH
{
	const char *strFunction = ftcFunc < FTC_USER_START ?
		FTC_FUNCTION_STR[ftcFunc] : USER_FUNCTION_STR[ftcFunc-FTC_USER_START];
	const char *strStatus   = ftcStatus < FTC_USER_START ?
		FTC_STATUS_STR[ftcStatus] : USER_STATUS_STR[ftcStatus-FTC_USER_START];
	verbosity_printf(1, "Error %s(%xh) in function %s(%xh)\n", 
		strStatus, ftcStatus, strFunction, ftcFunc);
} // } --> end of catch(i){
	/*if(pcBufRead)
		free(pcBufRead);*/
	return 0;
}

/*
FT_STATUS Sync_to_MPSSE(FT_HANDLE);
FT_STATUS StartConfig  (FT_HANDLE);
FT_STATUS CheckDone    (FT_HANDLE);
FT_STATUS CheckStat    (FT_HANDLE);
void      CompleteProg (FT_HANDLE);
int programb22(FT_HANDLE ftHandle, char *fname);
#define BUF_SIZE 512
#define MAX_DEVICES	5


int programb22(FT_HANDLE ftHandle, char *fname)
{

	unsigned char 	cBufWrite[BUF_SIZE+3];
	//unsigned char  *pcBufRead = NULL;
	//unsigned char  *pcBufLD[MAX_DEVICES + 1];
	//unsigned char 	cBufLD[MAX_DEVICES][64];
	DWORD 	dwBytesWritten;
	FT_STATUS	ftStatus;
	//FT_HANDLE	ftHandle;
	int i;
	DWORD	iNumDevs = 0;
	int	i;
	
	FT_DEVICE_LIST_INFO_NODE *devInfo = NULL;

	//
	// Argument list processing
	//

	i = 0;
	int optV = 0;
	if (argc > 1 && argv[1][0] == '-') {
	  switch (argv[1][1]) {
	  case 'v': 
	    optV = 1; i++; break;
	  default:
	    if (strcmp(argv[1],"-h")) printf("Unknown option %s\n",argv[1]);
	    printf("Usage: %s [-v] [file.rbf] [interface]\n",argv[0]);
	    return 1;
	  }
	}
	  
	
	i++;
	char *RBFFileName="MariachiV2.rbf";
	if (argc > i) RBFFileName=argv[i];

	i++;
	char *DevName = "Morph-IC A";
	if (argc > i) DevName = argv[i];


	FT_SetVIDPID(0x0403, 0x9842);


	if (optV) { // Verbose
	//
	// if verbose create the device information list
	//
	  printf("File   requested: %s\n",RBFFileName);
	  printf("Device requested: %s\n\n",DevName);

	  ftStatus = FT_CreateDeviceInfoList(&iNumDevs);
	  if (ftStatus == FT_OK) {
	    printf("Number of devices is %d\n",iNumDevs);
	  }

	  //
	  // allocate storage for list based on numDevs
	  //

	  devInfo = (FT_DEVICE_LIST_INFO_NODE*)malloc(sizeof(FT_DEVICE_LIST_INFO_NODE)*iNumDevs);

	  //
	  // get the device information list
	  //
	  ftStatus = FT_GetDeviceInfoList(devInfo,&iNumDevs);
	  if (ftStatus == FT_OK) {
	    for (i = 0; i < (int)iNumDevs; i++) {  
	      printf("Dev %d:\n",i);  
		
	      printf("  Flags=0x%x\n",devInfo[i].Flags);
	      printf("  Type=0x%x\n",devInfo[i].Type);
	      printf("  ID=0x%x\n",devInfo[i].ID);
	      printf("  LocId=0x%x\n",devInfo[i].LocId);
	      printf("  SerialNumber=%s\n",devInfo[i].SerialNumber);
	      printf("  Description=%s\n",devInfo[i].Description);
	      printf("  ftHandle=0x%lx\n",(unsigned long int)devInfo[i].ftHandle);
	    }
	  } 	
	}
	
	for(i = 0; i < MAX_DEVICES; i++) {
		pcBufLD[i] = cBufLD[i];
	}
	pcBufLD[MAX_DEVICES] = NULL;


	// Opening the device by a description (name)

	int match = 0;
	ftStatus = FT_ListDevices(pcBufLD, &iNumDevs, FT_LIST_ALL | FT_OPEN_BY_DESCRIPTION);
	if (ftStatus == FT_OK) {
	  printf("Devices found: %d\n",iNumDevs);
	  for (i = 0; i< (int)iNumDevs; i++) {
	    // printf("DV %d %s\n", i, pcBufLD[i]);
	    if (strcmp((char*)pcBufLD[i],DevName)==0) {
	      printf("Matching device found %s\n",DevName);
	      match = 1;
	      break;
	    }
	  }
	}

	if (!match) {
	  printf("No matching device found %s\n",DevName);
	  return 1;
	}

	ftStatus = FT_OpenEx(DevName,FT_OPEN_BY_DESCRIPTION,&ftHandle);
	if(ftStatus != FT_OK) {
	  printf("Error FT_OpenEx(%d)\n",ftStatus);
	  return 1;
	}

	// by here the device should be opened

	ftStatus = FT_ResetDevice(ftHandle); // Reset
	if(ftStatus != FT_OK) {
	  printf("Error FT_ResetDevice(%d)\n",ftStatus);
	  FT_Close(ftHandle);
	  return 1;
	}

	ftStatus = FT_SetBitMode(ftHandle,0x00,0x02);
	if(ftStatus != FT_OK) {
	  printf("Error FT_SetBitMode(%d)\n",ftStatus);
	  FT_Close(ftHandle);
	  return 1;
	}

	ftStatus = Sync_to_MPSSE(ftHandle);
	if(ftStatus != FT_OK) {
	  printf("Error Sync_to_MPSSE(%d)\n",ftStatus);
	  FT_Close(ftHandle);
	  return 1;
	}

	i = 0;
	cBufWrite[i] = 0x86; i++; // set clk to 6MHz
	cBufWrite[i] = 0x00; i++;
	cBufWrite[i] = 0x00; i++;
	cBufWrite[i] = 0x80; i++; // set data bits low byte
	cBufWrite[i] = 0x06; i++; // value
	cBufWrite[i] = 0x87; i++; // direction
	ftStatus = FT_Write(ftHandle,cBufWrite,i,&dwBytesWritten);
	if(ftStatus != FT_OK) {
	  printf("Error FT_Write(%d)\n",ftStatus);
	  FT_Close(ftHandle);
	  return 1;
	}

	ftStatus = FT_ResetDevice(ftHandle); // Reset
	if(ftStatus != FT_OK) {
	  printf("Error FT_ResetDevice(%d)\n",ftStatus);
	  FT_Close(ftHandle);
	  return 1;
	}
	
	ftStatus = FT_SetBaudRate(ftHandle,3000000);
	if(ftStatus != FT_OK) {
	  printf("Error FT_SetBaudRate(%d)\n",ftStatus);
	  FT_Close(ftHandle);
	  return 1;
	}

	// Downloading an RBF file

	int nread, done, passed;
	int fd = open(fname,O_RDONLY,0);
	if(!fd) {
	  printf("Error opening RBF file %s\n",fname);
	  FT_Close(ftHandle);
	  return 1;
	}

	ftStatus = StartConfig(ftHandle);
	done = (ftStatus == FT_OK);
	// printf("ftStatus, done %d, %d",ftStatus,done);
	if (done) {
	  done = 0;
	  printf("Programming");
	  while((nread = read(fd, &cBufWrite[3], BUF_SIZE)) > 0) {
	    cBufWrite[0] = 0x19; // send bytes
	    cBufWrite[1] = (nread-1)%256;
	    cBufWrite[2] = (nread-1)/256;
	    // printf("Start Writing %d %d\n",cBufWrite[1],cBufWrite[2]);
	    ftStatus = FT_Write(ftHandle,cBufWrite,nread+3,&dwBytesWritten);
	    if(ftStatus != FT_OK) {
	      printf("Error FT_Write(%d)\n",ftStatus);
	      FT_Close(ftHandle);
	      return 1;
	    }
	    //printf(" %d bytes have been written to the device\n",dwBytesWritten);
	    printf(".");
	    done   = (CheckDone(ftHandle) == FT_OK);
	    passed = (CheckStat(ftHandle) == FT_OK);
	    if (done || (!passed)) break;
	  }
	}
	if (done) {
	  CompleteProg(ftHandle); // for last 10 clocks
	  printf("Programmed OK\n");
	} else {
	  if (!passed) {
	    printf("Programming Failed - nStatus\n");
	  } else {
	    printf("Programming Failed - ran out of file\n");
	    CompleteProg(ftHandle);
	  }
	}

	close(fd);
	  
	// Reseting the interface

	i = 0;
	cBufWrite[i] = 0x80; i++; // set data bits to low byte
	cBufWrite[i] = 0x06; i++; // value
	cBufWrite[i] = 0x87; i++; // direction
	cBufWrite[i] = 0x80; i++; // set data bits to low byte
	cBufWrite[i] = 0x86; i++; // value
	cBufWrite[i] = 0x87; i++; // direction
	cBufWrite[i] = 0x80; i++; // set data bits to low byte
	cBufWrite[i] = 0x06; i++; // value
	cBufWrite[i] = 0x87; i++; // direction
	ftStatus = FT_Write(ftHandle,cBufWrite,i,&dwBytesWritten);
	if(ftStatus != FT_OK) {
	  printf("Error FT_Write(%d)\n",ftStatus);
	  FT_Close(ftHandle);
	  return 1;
	}

	FT_Close(ftHandle);
		
	return 0;
}

FT_STATUS Sync_to_MPSSE(FT_HANDLE ftHandle) {
//
// This should satisfy outstanding commands.
//
// We will use $AA and $AB as commands which
// are invalid so that the MPSSE block should echo these
// back to us preceded with an $FA
//
  FT_STATUS ftStatus;
  DWORD     dwBytesRead;
  DWORD     dwBytesWritten;
  unsigned char  cBufWrite[16];
  unsigned char *pcBufRead = NULL;
  int       j;

  ftStatus = FT_GetQueueStatus(ftHandle,&dwBytesRead);
  if(ftStatus != FT_OK) return ftStatus;

  if (dwBytesRead > 0) {
    pcBufRead = (unsigned char *)realloc(pcBufRead,dwBytesRead);
    ftStatus = FT_Read(ftHandle,pcBufRead,dwBytesRead,&dwBytesRead);
    if(ftStatus != FT_OK) return ftStatus;
  }

  // write 0xAA

  do {
    cBufWrite[0] = 0xAA;
    ftStatus = FT_Write(ftHandle,cBufWrite,1,&dwBytesWritten);
    if(ftStatus != FT_OK) return ftStatus;
    
    ftStatus = FT_GetQueueStatus(ftHandle,&dwBytesRead);
    // printf("stat,bwrit,bread %d, %d, %d\n",ftStatus,dwBytesWritten,dwBytesRead);
  } while (!(dwBytesRead > 0 || ftStatus != FT_OK));

  if(ftStatus != FT_OK) return ftStatus;

  pcBufRead = (unsigned char *)realloc(pcBufRead,dwBytesRead);
  ftStatus = FT_Read(ftHandle,pcBufRead,dwBytesRead,&dwBytesRead);
  if(ftStatus != FT_OK) return ftStatus;

  //for(j=0; j<dwBytesRead; j++) printf("after 0xAA read: %02X\n",pcBufRead[j]);
    
  char Done = 0;
  j = 0;
  do {
    if (pcBufRead[j] == 0xFA) {
      if (j < (int)dwBytesRead-2) {
	if (pcBufRead[j+1] == 0xAA) Done = 1;
      }
    }
    j++;
  } while (!(j == (int)dwBytesRead || Done));

  // printf("done: %d\n",Done);

  // write 0xAB

  cBufWrite[0] = 0xAB;
  ftStatus = FT_Write(ftHandle,cBufWrite,1,&dwBytesWritten);
  if(ftStatus != FT_OK) return ftStatus;

  do {
    ftStatus = FT_GetQueueStatus(ftHandle,&dwBytesRead);
  } while (!(dwBytesRead > 0 || ftStatus != FT_OK));

  if(ftStatus != FT_OK) return ftStatus;

  pcBufRead = (unsigned char *)realloc(pcBufRead,dwBytesRead);
  ftStatus = FT_Read(ftHandle,pcBufRead,dwBytesRead,&dwBytesRead);
  if(ftStatus != FT_OK) return ftStatus;
    
  //for(j=0; j<dwBytesRead; j++) printf("after 0xAA read: %02X\n",pcBufRead[j]);

  Done = 0;
  j = 0;
  do {
    if (pcBufRead[j] == 0xFA) {
      if (j <= (int)dwBytesRead-2) {
	if (pcBufRead[j+1] == 0xAB) Done = 1;
      }
    }
    j++;
  } while (!(j == (int)dwBytesRead || Done));

  // printf("done: %d\n",Done);

  // Exit

  if (!Done) return FT_OTHER_ERROR;

  return FT_OK;
}

FT_STATUS StartConfig(FT_HANDLE ftHandle) {
  int i;
  unsigned char cBufWrite[16];
  
  FT_STATUS ftStatus;
  DWORD     dwBytesWritten;


  i = 0;
  cBufWrite[i] = 0x80; i++; // set data bits to low byte
  cBufWrite[i] = 0x06; i++; // value
  cBufWrite[i] = 0x87; i++; // direction
  cBufWrite[i] = 0x80; i++; // set data bits to low byte
  cBufWrite[i] = 0x02; i++; // value
  cBufWrite[i] = 0x87; i++; // direction
  cBufWrite[i] = 0x80; i++; // set data bits to low byte
  cBufWrite[i] = 0x06; i++; // value
  cBufWrite[i] = 0x87; i++; // direction
  ftStatus = FT_Write(ftHandle,cBufWrite,i,&dwBytesWritten);
  if(ftStatus != FT_OK) return ftStatus;

  ftStatus = CheckDone(ftHandle);
  if(ftStatus == FT_OK) return FT_OTHER_ERROR;

  return FT_OK; 
}
  
FT_STATUS CheckDone(FT_HANDLE ftHandle) {

  FT_STATUS ftStatus;
  DWORD     dwBytesRead;
  DWORD     dwBytesWritten;
  unsigned char  cBufWrite[16];
  unsigned char *pcBufRead = NULL;
  //int       i;
		    
  // Read out garbage characters (?)

  ftStatus = FT_GetQueueStatus(ftHandle,&dwBytesRead);
  if(ftStatus != FT_OK) return ftStatus;

  if (dwBytesRead > 0) {
    pcBufRead = (unsigned char *)realloc(pcBufRead,dwBytesRead);
    ftStatus = FT_Read(ftHandle,pcBufRead,dwBytesRead,&dwBytesRead);
    if(ftStatus != FT_OK) return ftStatus;
  }

  // Send some command, wait for and read the responce

  cBufWrite[0] = 0x81;
  cBufWrite[1] = 0x87;
  ftStatus = FT_Write(ftHandle,cBufWrite,2,&dwBytesWritten);
  if(ftStatus != FT_OK) return ftStatus;

  do {
    ftStatus = FT_GetQueueStatus(ftHandle,&dwBytesRead);
  } while (!(dwBytesRead > 0 || ftStatus != FT_OK));
  
  if(ftStatus != FT_OK) return ftStatus;

  pcBufRead = (unsigned char *)realloc(pcBufRead,dwBytesRead);
  ftStatus = FT_Read(ftHandle,pcBufRead,dwBytesRead,&dwBytesRead);
  if(ftStatus != FT_OK) return ftStatus;
		 
  // Decision is base on the responce
 
  //printf("CheckDone: read back byte is %02X %02X\n",pcBufRead[0],(pcBufRead[0] & 0x10));
  if(!(pcBufRead[0] & 0x10)) return FT_OTHER_ERROR;
  
  return FT_OK;
}

FT_STATUS CheckStat(FT_HANDLE ftHandle) {

  FT_STATUS ftStatus;
  DWORD     dwBytesRead;
  DWORD     dwBytesWritten;
  unsigned char  cBufWrite[16];
  unsigned char *pcBufRead = NULL;
  //int       i;
		    
  // Read out garbage characters (?)

  ftStatus = FT_GetQueueStatus(ftHandle,&dwBytesRead);
  if(ftStatus != FT_OK) return ftStatus;

  if (dwBytesRead > 0) {
    pcBufRead = (unsigned char *)realloc(pcBufRead,dwBytesRead);
    ftStatus = FT_Read(ftHandle,pcBufRead,dwBytesRead,&dwBytesRead);
    if(ftStatus != FT_OK) return ftStatus;
  }

  // Send some command, wait for and read the responce

  cBufWrite[0] = 0x81;
  cBufWrite[1] = 0x87;
  ftStatus = FT_Write(ftHandle,cBufWrite,2,&dwBytesWritten);
  if(ftStatus != FT_OK) return ftStatus;

  do {
    ftStatus = FT_GetQueueStatus(ftHandle,&dwBytesRead);
  } while (!(dwBytesRead > 0 || ftStatus != FT_OK));
  
  if(ftStatus != FT_OK) return ftStatus;

  pcBufRead = (unsigned char *)realloc(pcBufRead,dwBytesRead);
  ftStatus = FT_Read(ftHandle,pcBufRead,dwBytesRead,&dwBytesRead);
  if(ftStatus != FT_OK) return ftStatus;
		 
  // Decision is base on the responce
 
  if(!(pcBufRead[0] & 0x08)) return FT_OTHER_ERROR;
  
  return FT_OK;
}

void CompleteProg(FT_HANDLE ftHandle) {

  //FT_STATUS ftStatus;
  DWORD          dwBytesWritten;
  unsigned char  cBufWrite[16];
  int       i;

  i = 0;
  cBufWrite[i] = 0x19; i++; // clk data out on -ve edge LSB
  cBufWrite[i] = 0x01; i++; // 2 bytes
  cBufWrite[i] = 0x00; i++; 
  cBufWrite[i] = 0x06; i++; 
  cBufWrite[i] = 0x06; i++; 
  FT_Write(ftHandle,cBufWrite,i,&dwBytesWritten);
}
*/
