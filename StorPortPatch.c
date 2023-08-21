
/**
 *******************************************************************************
 ** Copyright (C) 2020 Kai Schtrom                                            **
 **                                                                           **
 ** All rights reserved.                                                      **
 **                                                                           **
 *******************************************************************************
 **                                                                           **
 ** Redistribution and use in source and binary forms, with or without        **
 ** modification, are permitted provided that the following conditions are    **
 ** met:                                                                      **
 **                                                                           **
 **   1. Redistributions of source code must retain the above copyright       **
 **      notice, this list of conditions and the following disclaimer.        **
 **                                                                           **
 **   2. Redistributions in binary form must reproduce the above copyright    **
 **      notice, this list of conditions and the following disclaimer in the  **
 **      documentation and/or other materials provided with the distribution. **
 **                                                                           **
 ** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS   **
 ** IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, **
 ** THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR    **
 ** PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR         **
 ** CONTRIBUTORS BE LIABLE FOR ANY DIRECT,INDIRECT, INCIDENTAL, SPECIAL,      **
 ** EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,       **
 ** PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR        **
 ** PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF    **
 ** LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING      **
 ** NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS        **
 ** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.              **
 **                                                                           **
 ** The views and conclusions contained in the software and documentation     **
 ** are those of the authors and should not be interpreted as representing    **
 ** official policies, either expressed or implied, of Intel Corporation,     **
 ** Integrated Device Technology Inc., or Sandforce Corporation.              **
 **                                                                           **
 *******************************************************************************
**/

/*
 * File: StorPortPatch.c
 */

#include <ntddk.h>
#include ".\inc\ddk\storport.h"
#include <stdarg.h>


//------------------------------------------------------------------------------
// StorpAllocatePool -> function code 0
//------------------------------------------------------------------------------
ULONG StorpAllocatePool(__in PVOID HwDeviceExtension,__in ULONG NumberOfBytes,__in ULONG Tag,PVOID *BufferPointer)
{
	UNREFERENCED_PARAMETER(HwDeviceExtension);

	if(BufferPointer == NULL)
	{
		return STOR_STATUS_INVALID_PARAMETER;		
	}

	*BufferPointer = NULL;

	*BufferPointer = ExAllocatePoolWithTag(NonPagedPool,NumberOfBytes,Tag);
	if(*BufferPointer == NULL)
	{
		return STOR_STATUS_INSUFFICIENT_RESOURCES;
	}

	return STOR_STATUS_SUCCESS;
}


//------------------------------------------------------------------------------
// StorpFreePool -> function code 1
//------------------------------------------------------------------------------
ULONG StorpFreePool(__in PVOID HwDeviceExtension,__in PVOID BufferPointer)
{
	UNREFERENCED_PARAMETER(HwDeviceExtension);

	if(BufferPointer == NULL)
	{
		return STOR_STATUS_INVALID_PARAMETER;
	}

	ExFreePoolWithTag(BufferPointer,0);

	return STOR_STATUS_SUCCESS;
}


//------------------------------------------------------------------------------
// StorpAllocateContiguousMemorySpecifyCacheNode -> function code 24
//------------------------------------------------------------------------------
ULONG StorpAllocateContiguousMemorySpecifyCacheNode(__in PVOID HwDeviceExtension,__in SIZE_T NumberOfBytes,__in PHYSICAL_ADDRESS LowestAcceptableAddress,__in PHYSICAL_ADDRESS HighestAcceptableAddress,__in_opt PHYSICAL_ADDRESS BoundaryAddressMultiple,__in MEMORY_CACHING_TYPE CacheType,__in NODE_REQUIREMENT PreferredNode,PVOID* BufferPointer)
{
	UNREFERENCED_PARAMETER(HwDeviceExtension);
	UNREFERENCED_PARAMETER(PreferredNode);

	if(BufferPointer == NULL)
	{
		return STOR_STATUS_INVALID_PARAMETER;		
	}

	*BufferPointer = NULL;

	// we can not use MmAllocateContiguousMemorySpecifyCacheNode for Windows Server 2003, because the minimum target OS is Windows Vista
	*BufferPointer = MmAllocateContiguousMemorySpecifyCache(NumberOfBytes,LowestAcceptableAddress,HighestAcceptableAddress,BoundaryAddressMultiple,CacheType);
	if(*BufferPointer == NULL)
	{
		return STOR_STATUS_INSUFFICIENT_RESOURCES;
	}

	return STOR_STATUS_SUCCESS;
}


//------------------------------------------------------------------------------
// StorpFreeContiguousMemorySpecifyCache -> function code 25
//------------------------------------------------------------------------------
ULONG StorpFreeContiguousMemorySpecifyCache(__in PVOID HwDeviceExtension,__in_bcount (NumberOfBytes) PVOID BaseAddress,__in SIZE_T NumberOfBytes,__in MEMORY_CACHING_TYPE CacheType)
{
	UNREFERENCED_PARAMETER(HwDeviceExtension);

	MmFreeContiguousMemorySpecifyCache(BaseAddress,NumberOfBytes,CacheType);

	return STOR_STATUS_SUCCESS;
}


