#include "../ftd2xx.h"
#include <setjmp.h>

#define TRY(exception, code) if ( (code = setjmp(exception)) == 0 )
#define CATCH else
#define THROW(exception, code) longjmp(exception, code);
#define FTC_TRY(code) TRY(ftcException, code)
#define FTC_THROW(code) THROW(ftcException, code)

extern jmp_buf ftcException;
extern FT_STATUS ftcStatus;

enum {
	FTC_NONE,
	FTC_OPEN,
	FTC_OPEN_EX,
	FTC_LIST_DEVICES,
	FTC_CLOSE,
	FTC_READ,
	FTC_WRITE,
	FTC_IO_CTL,
	FTC_SET_BAUD_RATE,
	FTC_SET_DIVISOR,
	FTC_SET_DATA_CHARACTERISTICS,
	FTC_SET_FLOW_CONTROL,
	FTC_RESET_DEVICE,
	FTC_SET_DTR,
	FTC_CLR_DTR,
	FTC_SET_RTS,
	FTC_CLR_RTS,
	
	FTC_GET_QUEUE_STATUS,
	
	FTC_SET_BIT_MODE,
	
	FTC_OTHER
	
};

inline FT_HANDLE FTC_Open(int deviceNumber)
{
	 FT_HANDLE ftHandle;
	 if(!FT_SUCCESS(ftcStatus = FT_Open(deviceNumber, &ftHandle))) 
		FTC_THROW(FTC_OPEN);
	 return ftHandle;
}

inline	FT_HANDLE FTC_OpenEx(PVOID pArg1, DWORD Flags)
{
	 FT_HANDLE ftHandle;
	 if(!FT_SUCCESS(ftcStatus = FT_OpenEx(pArg1, Flags, &ftHandle))) 
		FTC_THROW(FTC_OPEN_EX);
	 return ftHandle;
}

inline void FTC_ListDevices(PVOID pArg1, PVOID pArg2, DWORD Flags)
{
	 if(!FT_SUCCESS(ftcStatus = FT_ListDevices(pArg1, pArg2, Flags))) 
		FTC_THROW(FTC_LIST_DEVICES);
}

inline void FTC_Close(FT_HANDLE ftHandle)
{
	 if(!FT_SUCCESS(ftcStatus = FT_Close(ftHandle))) 
		FTC_THROW(FTC_CLOSE);
}

inline DWORD FTC_Read(FT_HANDLE ftHandle, LPVOID lpBuffer, DWORD dwBytesToRead)
{
	 DWORD dwBytesReturned;
	 if(!FT_SUCCESS(ftcStatus = FT_Read(ftHandle, lpBuffer, dwBytesToRead, &dwBytesReturned))) 
		FTC_THROW(FTC_READ);
	 if(dwBytesReturned != dwBytesToRead)
		FTC_THROW(FTC_READ);
	 return dwBytesReturned;
}

inline DWORD FTC_Write(FT_HANDLE ftHandle, LPVOID lpBuffer, DWORD dwBytesToWrite)
{
	 DWORD dwBytesWritten;
	 if(!FT_SUCCESS(ftcStatus = FT_Write(ftHandle, lpBuffer, dwBytesToWrite, &dwBytesWritten))) 
		FTC_THROW(FTC_WRITE);
	 if(dwBytesWritten != dwBytesToWrite)
		FTC_THROW(FTC_WRITE);
	 return dwBytesWritten;
}

inline DWORD FTC_IoCtl(FT_HANDLE ftHandle, DWORD dwIoControlCode, LPVOID lpInBuf, DWORD nInBufSize, LPVOID lpOutBuf, DWORD nOutBufSize, LPOVERLAPPED lpOverlapped)
{
	 DWORD dwBytesReturned;
	 if(!FT_SUCCESS(ftcStatus = FT_IoCtl(ftHandle, dwIoControlCode, lpInBuf, nInBufSize, lpOutBuf, nOutBufSize, &dwBytesReturned, lpOverlapped))) 
		FTC_THROW(FTC_IO_CTL);
	 return dwBytesReturned;
}

