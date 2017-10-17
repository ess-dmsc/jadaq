/**
 * jadaq (Just Another DAQ)
 * Copyright (C) 2017  Troels Blum <troels@blum.dk>
 *
 * @file
 * @author Troels Blum <troels@blum.dk>
 * @section LICENSE
 * This program is free software: you can redistribute it and/or modify
 *        it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 *         but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @section DESCRIPTION
 * Wraps CAEN access for debugging purposes.
 *
 */

#include <stdio.h>
#include <stdint.h>
#define __USE_GNU
#include <dlfcn.h>

#define OSTREAM stdout

int fun();

typedef enum CAEN_Comm_ConnectionType {
    CAENComm_USB = 0,
    CAENComm_OpticalLink = 1,
    CAENComm_PCI_OpticalLink = 1, // DEPRECATED
    CAENComm_PCIE_OpticalLink = 1, // DEPRECATED
} CAENComm_ConnectionType;

typedef enum CAENComm_ErrorCode {
    CAENComm_Success		     =  0,     /* Operation completed successfully             			*/
    CAENComm_VMEBusError         = -1,     /* VME bus error during the cycle               			*/
    CAENComm_CommError           = -2,     /* Communication error                          			*/
    CAENComm_GenericError        = -3,     /* Unspecified error                            			*/
    CAENComm_InvalidParam        = -4,     /* Invalid parameter                            			*/
    CAENComm_InvalidLinkType     = -5,     /* Invalid Link Type                            			*/
    CAENComm_InvalidHandler      = -6,     /* Invalid device handler                       			*/
    CAENComm_CommTimeout		 = -7,     /* Communication Timeout						   			*/
    CAENComm_DeviceNotFound      = -8,     /* Unable to Open the requested Device          			*/
    CAENComm_MaxDevicesError     = -9,     /* Maximum number of devices exceeded        		    */
    CAENComm_DeviceAlreadyOpen   = -10,    /* The device is already opened							*/
    CAENComm_NotSupported		 = -11,    /* Not supported function								*/
    CAENComm_UnusedBridge	     = -12,    /* There aren't board controlled by that CAEN Bridge		*/
    CAENComm_Terminated		     = -13,    /* Communication terminated by the Device				*/
} CAENComm_ErrorCode;

typedef enum CAENCOMM_INFO {
    CAENComm_PCI_Board_SN		= 0,		/* s/n of the PCI/PCIe board	*/
    CAENComm_PCI_Board_FwRel	= 1,		/* Firmware Release fo the PCI/PCIe board	*/
    CAENComm_VME_Bridge_SN		= 2,		/* s/n of the VME bridge */
    CAENComm_VME_Bridge_FwRel1	= 3,		/* Firmware Release for the VME bridge */
    CAENComm_VME_Bridge_FwRel2	= 4,		/* Firmware Release for the optical chipset inside the VME bridge (V2718 only)*/
    CAENComm_VMELIB_handle		= 5,		/* The VMELib handle used by CAENCOMM */
} CAENCOMM_INFO;


typedef enum IRQLevels {
    IRQ1 = 0x01,        /* Interrupt level 1                            */
    IRQ2 = 0x02,        /* Interrupt level 2                            */
    IRQ3 = 0x04,        /* Interrupt level 3                            */
    IRQ4 = 0x08,        /* Interrupt level 4                            */
    IRQ5 = 0x10,        /* Interrupt level 5                            */
    IRQ6 = 0x20,        /* Interrupt level 6                            */
    IRQ7 = 0x40         /* Interrupt level 7                            */
} IRQLevels;