//------------------------------------------------------------------------------
// StorpInitializeTimer -> function code 32
//------------------------------------------------------------------------------
ULONG StorpInitializeTimer(__in PVOID HwDeviceExtension,__out PVOID *TimerHandle)
{
	UNREFERENCED_PARAMETER(HwDeviceExtension);
	UNREFERENCED_PARAMETER(TimerHandle);

	// without success status we run into an NT_ASSERT
	return STOR_STATUS_SUCCESS;
}


//------------------------------------------------------------------------------
// StorpFreeTimer -> function code 34
//------------------------------------------------------------------------------
ULONG StorpFreeTimer(__in PVOID HwDeviceExtension,__in PVOID TimerHandle)
{
	UNREFERENCED_PARAMETER(HwDeviceExtension);
	UNREFERENCED_PARAMETER(TimerHandle);

	// without success status we run into an NT_ASSERT
	return STOR_STATUS_SUCCESS;
}


//------------------------------------------------------------------------------
// StorPortExtendedFunction
//------------------------------------------------------------------------------
ULONG StorPortPatchExtendedFunction(__in STORPORT_FUNCTION_CODE FunctionCode,__in PVOID HwDeviceExtension,...)
{
	ULONG status;
	va_list argptr;

	if(HwDeviceExtension == NULL)
	{
		return STOR_STATUS_INVALID_PARAMETER;
	}

#if DBG
	DbgPrint("StorPortPatchExtendedFunction: Function code %d\n",FunctionCode);
#endif

	va_start(argptr,HwDeviceExtension);

	switch(FunctionCode)
	{
		// without handling this function code we can not allocate memory
		// function code 0
		case ExtFunctionAllocatePool:
		{
			ULONG NumberOfBytes;
			ULONG Tag;
			PVOID *BufferPointer;
			NumberOfBytes = va_arg(argptr,ULONG);
			Tag = va_arg(argptr,ULONG);
			BufferPointer = va_arg(argptr,PVOID*);

			status = StorpAllocatePool(HwDeviceExtension,NumberOfBytes,Tag,BufferPointer);
			break;
		}
		// without handling this function code we can not free memory
		// function code 1
		case ExtFunctionFreePool:
		{
			PVOID BufferPointer;
			BufferPointer = va_arg(argptr,PVOID);

			status = StorpFreePool(HwDeviceExtension,BufferPointer);
			break;
		}
		// without handling this function code we can not allocate a range of physically contiguous and cache aligned memory
		// function code 24
		case ExtFunctionAllocateContiguousMemorySpecifyCacheNode:
		{
			SIZE_T NumberOfBytes;
			PHYSICAL_ADDRESS LowestAcceptableAddress;
			PHYSICAL_ADDRESS HighestAcceptableAddress;
			PHYSICAL_ADDRESS BoundaryAddressMultiple;
			MEMORY_CACHING_TYPE CacheType;
			NODE_REQUIREMENT PreferredNode;
			PVOID* BufferPointer;
			NumberOfBytes = va_arg(argptr,SIZE_T);
			LowestAcceptableAddress = va_arg(argptr,PHYSICAL_ADDRESS);
			HighestAcceptableAddress = va_arg(argptr,PHYSICAL_ADDRESS);
			BoundaryAddressMultiple = va_arg(argptr,PHYSICAL_ADDRESS);
			CacheType = va_arg(argptr,MEMORY_CACHING_TYPE);
			PreferredNode = va_arg(argptr,NODE_REQUIREMENT);
			BufferPointer = va_arg(argptr,PVOID*);
			
			status = StorpAllocateContiguousMemorySpecifyCacheNode(HwDeviceExtension,NumberOfBytes,LowestAcceptableAddress,HighestAcceptableAddress,BoundaryAddressMultiple,CacheType,PreferredNode,BufferPointer);
			break;
		}
		// without handling this function code we can not free a range of physically contiguous and cache aligned memory
		// function code 25
		case ExtFunctionFreeContiguousMemorySpecifyCache:
		{
			PVOID BaseAddress;
			SIZE_T NumberOfBytes;
			MEMORY_CACHING_TYPE CacheType;
			BaseAddress = va_arg(argptr,PVOID);
			NumberOfBytes = va_arg(argptr,SIZE_T);
			CacheType = va_arg(argptr,MEMORY_CACHING_TYPE);
			
			status = StorpFreeContiguousMemorySpecifyCache(HwDeviceExtension,BaseAddress,NumberOfBytes,CacheType);
			break;
		}
		// without handling this function code we run into an NT_ASSERT
		// function code 32
		case ExtFunctionInitializeTimer:
		{
			PVOID *TimerHandle;
			TimerHandle = va_arg(argptr,PVOID*);

			status = StorpInitializeTimer(HwDeviceExtension,TimerHandle);
			break;
		}
		// without handling this function code we run into an NT_ASSERT
		// function code 34
		case ExtFunctionFreeTimer:
		{
			PVOID TimerHandle;
			TimerHandle = va_arg(argptr,PVOID);

			status = StorpFreeTimer(HwDeviceExtension,TimerHandle);
			break;
		}
		// all other function codes go here
		default:
		{
#if DBG
			DbgPrint("StorPortPatchExtendedFunction: Unknown function code %d\n",FunctionCode);
#endif
			// we handle all other function codes as not implemented
			status = STOR_STATUS_NOT_IMPLEMENTED;
			break;
		}
	}

	va_end(argptr);
	return status;
}