inline void FTC_SetBaudRate(FT_HANDLE ftHandle, ULONG BaudRate)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetBaudRate(ftHandle, BaudRate))) 
		FTC_THROW(FTC_SET_BAUD_RATE);
}

inline void FTC_SetDivisor(FT_HANDLE ftHandle, USHORT Divisor)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}

inline void FTC_SetDataCharacteristics(FT_HANDLE ftHandle, UCHAR WordLength, UCHAR StopBits, UCHAR Parity)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDataCharacteristics(ftHandle, WordLength, StopBits, Parity))) 
		FTC_THROW(FTC_SET_DATA_CHARACTERISTICS);
}


inline void FTC_SetFlowControl(FT_HANDLE ftHandle, USHORT FlowControl, UCHAR XonChar, UCHAR XoffChar)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetFlowControl(ftHandle, FlowControl, XonChar, XoffChar))) 
		FTC_THROW(FTC_SET_FLOW_CONTROL);
}

inline void FTC_ResetDevice(FT_HANDLE ftHandle)
{
	 if(!FT_SUCCESS(ftcStatus = FT_ResetDevice(ftHandle))) 
		FTC_THROW(FTC_RESET_DEVICE);
}

inline void FTC_SetDtr(FT_HANDLE ftHandle)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDtr(ftHandle))) 
		FTC_THROW(FTC_SET_DTR);
}

inline void FTC_ClrDtr(FT_HANDLE ftHandle)
{
	 if(!FT_SUCCESS(ftcStatus = FT_ClrDtr(ftHandle))) 
		FTC_THROW(FTC_CLR_DTR);
}

inline void FTC_SetRts(FT_HANDLE ftHandle)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetRts(ftHandle))) 
		FTC_THROW(FTC_SET_RTS);
}

inline void FTC_ClrRts(FT_HANDLE ftHandle)
{
	 if(!FT_SUCCESS(ftcStatus = FT_ClrRts(ftHandle))) 
		FTC_THROW(FTC_CLR_RTS);
}

/* TODO:


inline void FTC_GetModemStatus(FT_HANDLE ftHandle, ULONG *pModemStatus)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


inline void FTC_SetChars(FT_HANDLE ftHandle, UCHAR EventChar, UCHAR EventCharEnabled, UCHAR ErrorChar, UCHAR ErrorCharEnabled)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


inline void FTC_Purge(FT_HANDLE ftHandle, ULONG Mask)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


inline void FTC_SetTimeouts(FT_HANDLE ftHandle, ULONG ReadTimeout, ULONG WriteTimeout)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}
*/

inline DWORD FTC_GetQueueStatus(FT_HANDLE ftHandle)
{
	DWORD dwRxBytes;
	if(!FT_SUCCESS(ftcStatus = FT_GetQueueStatus(ftHandle, &dwRxBytes))) 
		FTC_THROW(FTC_GET_QUEUE_STATUS);
	return dwRxBytes;
}