#ifdef __cplusplus
extern "C" {
#endif

typedef CAENComm_ErrorCode (*DEBUG_CAENComm_OpenDevice)(CAENComm_ConnectionType LinkType, int LinkNum, int ConetNode, uint32_t VMEBaseAddress, int *handle);
typedef CAENComm_ErrorCode (*DEBUG_CAENComm_CloseDevice)(int handle);
typedef CAENComm_ErrorCode (*DEBUG_CAENComm_Write32)(int handle, uint32_t Address, uint32_t Data);
typedef CAENComm_ErrorCode (*DEBUG_CAENComm_Write16)(int handle, uint32_t Address, uint16_t Data);
typedef CAENComm_ErrorCode (*DEBUG_CAENComm_Read32)(int handle, uint32_t Address, uint32_t *Data);
typedef CAENComm_ErrorCode (*DEBUG_CAENComm_Read16)(int handle, uint32_t Address, uint16_t *Data);
typedef CAENComm_ErrorCode (*DEBUG_CAENComm_MultiRead32)(int handle, uint32_t *Address, int nCycles, uint32_t *data, CAENComm_ErrorCode *ErrorCode);
typedef CAENComm_ErrorCode (*DEBUG_CAENComm_MultiRead16)(int handle, uint32_t *Address, int nCycles, uint16_t *data, CAENComm_ErrorCode *ErrorCode);
typedef CAENComm_ErrorCode (*DEBUG_CAENComm_MultiWrite16)(int handle, uint32_t *Address, int nCycles, uint16_t *data, CAENComm_ErrorCode *ErrorCode);
typedef CAENComm_ErrorCode (*DEBUG_CAENComm_MultiWrite32)(int handle, uint32_t *Address, int nCycles, uint32_t *data, CAENComm_ErrorCode *ErrorCode);
typedef CAENComm_ErrorCode (*DEBUG_CAENComm_BLTRead)(int handle, uint32_t Address, uint32_t *Buff, int BltSize, int *nw);
typedef CAENComm_ErrorCode (*DEBUG_CAENComm_MBLTRead)(int handle, uint32_t Address, uint32_t *Buff, int BltSize, int *nw);
typedef CAENComm_ErrorCode (*DEBUG_CAENComm_VMEIRQCheck)(int VMEhandle, uint8_t *Mask);
typedef CAENComm_ErrorCode (*DEBUG_CAENComm_IRQDisable)(int handle);
typedef CAENComm_ErrorCode (*DEBUG_CAENComm_IRQEnable)(int handle);
typedef CAENComm_ErrorCode (*DEBUG_CAENComm_VMEIACKCycle16)(int VMEhandle, IRQLevels Level, int *BoardID);
typedef CAENComm_ErrorCode (*DEBUG_CAENComm_VMEIACKCycle32)(int VMEhandle, IRQLevels Level, int *BoardID);
typedef CAENComm_ErrorCode (*DEBUG_CAENComm_IACKCycle)(int handle, IRQLevels Level, int *BoardID);
typedef CAENComm_ErrorCode (*DEBUG_CAENComm_IRQWait)(int handle, uint32_t Timeout);
typedef CAENComm_ErrorCode (*DEBUG_CAENComm_VMEIRQWait)(CAENComm_ConnectionType LinkType, int LinkNum, int ConetNode, uint8_t IRQMask, uint32_t Timeout, int *VMEHandle);
typedef CAENComm_ErrorCode (*DEBUG_CAENComm_Info)(int handle, CAENCOMM_INFO info, void *data);

CAENComm_ErrorCode CAENComm_OpenDevice(CAENComm_ConnectionType LinkType, int LinkNum, int ConetNode, uint32_t VMEBaseAddress, int *handle)
{
    DEBUG_CAENComm_OpenDevice fun;
    fun = (DEBUG_CAENComm_OpenDevice)dlsym(RTLD_NEXT,"CAENComm_OpenDevice");
    fprintf(OSTREAM,"DEBUG CAENComm_OpenDevice(%i,%i,%i,%i,[int*]) -> ", LinkType, LinkNum, ConetNode, VMEBaseAddress);
    fflush(OSTREAM);
    CAENComm_ErrorCode res = fun(LinkType, LinkNum, ConetNode, VMEBaseAddress, handle);
    fprintf(OSTREAM,"%i, (...,%i)\n", res, *handle);
    return res;
}

CAENComm_ErrorCode CAENComm_CloseDevice(int handle)
{
    DEBUG_CAENComm_CloseDevice fun;
    fun = (DEBUG_CAENComm_CloseDevice)dlsym(RTLD_NEXT,"CAENComm_CloseDevice");
    fprintf(OSTREAM,"DEBUG CAENComm_CloseDevice(%i) -> ",handle);
    fflush(OSTREAM);
    CAENComm_ErrorCode res = fun(handle);
    fprintf(OSTREAM,"%i, ()\n", res);
    return res;
}

CAENComm_ErrorCode CAENComm_Write32(int handle, uint32_t Address, uint32_t Data)
{
    DEBUG_CAENComm_Write32 fun;
    fun = (DEBUG_CAENComm_Write32)dlsym(RTLD_NEXT,"CAENComm_Write32");
    fprintf(OSTREAM,"DEBUG CAENComm_Write32(%i,0x%x,0x%x) -> ",handle, Address, Data);
    fflush(OSTREAM);
    CAENComm_ErrorCode res = fun(handle, Address, Data);
    fprintf(OSTREAM,"%i, ()\n", res);
    return res;
}

CAENComm_ErrorCode CAENComm_Write16(int handle, uint32_t Address, uint16_t Data)
{
    DEBUG_CAENComm_Write16 fun;
    fun = (DEBUG_CAENComm_Write16)dlsym(RTLD_NEXT,"CAENComm_Write16");
    fprintf(OSTREAM,"DEBUG CAENComm_Write16(%i,0x%x,0x%x) -> ",handle, Address, Data);
    fflush(OSTREAM);
    CAENComm_ErrorCode res = fun(handle, Address, Data);
    fprintf(OSTREAM,"%i, ()\n", res);
    return res;
}

CAENComm_ErrorCode CAENComm_Read32(int handle, uint32_t Address, uint32_t *Data)
{
    DEBUG_CAENComm_Read32 fun;
    fun = (DEBUG_CAENComm_Read32)dlsym(RTLD_NEXT,"CAENComm_Read32");
    fprintf(OSTREAM,"DEBUG CAENComm_Read32(%i,0x%x,[uint32_t*]) -> ",handle, Address);
    fflush(OSTREAM);
    CAENComm_ErrorCode res = fun(handle, Address, Data);
    fprintf(OSTREAM,"%i, (...,0x%x)\n", res, *Data);
    return res;
}

CAENComm_ErrorCode CAENComm_Read16(int handle, uint32_t Address, uint16_t *Data)
{
    DEBUG_CAENComm_Read16 fun;
    fun = (DEBUG_CAENComm_Read16)dlsym(RTLD_NEXT,"CAENComm_Read16");
    fprintf(OSTREAM,"DEBUG CAENComm_Read16(%i,0x%x,[uint16_t*]) ->",handle, Address);
    fflush(OSTREAM);
    CAENComm_ErrorCode res = fun(handle, Address, Data);
    fprintf(OSTREAM,"%i, (...,0x%x)\n", res, *Data);
    return res;
}

#define PRINTARRAY(PTR,N,T) \
    fprintf(OSTREAM,"[");\
    for (int c = 0; c < (N); ++c)\
        fprintf(OSTREAM,T,PTR[c]);\
    if (N)\
        fprintf(OSTREAM,"\b");\
    fprintf(OSTREAM,"]");

CAENComm_ErrorCode CAENComm_MultiRead32(int handle, uint32_t *Address, int nCycles, uint32_t *data, CAENComm_ErrorCode *ErrorCode)
{
    DEBUG_CAENComm_MultiRead32 fun;
    fun = (DEBUG_CAENComm_MultiRead32)dlsym(RTLD_NEXT,"CAENComm_MultiRead32");
    fprintf(OSTREAM,"DEBUG CAENComm_MultiRead32(%i,",handle);
    PRINTARRAY(Address,nCycles,"0x%x,")
    fprintf(OSTREAM,",%i,[uint32_t*],[CAENComm_ErrorCode*]) -> ",nCycles);
    fflush(OSTREAM);
    CAENComm_ErrorCode res = fun(handle, Address, nCycles, data, ErrorCode);
    fprintf(OSTREAM,"%i, (...,", res);
    PRINTARRAY(data,nCycles,"0x%x,")
    fprintf(OSTREAM,",");
    PRINTARRAY(ErrorCode,nCycles,"%i,")
    fprintf(OSTREAM,")\n");
    return res;
}

CAENComm_ErrorCode CAENComm_MultiRead16(int handle, uint32_t *Address, int nCycles, uint16_t *data, CAENComm_ErrorCode *ErrorCode)
{
    DEBUG_CAENComm_MultiRead16 fun;
    fun = (DEBUG_CAENComm_MultiRead16)dlsym(RTLD_NEXT,"CAENComm_MultiRead16");
    fprintf(OSTREAM,"DEBUG CAENComm_MultiRead16(%i,",handle);
    PRINTARRAY(Address,nCycles,"0x%x,")
    fprintf(OSTREAM,",%i,[uint32_t*],[CAENComm_ErrorCode*]) -> ",nCycles);
    fflush(OSTREAM);
    CAENComm_ErrorCode res = fun(handle, Address, nCycles, data, ErrorCode);
    fprintf(OSTREAM,"%i, (...,", res);
    PRINTARRAY(data,nCycles,"0x%x,")
    fprintf(OSTREAM,",");
    PRINTARRAY(ErrorCode,nCycles,"%i,")
    fprintf(OSTREAM,")\n");
    return res;
}

CAENComm_ErrorCode CAENComm_MultiWrite16(int handle, uint32_t *Address, int nCycles, uint16_t *data, CAENComm_ErrorCode *ErrorCode)
{
    DEBUG_CAENComm_MultiWrite16 fun;
    fun = (DEBUG_CAENComm_MultiWrite16)dlsym(RTLD_NEXT,"CAENComm_MultiWrite16");
    fprintf(OSTREAM,"DEBUG CAENComm_MultiWrite16(%i,",handle);
    PRINTARRAY(Address,nCycles,"0x%x,")
    fprintf(OSTREAM,",%i,",nCycles);
    PRINTARRAY(data,nCycles,"0x%x,")
    fprintf(OSTREAM,",[CAENComm_ErrorCode*]) -> ");
    fflush(OSTREAM);
    CAENComm_ErrorCode res = fun(handle, Address, nCycles, data, ErrorCode);
    fprintf(OSTREAM,"%i, (...,", res);
    PRINTARRAY(ErrorCode,nCycles,"%i,")
    fprintf(OSTREAM,")\n");
    return res;
}

CAENComm_ErrorCode CAENComm_MultiWrite32(int handle, uint32_t *Address, int nCycles, uint32_t *data, CAENComm_ErrorCode *ErrorCode)
{
    DEBUG_CAENComm_MultiWrite32 fun;
    fun = (DEBUG_CAENComm_MultiWrite32)dlsym(RTLD_NEXT,"CAENComm_MultiWrite32");
    fprintf(OSTREAM,"DEBUG CAENComm_MultiWrite32(%i,",handle);
    PRINTARRAY(Address,nCycles,"0x%x,")
    fprintf(OSTREAM,",%i,",nCycles);
    PRINTARRAY(data,nCycles,"0x%x,")
    fprintf(OSTREAM,",[CAENComm_ErrorCode*]) -> ");
    fflush(OSTREAM);
    CAENComm_ErrorCode res = fun(handle, Address, nCycles, data, ErrorCode);
    fprintf(OSTREAM,"%i, (...,", res);
    PRINTARRAY(ErrorCode,nCycles,"%i,")
    fprintf(OSTREAM,")\n");
    return res;
}

CAENComm_ErrorCode CAENComm_BLTRead(int handle, uint32_t Address, uint32_t *Buff, int BltSize, int *nw)
{
    DEBUG_CAENComm_BLTRead fun;
    fun = (DEBUG_CAENComm_BLTRead)dlsym(RTLD_NEXT,"CAENComm_BLTRead");
    fprintf(OSTREAM,"DEBUG CAENComm_BLTRead(%i,0x%x,%p,%i,[int*]) -> ",handle, Address, Buff, BltSize);
    fflush(OSTREAM);
    CAENComm_ErrorCode res = fun(handle, Address, Buff, BltSize, nw);
    fprintf(OSTREAM,"%i, (...,%i)\n", res, *nw);
    return res;
}

CAENComm_ErrorCode CAENComm_MBLTRead(int handle, uint32_t Address, uint32_t *Buff, int BltSize, int *nw)
{
    DEBUG_CAENComm_MBLTRead fun;
    fun = (DEBUG_CAENComm_MBLTRead)dlsym(RTLD_NEXT,"CAENComm_MBLTRead");
    fprintf(OSTREAM,"DEBUG CAENComm_MBLTRead(%i,0x%x,%p,%i,[int*]) -> ",handle, Address, Buff, BltSize);
    fflush(OSTREAM);
    CAENComm_ErrorCode res = fun(handle, Address, Buff, BltSize, nw);
    fprintf(OSTREAM,"%i, (...,%i)\n", res, *nw);
    return res;
}

CAENComm_ErrorCode CAENComm_VMEIRQCheck(int VMEhandle, uint8_t *Mask)
{
    DEBUG_CAENComm_VMEIRQCheck fun;
    fun = (DEBUG_CAENComm_VMEIRQCheck)dlsym(RTLD_NEXT,"CAENComm_VMEIRQCheck");
    fprintf(OSTREAM,"DEBUG CAENComm_VMEIRQCheck(%i,[uint8_t*]) -> ",VMEhandle);
    fflush(OSTREAM);
    CAENComm_ErrorCode res = fun(VMEhandle, Mask);
    fprintf(OSTREAM,"%i, (0x%x)\n", res, *Mask);
    return res;
}

CAENComm_ErrorCode CAENComm_IRQDisable(int handle)
{
    DEBUG_CAENComm_IRQDisable fun;
    fun = (DEBUG_CAENComm_IRQDisable)dlsym(RTLD_NEXT,"CAENComm_IRQDisable");
    fprintf(OSTREAM,"DEBUG CAENComm_IRQDisable(%i) -> ",handle);
    fflush(OSTREAM);
    CAENComm_ErrorCode res = fun(handle);
    fprintf(OSTREAM,"%i, ()\n", res);
    return res;
}

CAENComm_ErrorCode CAENComm_IRQEnable(int handle)
{
    DEBUG_CAENComm_IRQEnable fun;
    fun = (DEBUG_CAENComm_IRQEnable)dlsym(RTLD_NEXT,"CAENComm_IRQEnable");
    fprintf(OSTREAM,"DEBUG CAENComm_IRQEnable(%i) -> ",handle);
    fflush(OSTREAM);
    CAENComm_ErrorCode res = fun(handle);
    fprintf(OSTREAM,"%i, ()\n", res);
    return res;
}

CAENComm_ErrorCode CAENComm_VMEIACKCycle16(int VMEhandle, IRQLevels Level, int *BoardID)
{
    DEBUG_CAENComm_VMEIACKCycle16 fun;
    fun = (DEBUG_CAENComm_VMEIACKCycle16)dlsym(RTLD_NEXT,"CAENComm_VMEIACKCycle16");
    fprintf(OSTREAM,"DEBUG CAENComm_VMEIACKCycle16(%i,0x%x,[int*]) -> ",VMEhandle, Level);
    fflush(OSTREAM);
    CAENComm_ErrorCode res = fun(VMEhandle, Level, BoardID);
    fprintf(OSTREAM,"%i, (...,%i)\n", res, *BoardID);
    return res;
}

CAENComm_ErrorCode CAENComm_VMEIACKCycle32(int VMEhandle, IRQLevels Level, int *BoardID)
{
    DEBUG_CAENComm_VMEIACKCycle32 fun;
    fun = (DEBUG_CAENComm_VMEIACKCycle32)dlsym(RTLD_NEXT,"CAENComm_VMEIACKCycle32");
    fprintf(OSTREAM,"DEBUG CAENComm_VMEIACKCycle32(%i,0x%x,[int*]) -> ",VMEhandle, Level);
    fflush(OSTREAM);
    CAENComm_ErrorCode res = fun(VMEhandle, Level, BoardID);
    fprintf(OSTREAM,"%i, (...,%i)\n", res, *BoardID);
    return res;
}

CAENComm_ErrorCode CAENComm_IACKCycle(int handle, IRQLevels Level, int *BoardID)
{
    DEBUG_CAENComm_IACKCycle fun;
    fun = (DEBUG_CAENComm_IACKCycle)dlsym(RTLD_NEXT,"CAENComm_IACKCycle");
    fprintf(OSTREAM,"DEBUG CAENComm_IACKCycle(%i,0x%x,[int*]) -> ",handle, Level);
    fflush(OSTREAM);
    CAENComm_ErrorCode res = fun(handle, Level, BoardID);
    fprintf(OSTREAM,"%i, (...,%i)\n", res, *BoardID);
    return res;
}

CAENComm_ErrorCode CAENComm_IRQWait(int handle, uint32_t Timeout)
{
    DEBUG_CAENComm_IRQWait fun;
    fun = (DEBUG_CAENComm_IRQWait)dlsym(RTLD_NEXT,"CAENComm_IRQWait");
    fprintf(OSTREAM,"DEBUG CAENComm_IRQWait(%i,%i) -> ",handle, Timeout);
    fflush(OSTREAM);
    CAENComm_ErrorCode res = fun(handle, Timeout);
    fprintf(OSTREAM,"%i, ()\n", res);
    return res;
}

CAENComm_ErrorCode CAENComm_VMEIRQWait(CAENComm_ConnectionType LinkType, int LinkNum, int ConetNode, uint8_t IRQMask, uint32_t Timeout, int *VMEHandle)
{
    DEBUG_CAENComm_VMEIRQWait fun;
    fun = (DEBUG_CAENComm_VMEIRQWait)dlsym(RTLD_NEXT,"CAENComm_VMEIRQWait");
    fprintf(OSTREAM,"DEBUG CAENComm_VMEIRQWait(%i,%i,%i,0x%x,%i,[int*]) -> ",LinkType, LinkNum, ConetNode, IRQMask, Timeout);
    fflush(OSTREAM);
    CAENComm_ErrorCode res = fun(LinkType, LinkNum, ConetNode, IRQMask, Timeout,VMEHandle);
    fprintf(OSTREAM,"%i, (...,%i)\n", res, *VMEHandle);
    return res;
}

CAENComm_ErrorCode CAENComm_Info(int handle, CAENCOMM_INFO info, void *data)
{
    DEBUG_CAENComm_Info fun;
    fun = (DEBUG_CAENComm_Info)dlsym(RTLD_NEXT,"CAENComm_Info");
    fprintf(OSTREAM,"DEBUG CAENComm_Info(%i,%i,[void*]) -> ",handle, info);
    fflush(OSTREAM);
    CAENComm_ErrorCode res = fun(handle, info, data);
    fprintf(OSTREAM,"%i, (...,%p)\n", res, data);
    return res;
}

#ifdef __cplusplus
}
#endif
