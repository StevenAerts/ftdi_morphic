// FT2232H EEPROM Modify.cpp : Defines the entry point for the console application.
//

// NOTE:	This code is provided as an example only and is not supported or guaranteed by FTDI.
//			It is the responsibility of the recipient/user to ensure the correct operation of 
//			any software which is created based upon this example.

//#include "stdafx.h"
//#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "ftd2xx.h"

int main()
{
	//********************************************************
	//Definitions
	//********************************************************

	FT_HANDLE fthandle;
	FT_STATUS status;
	
	BOOLEAN Dev_Found = FALSE;

	FT_PROGRAM_DATA ftData;

	WORD VendorIdBuf = 0x0403;
	WORD ProductIdBuf = 0x6010;
	char ManufacturerBuf[32];
	char ManufacturerIdBuf[16];
	char DescriptionBuf[64];
	char SerialNumberBuf[16];

	ftData.Signature1 = 0x00000000;		// Always 0x00000000
	ftData.Signature2 = 0xffffffff;		// Always 0xFFFFFFFF
	ftData.Version = 3;	// Header - FT_PROGRAM_DATA version 0 = original (FT232B), 1 = FT2232 extensions, 2 = FT232R extensions, 3 = FT2232H extensions, 4 = FT4232H extensions, 5 = FT232H extensions

	ftData.VendorId = VendorIdBuf;
	ftData.ProductId = ProductIdBuf;
	ftData.Manufacturer = ManufacturerBuf;
	ftData.ManufacturerId = ManufacturerIdBuf;
	ftData.Description = DescriptionBuf;
	ftData.SerialNumber = SerialNumberBuf;

/*	ftData.MaxPower;
	ftData.PnP;
	ftData.SelfPowered;
	ftData.RemoteWakeup;

	//'FT2232H features require section below
	ftData.PullDownEnable7;		// non-zero if pull down enabled 
	ftData.SerNumEnable7;		// non-zero if serial number to be used 
	ftData.ALSlowSlew;			// non-zero if AL pins have slow slew 
	ftData.ALSchmittInput;		// non-zero if AL pins are Schmitt input 
	ftData.ALDriveCurrent;		// valid values are 4mA, 8mA, 12mA, 16mA 
	ftData.AHSlowSlew;			// non-zero if AH pins have slow slew 
	ftData.AHSchmittInput;		// non-zero if AH pins are Schmitt input 
	ftData.AHDriveCurrent;		// valid values are 4mA, 8mA, 12mA, 16mA 
	ftData.BLSlowSlew;			// non-zero if BL pins have slow slew 
	ftData.BLSchmittInput;		// non-zero if BL pins are Schmitt input 
	ftData.BLDriveCurrent;		// valid values are 4mA, 8mA, 12mA, 16mA 
	ftData.BHSlowSlew;			// non-zero if BH pins have slow slew 
	ftData.BHSchmittInput;		// non-zero if BH pins are Schmitt input 
	ftData.BHDriveCurrent;		// valid values are 4mA, 8mA, 12mA, 16mA 
	ftData.IFAIsFifo7;			// non-zero if interface is 245 FIFO 
	ftData.IFAIsFifoTar7;		// non-zero if interface is 245 FIFO CPU target 
	ftData.IFAIsFastSer7;		// non-zero if interface is Fast serial 
	ftData.AIsVCP7;				// non-zero if interface is to use VCP drivers 
	ftData.IFBIsFifo7;			// non-zero if interface is 245 FIFO 
	ftData.IFBIsFifoTar7;		// non-zero if interface is 245 FIFO CPU target 
	ftData.IFBIsFastSer7;		// non-zero if interface is Fast serial 
	ftData.BIsVCP7;				// non-zero if interface is to use VCP drivers 
	ftData.PowerSaveEnable;		// non-zero if using BCBUS7 to save power for self-powered
*/

	//********************************************************
	//List Devices
	//********************************************************

	FT_DEVICE_LIST_INFO_NODE *devInfo;
	DWORD numDevs;
	DWORD i;
	
	FT_SetVIDPID(0x0403, 0x9842);

	// create the device information list 
	status = FT_CreateDeviceInfoList(&numDevs);

	if (status != FT_OK) {
		printf("FT_CreateDeviceInfoList status not ok %d\n", status);
		return 0;
	}
	else
	{
		printf("Number of devices is %d\n", numDevs);
		if (numDevs > 0) {
			// allocate storage for list based on numDevs 
			devInfo =
				(FT_DEVICE_LIST_INFO_NODE*)malloc(sizeof(FT_DEVICE_LIST_INFO_NODE)*numDevs);
			// get the device information list 
			status = FT_GetDeviceInfoList(devInfo, &numDevs);
			if (status == FT_OK) {
				for (i = 0; i < numDevs; i++) {
					printf("Dev %d:\n", i);
					printf("Flags=0x%x\n", devInfo[i].Flags);
					printf("Type=0x%x\n", devInfo[i].Type);
					printf("ID=0x%x\n", devInfo[i].ID);
					printf("LocId=0x%x\n", devInfo[i].LocId);
					printf("SerialNumber=%s\n", devInfo[i].SerialNumber);
					printf("Description=%s\n", devInfo[i].Description);
					printf("\n");
				}
			}
		}
	}

	//********************************************************
	//Open the port
	//********************************************************

	for (i = 0; i < numDevs; i++)
	{
		if ((Dev_Found == FALSE) && (devInfo[i].Type == FT_DEVICE_2232H))
		{
			Dev_Found = TRUE;
			
			status = FT_OpenEx("Morph-IC-II A", FT_OPEN_BY_DESCRIPTION, &fthandle);

			if (status != FT_OK)
			{
				printf("Open status not ok %d\n", status);
				printf("Trying to open unprogrammed EEPROM device...\n");
				status = FT_OpenEx("Dual RS232-HS A", FT_OPEN_BY_DESCRIPTION, &fthandle);
				if (status != FT_OK)
				{
					printf("Open status not ok %d\n", status);
					printf("\n");
					return 0;
				}
				else
				{
					printf("Open status OK %d\n", status);
					printf("\n");
				}

				printf("\n");
			}
			else
			{
				printf("Open status OK %d\n", status);
				printf("\n");
			}


			//********************************************************
			//Read the EEPROM
			//********************************************************

			status = FT_EE_Read(fthandle, &ftData);

			if (status != FT_OK)
			{
				printf("EE_Read status not ok %d\n", status);
				if (status == FT_EEPROM_NOT_PROGRAMMED)
				{
					printf("EEPROM is not programmed! Programming with preset values.\n");
					printf("\n");

					char ManufacturerBufNew[32] = "FTDI";
					char ManufacturerIdBufNew[16] = "FT";
					char DescriptionBufNew[64] = "FT2232H_MM";
					char SerialNumberBufNew[16] = "FT11111";

					ftData.Manufacturer = ManufacturerBufNew;
					ftData.ManufacturerId = ManufacturerIdBufNew;
					ftData.Description = DescriptionBufNew;
					ftData.SerialNumber = SerialNumberBufNew;

					ftData.MaxPower = 90;
					ftData.PnP = 1;
					ftData.SelfPowered = 0;
					ftData.RemoteWakeup = 0;

					//'FT2232H features require section below
					ftData.PullDownEnable7 = 0;
					ftData.SerNumEnable7 = 1;
					ftData.ALSlowSlew = 0;
					ftData.ALSchmittInput = 0;
					ftData.ALDriveCurrent = 4;
					ftData.AHSlowSlew = 0;
					ftData.AHSchmittInput = 0;
					ftData.AHDriveCurrent = 4;
					ftData.BLSlowSlew = 0;
					ftData.BLSchmittInput = 0;
					ftData.BLDriveCurrent = 4;
					ftData.BHSlowSlew = 0;
					ftData.BHSchmittInput = 0;
					ftData.BHDriveCurrent = 4;
					ftData.IFAIsFifo7 = 0;
					ftData.IFAIsFifoTar7 = 0;
					ftData.IFAIsFastSer7 = 0;
					ftData.AIsVCP7 = 1;
					ftData.IFBIsFifo7 = 0;
					ftData.IFBIsFifoTar7 = 0;
					ftData.IFBIsFastSer7 = 0;
					ftData.BIsVCP7 = 1;
					ftData.PowerSaveEnable = 0;

					//********************************************************
					//program a blank EEPROM first.
					//********************************************************

					//dont for now: status = FT_EE_Program(fthandle, &ftData);

					if (status != FT_OK)
					{
						printf("Initial FT_EE_Program not ok %d\n", status);
						printf("\n");
						return 0;
					}
					else
					{
						printf("Initial FT_EE_Program OK!\n");
						printf("\n");
					}

				}
				else
				{
					return 0;
				}
			}
			else
			{
				printf("EEPROM is already programmed! Reading EEPROM.\n");
				printf("\n");

				printf("Signature1 =  0x%04x\n", ftData.Signature1);
				printf("Signature2 =  0x%04x\n", ftData.Signature2);
				printf("Version =  0x%04x\n", ftData.Version);


				printf("VendorID =  0x%04x\n", ftData.VendorId);
				printf("ProductID =  0x%04x\n", ftData.ProductId);
				printf("Manufacturer =  %s\n", ftData.Manufacturer);
				printf("ManufacturerID =  %s\n", ftData.ManufacturerId);
				printf("Description =  %s\n", ftData.Description);
				printf("SerialNumber =  %s\n", ftData.SerialNumber);
				printf("MaxPower =  %d\n", ftData.MaxPower);
				printf("PnP =  %x\n", ftData.PnP);
				printf("SelfPowered =  %x\n", ftData.SelfPowered);
				printf("RemoteWakeup =  %x\n", ftData.RemoteWakeup);

				printf("PullDownEnable7 =  %x\n", ftData.PullDownEnable7);
				printf("SerNumEnable7 =  %x\n", ftData.SerNumEnable7);
				printf("ALSlowSlew =  %x\n", ftData.ALSlowSlew);
				printf("ALSchmittInput =  %x\n", ftData.ALSchmittInput);
				printf("ALDriveCurrent =  %x\n", ftData.ALDriveCurrent);
				printf("AHSlowSlew =  %x\n", ftData.AHSlowSlew);
				printf("AHSchmittInput =  %x\n", ftData.AHSchmittInput);
				printf("AHDriveCurrent =  %x\n", ftData.AHDriveCurrent);
				printf("BLSlowSlew =  %x\n", ftData.BLSlowSlew);
				printf("BLSchmittInput =  %x\n", ftData.BLSchmittInput);
				printf("BLDriveCurrent =  %x\n", ftData.BLDriveCurrent);
				printf("BHSlowSlew =  %x\n", ftData.BHSlowSlew);
				printf("BHSchmittInput =  %x\n", ftData.BHSchmittInput);
				printf("BHDriveCurrent =  %x\n", ftData.BHDriveCurrent);
				printf("IFAIsFifo7 =  %x\n", ftData.IFAIsFifo7);
				printf("IFAIsFifoTar7 =  %x\n", ftData.IFAIsFifoTar7);
				printf("IFAIsFastSer7 =  %x\n", ftData.IFAIsFastSer7);
				printf("AIsVCP7 =  %x\n", ftData.AIsVCP7);
				printf("IFBIsFifo7 =  %x\n", ftData.IFBIsFifo7);
				printf("IFBIsFifoTar7 =  %x\n", ftData.IFBIsFifoTar7);
				printf("IFBIsFastSer7 =  %x\n", ftData.IFBIsFastSer7);
				printf("BIsVCP7 =  %x\n", ftData.BIsVCP7);
				printf("PowerSaveEnable =  %x\n", ftData.PowerSaveEnable);
				printf("\n");
			}


			//********************************************************
			//change serial number from one that was read.
			//********************************************************

			//ftData.SerialNumber = "FT12345";

			ftData.ProductId = 0x6010;
			status = FT_EE_Program(fthandle, &ftData);

			if (status != FT_OK)
			{
				printf("EE_Program status not ok %d\n", status);
				return 0;
			}
			else
			{
				printf("EE_Program status ok %d\n", status);
				printf("\n");
			}


			//********************************************************
			// Delay
			//********************************************************

			sleep(1);


			//********************************************************
			// Re - Read
			//********************************************************


			//ftData.SerialNumber = SerialNumberBuf; //Reset to variable

			printf("Reading EEPROM to check changed values!\n");
			printf("\n");

			status = FT_EE_Read(fthandle, &ftData);

			if (status != FT_OK)
			{
				printf("EE_Read status not ok %d\n", status);
				return 0;
			}
			else
			{
				printf("Signature1 =  0x%04x\n", ftData.Signature1);
				printf("Signature2 =  0x%04x\n", ftData.Signature2);
				printf("Version =  0x%04x\n", ftData.Version);

				printf("VendorID =  0x%04x\n", ftData.VendorId);
				printf("ProductID =  0x%04x\n", ftData.ProductId);
				printf("Manufacturer =  %s\n", ftData.Manufacturer);
				printf("ManufacturerID =  %s\n", ftData.ManufacturerId);
				printf("Description =  %s\n", ftData.Description);
				printf("SerialNumber =  %s\n", ftData.SerialNumber);
				printf("MaxPower =  %d\n", ftData.MaxPower);
				printf("PnP =  %x\n", ftData.PnP);
				printf("SelfPowered =  %x\n", ftData.SelfPowered);
				printf("RemoteWakeup =  %x\n", ftData.RemoteWakeup);

				printf("PullDownEnable7 =  %x\n", ftData.PullDownEnable7);
				printf("SerNumEnable7 =  %x\n", ftData.SerNumEnable7);
				printf("ALSlowSlew =  %x\n", ftData.ALSlowSlew);
				printf("ALSchmittInput =  %x\n", ftData.ALSchmittInput);
				printf("ALDriveCurrent =  %x\n", ftData.ALDriveCurrent);
				printf("AHSlowSlew =  %x\n", ftData.AHSlowSlew);
				printf("AHSchmittInput =  %x\n", ftData.AHSchmittInput);
				printf("AHDriveCurrent =  %x\n", ftData.AHDriveCurrent);
				printf("BLSlowSlew =  %x\n", ftData.BLSlowSlew);
				printf("BLSchmittInput =  %x\n", ftData.BLSchmittInput);
				printf("BLDriveCurrent =  %x\n", ftData.BLDriveCurrent);
				printf("BHSlowSlew =  %x\n", ftData.BHSlowSlew);
				printf("BHSchmittInput =  %x\n", ftData.BHSchmittInput);
				printf("BHDriveCurrent =  %x\n", ftData.BHDriveCurrent);
				printf("IFAIsFifo7 =  %x\n", ftData.IFAIsFifo7);
				printf("IFAIsFifoTar7 =  %x\n", ftData.IFAIsFifoTar7);
				printf("IFAIsFastSer7 =  %x\n", ftData.IFAIsFastSer7);
				printf("AIsVCP7 =  %x\n", ftData.AIsVCP7);
				printf("IFBIsFifo7 =  %x\n", ftData.IFBIsFifo7);
				printf("IFBIsFifoTar7 =  %x\n", ftData.IFBIsFifoTar7);
				printf("IFBIsFastSer7 =  %x\n", ftData.IFBIsFastSer7);
				printf("BIsVCP7 =  %x\n", ftData.BIsVCP7);
				printf("PowerSaveEnable =  %x\n", ftData.PowerSaveEnable);
				printf("\n");
			}

			//*****************************************************
			//Close the port
			//*****************************************************

			// Close the device
			status = FT_Close(fthandle);

			//Increment i to avoid opening channel B of the same device
			i++;

		}

	}

	printf("Press Return To End Program");
	getchar();
	printf("closed \n");

	return 0;
}