/*
inline void FTC_SetEventNotification(
		FT_HANDLE ftHandle,
		DWORD Mask,
		PVOID Param
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


inline void FTC_GetStatus(
		FT_HANDLE ftHandle,
		DWORD *dwRxBytes,
		DWORD *dwTxBytes,
		DWORD *dwEventDWord
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


inline void FTC_SetBreakOn(
		FT_HANDLE ftHandle
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


inline void FTC_SetBreakOff(
		FT_HANDLE ftHandle
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


inline void FTC_SetWaitMask(
		FT_HANDLE ftHandle,
		DWORD Mask
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


inline void FTC_WaitOnMask(
		FT_HANDLE ftHandle,
		DWORD *Mask
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


inline void FTC_GetEventStatus(
		FT_HANDLE ftHandle,
		DWORD *dwEventDWord
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


inline void FTC_ReadEE(
		FT_HANDLE ftHandle,
		DWORD dwWordOffset,
		LPWORD lpwValue
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


inline void FTC_WriteEE(
		FT_HANDLE ftHandle,
		DWORD dwWordOffset,
		WORD wValue
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


inline void FTC_EraseEE(
		FT_HANDLE ftHandle
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


inline void FTC_EE_Program(
		FT_HANDLE ftHandle,
		PFT_PROGRAM_DATA pData
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


inline void FTC_EE_ProgramEx(
		FT_HANDLE ftHandle,
		PFT_PROGRAM_DATA pData,
		char *Manufacturer,
		char *ManufacturerId,
		char *Description,
		char *SerialNumber
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


inline void FTC_EE_Read(
		FT_HANDLE ftHandle,
		PFT_PROGRAM_DATA pData
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


inline void FTC_EE_ReadEx(
		FT_HANDLE ftHandle,
		PFT_PROGRAM_DATA pData,
		char *Manufacturer,
		char *ManufacturerId,
		char *Description,
		char *SerialNumber
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


inline void FTC_EE_UASize(
		FT_HANDLE ftHandle,
		LPDWORD lpdwSize
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


inline void FTC_EE_UAWrite(
		FT_HANDLE ftHandle,
		PUCHAR pucData,
		DWORD dwDataLen
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


inline void FTC_EE_UARead(
		FT_HANDLE ftHandle,
		PUCHAR pucData,
		DWORD dwDataLen,
		LPDWORD lpdwBytesRead
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


inline void FTC_EEPROM_Read(
		FT_HANDLE ftHandle,
		void *eepromData,
		DWORD eepromDataSize,
		char *Manufacturer,
		char *ManufacturerId,
		char *Description,
		char *SerialNumber
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}



inline void FTC_EEPROM_Program(
		FT_HANDLE ftHandle,
		void *eepromData,
		DWORD eepromDataSize,
		char *Manufacturer,
		char *ManufacturerId,
		char *Description,
		char *SerialNumber
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}



inline void FTC_SetLatencyTimer(
		FT_HANDLE ftHandle,
		UCHAR ucLatency
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


inline void FTC_GetLatencyTimer(
		FT_HANDLE ftHandle,
		PUCHAR pucLatency
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}

*/
inline void FTC_SetBitMode(FT_HANDLE ftHandle, UCHAR ucMask, UCHAR ucEnable)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetBitMode(ftHandle, ucMask, ucEnable))) 
		FTC_THROW(FTC_SET_BIT_MODE);
}

