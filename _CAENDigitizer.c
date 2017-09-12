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

#define MAX_GROUPS    8

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

static CAEN_DGTZ_ErrorCode V1740DPP_QDC_MallocDPPEvents(int handle, CAEN_DGTZ_DPP_QDC_Event_t **events, uint32_t *allocatedSize)
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

CAEN_DGTZ_ErrorCode CAENDGTZ_API _CAEN_DGTZ_MallocDPPEvents(int handle, void **events, uint32_t *allocatedSize)
{
    CAEN_DGTZ_DPPFirmware_t firmware;
    CAEN_DGTZ_ErrorCode err = _CAEN_DGTZ_GetDPPFirmwareType(handle, &firmware);
    if (err != CAEN_DGTZ_Success) { return err; }
    if (firmware == CAEN_DGTZ_DPPFirmware_QDC)
        return V1740DPP_QDC_MallocDPPEvents(handle, (CAEN_DGTZ_DPP_QDC_Event_t **) events, allocatedSize);
    else
        return CAEN_DGTZ_MallocDPPEvents(handle, events, allocatedSize);
}

static CAEN_DGTZ_ErrorCode V1740DPP_QDC_FreeDPPEvents(int handle, CAEN_DGTZ_DPP_QDC_Event_t **events)
{
    free(events[0]);
    return CAEN_DGTZ_Success;
}

CAEN_DGTZ_ErrorCode CAENDGTZ_API _CAEN_DGTZ_FreeDPPEvents(int handle, void **events)
{
    CAEN_DGTZ_DPPFirmware_t firmware;
    CAEN_DGTZ_ErrorCode err = _CAEN_DGTZ_GetDPPFirmwareType(handle, &firmware);
    if (err != CAEN_DGTZ_Success) { return err; }
    if (firmware == CAEN_DGTZ_DPPFirmware_QDC)
        return V1740DPP_QDC_FreeDPPEvents(handle, (CAEN_DGTZ_DPP_QDC_Event_t**)events);
    else
        return CAEN_DGTZ_FreeDPPEvents(handle, events);
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

CAEN_DGTZ_ErrorCode CAENDGTZ_API _CAEN_DGTZ_MallocReadoutBuffer(int handle, char **buffer, uint32_t *size)
{
    CAEN_DGTZ_DPPFirmware_t firmware;
    CAEN_DGTZ_ErrorCode err = _CAEN_DGTZ_GetDPPFirmwareType(handle, &firmware);
    if (err != CAEN_DGTZ_Success) { return err; }
    if (firmware == CAEN_DGTZ_DPPFirmware_QDC)
        return V1740DPP_QDC_MallocReadoutBuffer(handle, buffer, size);
    else
        return CAEN_DGTZ_MallocReadoutBuffer(handle, buffer, size);
}