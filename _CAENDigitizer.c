/*
 * jadaq (Just Another DAQ)
 * Copyright (C) 2017  Troels Blum <troels@blum.dk>
 *
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
 */

/*
 * This file contains functions and definitions that belong in the CAENDigitizer library.
 * But either they are not there or they are not exposed
 */

#include <stdlib.h>
#include <stdio.h>
#include "_CAENDigitizer.h"

#define QDC_FUNCTION(F_NAME,HANDLE,...)                                         \
    CAEN_DGTZ_DPPFirmware_t firmware;                                           \
    CAEN_DGTZ_ErrorCode err = _CAEN_DGTZ_GetDPPFirmwareType(HANDLE, &firmware); \
    if (err != CAEN_DGTZ_Success) { return err; }                               \
    if (firmware == CAEN_DGTZ_DPPFirmware_QDC)                                  \
        return V1740DPP_QDC_##F_NAME(HANDLE, __VA_ARGS__);                      \
    else                                                                        \
        return CAEN_DGTZ_##F_NAME(HANDLE, __VA_ARGS__);


static inline CAEN_DGTZ_DPPFirmware_t getDPPFirmwareType(int vx, int vy, int handle)
{
	CAEN_DGTZ_AcquisitionMode_t mode;

	switch (vx) {
	case V1724_DPP_PHA_CODE:
    case V1730_DPP_PHA_CODE:
		return vy>16 ? CAEN_DGTZ_DPPFirmware_PHA : CAEN_DGTZ_DPPFirmwareNotSupported;
	case V1720_DPP_CI_CODE:
		return vy>16 ? CAEN_DGTZ_DPPFirmware_CI : CAEN_DGTZ_DPPFirmwareNotSupported;
	case V1720_DPP_PSD_CODE: // This is for x720
	case V1751_DPP_PSD_CODE: // This is for x751
	case V1730_DPP_PSD_CODE: // This is for x730
		return CAEN_DGTZ_DPPFirmware_PSD;
	case V1743_DPP_CI_CODE: // This is for X743
		CAEN_DGTZ_GetSAMAcquisitionMode(handle, &mode);
		if(mode == CAEN_DGTZ_AcquisitionMode_DPP_CI)
			return CAEN_DGTZ_DPPFirmware_CI;
	case V1730_DPP_ZLE_CODE: // x X730 ZLE
		return CAEN_DGTZ_DPPFirmware_ZLE;
    case V1740_DPP_QDC_CODE:
        return CAEN_DGTZ_DPPFirmware_QDC;
	default:
		return CAEN_DGTZ_NotDPPFirmware;
	}
}

CAEN_DGTZ_ErrorCode CAENDGTZ_API _CAEN_DGTZ_GetDPPFirmwareType(int handle, CAEN_DGTZ_DPPFirmware_t* firmware)
{
    char str[40];
    int vx, vy;
    CAEN_DGTZ_BoardInfo_t boardInfo;
    if (CAEN_DGTZ_GetInfo(handle, &boardInfo) != CAEN_DGTZ_Success)
        return CAEN_DGTZ_InvalidHandle;

    sscanf(boardInfo.AMC_FirmwareRel, "%d.%d%s", &vx, &vy, str);
    *firmware = getDPPFirmwareType(vx, vy, handle);

    return CAEN_DGTZ_Success;
}

static CAEN_DGTZ_ErrorCode V1740DPP_QDC_MallocDPPEvents(int handle, void **events, uint32_t *allocatedSize)
{
    uint32_t size = MAX_V1740_DPP_GROUP_SIZE*V1740_MAX_CHANNELS*V1740_QDC_MAX_EVENT_QUEUE_DEPTH*sizeof(CAEN_DGTZ_DPP_QDC_Event_t);
    CAEN_DGTZ_DPP_QDC_Event_t* ptr = malloc(size);
    if (ptr == NULL)
        return CAEN_DGTZ_OutOfMemory;
    for(int i=0; i<MAX_V1740_DPP_GROUP_SIZE ; i++)
        events[i] = ptr+V1740_MAX_CHANNELS*V1740_QDC_MAX_EVENT_QUEUE_DEPTH;
    *allocatedSize = size;
    return CAEN_DGTZ_Success;
}

static CAEN_DGTZ_ErrorCode V1740DPP_QDC_FreeDPPEvents(int handle, void **events)
{
    free(events[0]);
    return CAEN_DGTZ_Success;
}

static CAEN_DGTZ_ErrorCode V1740DPP_QDC_MallocReadoutBuffer(int handle, char **buffer, uint32_t *size)
{
    // TODO allocato only the memory needed
    uint32_t mysize = V1740_QDC_MAX_ALLOCATED_MEM_PER_GROUP*MAX_V1740_DPP_GROUP_SIZE;
    char* mybuffer = (char *)malloc(mysize);

    if(mybuffer == NULL)
        return CAEN_DGTZ_OutOfMemory;

    *size = mysize;
    *buffer = mybuffer;
    return CAEN_DGTZ_Success;
}

static CAEN_DGTZ_ErrorCode V1740DPP_QDC_GetRecordLength(int handle, uint32_t* size, int channel)
{
    if (channel < 0)
        return CAEN_DGTZ_InvalidChannelNumber;
    uint32_t s;
    CAEN_DGTZ_ErrorCode err = CAEN_DGTZ_ReadRegister(handle, 0x1024 | channel<<8, &s);
    if (err != CAEN_DGTZ_Success)
        return err;
    *size = s<<3; // TODO why the shift? Not documented
    return CAEN_DGTZ_Success;
}

static CAEN_DGTZ_ErrorCode V1740DPP_QDC_SetRecordLength(int handle, uint32_t size, int channel) {
    CAEN_DGTZ_ErrorCode err;
    if (channel < 0)
        err = CAEN_DGTZ_WriteRegister(handle, 0x8024, size>>3); // TODO why the shift? Not documented
    else
        err = CAEN_DGTZ_WriteRegister(handle, 0x1024 | channel<<8, size>>3); // TODO why the shift? Not documented
    return err;
}


CAEN_DGTZ_ErrorCode CAENDGTZ_API _CAEN_DGTZ_MallocDPPEvents(int handle, void **events, uint32_t *allocatedSize)
{
    QDC_FUNCTION(MallocDPPEvents,handle,events,allocatedSize)
}

CAEN_DGTZ_ErrorCode CAENDGTZ_API _CAEN_DGTZ_FreeDPPEvents(int handle, void **events)
{
    QDC_FUNCTION(FreeDPPEvents,handle,events)
}

CAEN_DGTZ_ErrorCode CAENDGTZ_API _CAEN_DGTZ_MallocReadoutBuffer(int handle, char **buffer, uint32_t *size)
{
    QDC_FUNCTION(MallocReadoutBuffer,handle,buffer,size)
}

CAEN_DGTZ_ErrorCode CAENDGTZ_API _CAEN_DGTZ_SetRecordLength(int handle, uint32_t size, int channel)
{
    QDC_FUNCTION(SetRecordLength,handle,size,channel)
}

CAEN_DGTZ_ErrorCode CAENDGTZ_API _CAEN_DGTZ_GetRecordLength(int handle, uint32_t *size, int channel)
{
    QDC_FUNCTION(GetRecordLength,handle,size,channel)
}