/*
inline void FTC_GetBitMode(
		FT_HANDLE ftHandle,
		PUCHAR pucMode
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


inline void FTC_SetUSBParameters(
		FT_HANDLE ftHandle,
		ULONG ulInTransferSize,
		ULONG ulOutTransferSize
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


inline void FTC_SetDeadmanTimeout(
		FT_HANDLE ftHandle,
		ULONG ulDeadmanTimeout
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


#ifndef _WIN32
	// Extra functions for non-Windows platforms to compensate
	// for lack of .INF file to specify Vendor and Product IDs.

FT_SetVIDPID(
		DWORD dwVID, 
		DWORD dwPID
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}

			
FT_GetVIDPID(
		DWORD * pdwVID, 
		DWORD * pdwPID
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


inline void FTC_GetDeviceLocId(
		FT_HANDLE ftHandle,
		LPDWORD lpdwLocId
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}

#endif // _WIN32        

inline void FTC_GetDeviceInfo(
		FT_HANDLE ftHandle,
		FT_DEVICE *lpftDevice,
		LPDWORD lpdwID,
		PCHAR SerialNumber,
		PCHAR Description,
		LPVOID Dummy
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


inline void FTC_StopInTask(
		FT_HANDLE ftHandle
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


inline void FTC_RestartInTask(
		FT_HANDLE ftHandle
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


inline void FTC_SetResetPipeRetryCount(
		FT_HANDLE ftHandle,
		DWORD dwCount
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


inline void FTC_ResetPort(
		FT_HANDLE ftHandle
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


inline void FTC_CyclePort(
		FT_HANDLE ftHandle
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}



	//
	// Win32-type functions
	//

	FTD2XX_API
		FT_HANDLE inline void FTC_W32_CreateFile(
		LPCTSTR					lpszName,
		DWORD					dwAccess,
		DWORD					dwShareMode,
		LPSECURITY_ATTRIBUTES	lpSecurityAttributes,
		DWORD					dwCreate,
		DWORD					dwAttrsAndFlags,
		HANDLE					hTemplate
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


	FTD2XX_API
		BOOL inline void FTC_W32_CloseHandle(
		FT_HANDLE ftHandle
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


	FTD2XX_API
		BOOL inline void FTC_W32_ReadFile(
		FT_HANDLE ftHandle,
		LPVOID lpBuffer,
		DWORD nBufferSize,
		LPDWORD lpBytesReturned,
		LPOVERLAPPED lpOverlapped
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


	FTD2XX_API
		BOOL inline void FTC_W32_WriteFile(
		FT_HANDLE ftHandle,
		LPVOID lpBuffer,
		DWORD nBufferSize,
		LPDWORD lpBytesWritten,
		LPOVERLAPPED lpOverlapped
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


	FTD2XX_API
		DWORD inline void FTC_W32_GetLastError(
		FT_HANDLE ftHandle
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


	FTD2XX_API
		BOOL inline void FTC_W32_GetOverlappedResult(
		FT_HANDLE ftHandle,
		LPOVERLAPPED lpOverlapped,
		LPDWORD lpdwBytesTransferred,
		BOOL bWait
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


	FTD2XX_API
		BOOL inline void FTC_W32_CancelIo(
		FT_HANDLE ftHandle
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


	FTD2XX_API
		BOOL inline void FTC_W32_ClearCommBreak(
		FT_HANDLE ftHandle
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


	FTD2XX_API
		BOOL inline void FTC_W32_ClearCommError(
		FT_HANDLE ftHandle,
		LPDWORD lpdwErrors,
		LPFTCOMSTAT lpftComstat
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


	FTD2XX_API
		BOOL inline void FTC_W32_EscapeCommFunction(
		FT_HANDLE ftHandle,
		DWORD dwFunc
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


	FTD2XX_API
		BOOL inline void FTC_W32_GetCommModemStatus(
		FT_HANDLE ftHandle,
		LPDWORD lpdwModemStatus
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


	FTD2XX_API
		BOOL inline void FTC_W32_GetCommState(
		FT_HANDLE ftHandle,
		LPFTDCB lpftDcb
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


	FTD2XX_API
		BOOL inline void FTC_W32_GetCommTimeouts(
		FT_HANDLE ftHandle,
		FTTIMEOUTS *pTimeouts
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


	FTD2XX_API
		BOOL inline void FTC_W32_PurgeComm(
		FT_HANDLE ftHandle,
		DWORD dwMask
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


	FTD2XX_API
		BOOL inline void FTC_W32_SetCommBreak(
		FT_HANDLE ftHandle
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


	FTD2XX_API
		BOOL inline void FTC_W32_SetCommMask(
		FT_HANDLE ftHandle,
		ULONG ulEventMask
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


	FTD2XX_API
		BOOL inline void FTC_W32_GetCommMask(
		FT_HANDLE ftHandle,
		LPDWORD lpdwEventMask
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


	FTD2XX_API
		BOOL inline void FTC_W32_SetCommState(
		FT_HANDLE ftHandle,
		LPFTDCB lpftDcb
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


	FTD2XX_API
		BOOL inline void FTC_W32_SetCommTimeouts(
		FT_HANDLE ftHandle,
		FTTIMEOUTS *pTimeouts
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


	FTD2XX_API
		BOOL inline void FTC_W32_SetupComm(
		FT_HANDLE ftHandle,
		DWORD dwReadBufferSize,
		DWORD dwWriteBufferSize
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


	FTD2XX_API
		BOOL inline void FTC_W32_WaitCommEvent(
		FT_HANDLE ftHandle,
		PULONG pulEvent,
		LPOVERLAPPED lpOverlapped
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}

inline void FTC_CreateDeviceInfoList(
		LPDWORD lpdwNumDevs
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


inline void FTC_GetDeviceInfoList(
		FT_DEVICE_LIST_INFO_NODE *pDest,
		LPDWORD lpdwNumDevs
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


inline void FTC_GetDeviceInfoDetail(
		DWORD dwIndex,
		LPDWORD lpdwFlags,
		LPDWORD lpdwType,
		LPDWORD lpdwID,
		LPDWORD lpdwLocId,
		LPVOID lpSerialNumber,
		LPVOID lpDescription,
		FT_HANDLE *pftHandle
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}



	//
	// Version information
	//

inline void FTC_GetDriverVersion(
		FT_HANDLE ftHandle,
		LPDWORD lpdwVersion
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


inline void FTC_GetLibraryVersion(
		LPDWORD lpdwVersion
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}



inline void FTC_Rescan(
		void
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


inline void FTC_Reload(
		WORD wVid,
		WORD wPid
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


inline void FTC_GetComPortNumber(
		FT_HANDLE ftHandle,
		LPLONG	lpdwComPortNumber
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}



	//
	// FT232H additional EEPROM functions
	//

inline void FTC_EE_ReadConfig(
		FT_HANDLE ftHandle,
		UCHAR ucAddress,
		PUCHAR pucValue
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


inline void FTC_EE_WriteConfig(
		FT_HANDLE ftHandle,
		UCHAR ucAddress,
		UCHAR ucValue
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


inline void FTC_EE_ReadECC(
		FT_HANDLE ftHandle,
		UCHAR ucOption,
		LPWORD lpwValue
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


inline void FTC_GetQueueStatusEx(
		FT_HANDLE ftHandle,
		DWORD *dwRxBytes
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


inline void FTC_ComPortIdle(
		FT_HANDLE ftHandle
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


inline void FTC_ComPortCancelIdle(
		FT_HANDLE ftHandle
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


inline void FTC_VendorCmdGet(
		FT_HANDLE ftHandle,
		UCHAR Request,
		UCHAR *Buf,
		USHORT Len
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


inline void FTC_VendorCmdSet(
		FT_HANDLE ftHandle,
		UCHAR Request,
		UCHAR *Buf,
		USHORT Len
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


inline void FTC_VendorCmdGetEx(
		FT_HANDLE ftHandle,
		USHORT wValue,
		UCHAR *Buf,
		USHORT Len
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}


inline void FTC_VendorCmdSetEx(
		FT_HANDLE ftHandle,
		USHORT wValue,
		UCHAR *Buf,
		USHORT Len
		)
{
	 if(!FT_SUCCESS(ftcStatus = FT_SetDivisor(ftHandle, Divisor))) 
		FTC_THROW(FTC_SET_DIVISOR);
}

*/

