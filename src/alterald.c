/*
        Loads an RBF file to Morph-IC FPGA board through FT2232C interface and D2XX driver.
	Tested on Debian Etch and SL4.0 Linux systems. The driver can be downloaded
	from http://www.ftdichip.com/Drivers/D2XX.htm. Version 0.4.13 of the driver
	was used for the tests.

	Usage: alterald [-vh]     [file.rbf]      [interface]
	Defaults:        ""     "MariachiV2.rbf" "Morphic-IC A"

	This is basically a translation of Pascal code used for the same purpose and
        distributed as a part of Morph-IC board software CD. Unlike the Pascal program
	this one doesn't have any X-windows interface and supposed to be used from
	a command line.

	Author: Dmitry Vavilov, vavilov@bnl.gov
	Date:   Aug 10, 2007
*/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h> 
#include <unistd.h>
#include "ftc.h"

FT_STATUS Sync_to_MPSSE(FT_HANDLE);
FT_STATUS StartConfig  (FT_HANDLE);
FT_STATUS CheckDone    (FT_HANDLE);
FT_STATUS CheckStat    (FT_HANDLE);
void      CompleteProg (FT_HANDLE);

#define BUF_SIZE 512
#define MAX_DEVICES	5

int main(int argc, char *argv[])
{
	unsigned char 	cBufWrite[BUF_SIZE+3];
	//unsigned char  *pcBufRead = NULL;
	unsigned char  *pcBufLD[MAX_DEVICES + 1];
	unsigned char 	cBufLD[MAX_DEVICES][64];
	//DWORD	dwRxSize = 0;
	DWORD 	dwBytesWritten/*, dwBytesRead*/;
	FT_STATUS	ftStatus;
	FT_HANDLE	ftHandle;
	DWORD	iNumDevs = 0;
	int	i/*, j*/;
	
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
	int fd = open(RBFFileName,O_RDONLY,0);
	if(!fd) {
	  printf("Error opening RBF file %s\n",RBFFileName);
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
  /*ftStatus =*/ FT_Write(ftHandle,cBufWrite,i,&dwBytesWritten);
}