const char* FTC_STATUS_STR[] = {
	"FT_OK",
	"FT_INVALID_HANDLE",
	"FT_DEVICE_NOT_FOUND",
	"FT_DEVICE_NOT_OPENED",
	"FT_IO_ERROR",
	"FT_INSUFFICIENT_RESOURCES",
	"FT_INVALID_PARAMETER",
	"FT_INVALID_BAUD_RATE",

	"FT_DEVICE_NOT_OPENED_FOR_ERASE",
	"FT_DEVICE_NOT_OPENED_FOR_WRITE",
	"FT_FAILED_TO_WRITE_DEVICE",
	"FT_EEPROM_READ_FAILED",
	"FT_EEPROM_WRITE_FAILED",
	"FT_EEPROM_ERASE_FAILED",
	"FT_EEPROM_NOT_PRESENT",
	"FT_EEPROM_NOT_PROGRAMMED",
	"FT_INVALID_ARGS",
	"FT_NOT_SUPPORTED",
	"FT_OTHER_ERROR",
	"FT_DEVICE_LIST_NOT_READY",
};

const char* FTC_FUNCTION_STR[] = {
	"FTC_NONE",
	"FTC_OPEN",
	"FTC_OPEN_EX",
	"FTC_LIST_DEVICES",
	"FTC_CLOSE",
	"FTC_READ",
	"FTC_WRITE",
	"FTC_IO_CTL",
	"FTC_SET_BAUD_RATE",
	"FTC_SET_DIVISOR",
	"FTC_SET_DATA_CHARACTERISTICS",
	"FTC_SET_FLOW_CONTROL",
	"FTC_RESET_DEVICE",
	"FTC_SET_DTR",
	"FTC_CLR_DTR",
	"FTC_SET_RTS",
	"FTC_CLR_RTS",
	
	"FTC_GET_QUEUE_STATUS",

	"FTC_SET_BIT_MODE",
	
	"FTC_OTHER"
	
};
