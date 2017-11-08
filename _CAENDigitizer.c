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
 * This file contains functions and definitions that belong in the CAENDigitizer library.
 * But either they are not there or they are not exposed
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <CAENDigitizer.h>
#include "_CAENDigitizer.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))

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
    /* Allocate the entire chunk in one go and assign slices to event array */
    uint32_t size = MAX_V1740_DPP_GROUP_SIZE*V1740_MAX_CHANNELS*V1740_QDC_MAX_EVENT_QUEUE_DEPTH*sizeof(CAEN_DGTZ_DPP_QDC_Event_t);
    CAEN_DGTZ_DPP_QDC_Event_t* ptr = malloc(size);
    if (ptr == NULL)
        return CAEN_DGTZ_OutOfMemory;
    for(int i=0; i<MAX_V1740_DPP_GROUP_SIZE ; i++) {
        /* NOTE: changed to ptr+i instead of the following original constant assignment!?! */
        //events[i] = ptr+V1740_MAX_CHANNELS*V1740_QDC_MAX_EVENT_QUEUE_DEPTH;
        events[i] = ptr+i;
    }
    *allocatedSize = size;
    return CAEN_DGTZ_Success;
}

static CAEN_DGTZ_ErrorCode V1740DPP_QDC_FreeDPPEvents(int handle, void **events)
{
    /* Free the entire chunk in one go by freeing element 0 */
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

static CAEN_DGTZ_ErrorCode V1740DPP_QDC_MallocDPPWaveforms(int handle, void** waveforms, uint32_t* allocatedSize)
{
    CAEN_DGTZ_ErrorCode err;
    uint32_t enabled;
    uint32_t samples = 0;
    if (err = CAEN_DGTZ_ReadRegister(handle, CAEN_DGTZ_CH_ENABLE_ADD, &enabled) != CAEN_DGTZ_Success)
        return err;

    for(int i=0; i<MAX_V1740_DPP_GROUP_SIZE; ++i)
    {
        if (enabled & (1<<i))
        {
            uint32_t s;
            if (err = V1740DPP_QDC_GetRecordLength(handle, &s, i) != CAEN_DGTZ_Success)
                return err;
            samples = MAX(samples, s);
        }
    }

    uint32_t size = sizeof(CAEN_DGTZ_DPP_PSD_Waveforms_t) + 2*samples*sizeof(uint16_t) + 4*samples*sizeof(uint8_t);
    CAEN_DGTZ_DPP_QDC_Waveforms_t* buffer = (CAEN_DGTZ_DPP_QDC_Waveforms_t*)malloc(size);
    if (buffer == NULL)
        return CAEN_DGTZ_OutOfMemory;
    buffer->Trace1 = (uint16_t*)(buffer+1);
    buffer->Trace2 = buffer->Trace1+samples;
    buffer->DTrace1 = (uint8_t*)(buffer->Trace2+samples);
    buffer->DTrace2 = buffer->DTrace1+samples;
    buffer->DTrace3 = buffer->DTrace2+samples;
    buffer->DTrace4 = buffer->DTrace3+samples;
    *waveforms = buffer;
    *allocatedSize = size;
    return CAEN_DGTZ_Success;
}

static CAEN_DGTZ_ErrorCode V1740DPP_QDC_FreeDPPWaveforms(int handle, void* waveforms)
{
    free(waveforms);
    return CAEN_DGTZ_Success;
}

static inline uint32_t _COMMON_GetChannelAddress(uint32_t base, uint16_t channel) {
    //return base + (0x100 * channel);
    int chShift = 8;
    uint32_t res = base & ~(0xF<<chShift); // set bits [11:8] to zero
    res |= channel << chShift;
    return res;
}

static CAEN_DGTZ_ErrorCode V1740DPP_QDC_SetChannelGroupMask(int handle, uint32_t group, uint32_t channelmask)
{
    if (group < MAX_V1740_DPP_GROUP_SIZE)
    {
        uint32_t address = _COMMON_GetChannelAddress(CAEN_DGTZ_CHANNEL_GROUP_V1740_BASE_ADDRESS, group);
        return CAEN_DGTZ_WriteRegister(handle, address, channelmask);
    } else
        return CAEN_DGTZ_InvalidChannelNumber;
}

static CAEN_DGTZ_ErrorCode V1740DPP_QDC_GetChannelGroupMask(int handle, uint32_t group, uint32_t *channelmask)
{
    if (group < MAX_V1740_DPP_GROUP_SIZE)
    {
        uint32_t address = _COMMON_GetChannelAddress(CAEN_DGTZ_CHANNEL_GROUP_V1740_BASE_ADDRESS, group);
        return CAEN_DGTZ_ReadRegister(handle, address, channelmask);
    } else
        return CAEN_DGTZ_InvalidChannelNumber;

}

static inline uint32_t channelTriggerAddress(uint32_t channel)
{
    uint32_t gr = channel>>3;
    uint32_t ch = channel&7;
    uint32_t address = 0x1000 | gr<<8 | 0xD0 | ch<<2;
    return address;
}

static CAEN_DGTZ_ErrorCode V1740DPP_QDC_SetChannelTriggerThreshold(int handle, uint32_t channel, uint32_t Tvalue)
{
    if (channel > V1740_MAX_CHANNELS)
        return CAEN_DGTZ_InvalidChannelNumber;
    else
        return CAEN_DGTZ_WriteRegister(handle, channelTriggerAddress(channel), Tvalue);
}

static CAEN_DGTZ_ErrorCode V1740DPP_QDC_GetChannelTriggerThreshold(int handle, uint32_t channel, uint32_t* Tvalue)
{
    if (channel > V1740_MAX_CHANNELS)
        return CAEN_DGTZ_InvalidChannelNumber;
    else
        return CAEN_DGTZ_ReadRegister(handle, channelTriggerAddress(channel), Tvalue);
}

static CAEN_DGTZ_ErrorCode V1740DPP_QDC_GetNumEventsPerAggregate(int handle, uint32_t *numEvents, int channel)
{
    if (channel >= 0)
        return CAEN_DGTZ_InvalidParam;
    CAEN_DGTZ_ErrorCode err;
    uint32_t d32;
    if (err = CAEN_DGTZ_ReadRegister(handle, 0x8020, &d32) != CAEN_DGTZ_Success)
        return err;
    *numEvents = d32 & 0x3FF;
    return CAEN_DGTZ_Success;
}

static CAEN_DGTZ_ErrorCode V1740DPP_QDC_SetNumEventsPerAggregate(int handle, uint32_t numEvents, int channel)
{
    if (channel >= 0)
        return CAEN_DGTZ_InvalidParam;
    CAEN_DGTZ_ErrorCode err;
    uint32_t config;

    // TODO actually understand the 0x800C & 0x8020 register correlation
    // and why do we care about waveform?
    if (numEvents == 0) {
        CAEN_DGTZ_ReadRegister(handle, 0x8000, &config);
        if (config & (1<<16)) {                                          /* waveform enabled      */
            err |= CAEN_DGTZ_WriteRegister(handle, 0x800C, 0x3);      /* Buffer organization   */
            err |= CAEN_DGTZ_WriteRegister(handle, 0x8020, 1);        /* Events per aggregate  */
        } else {
            err |= CAEN_DGTZ_WriteRegister(handle, 0x800C, 0xA);      /* Buffer organization   */
            err |= CAEN_DGTZ_WriteRegister(handle, 0x8020, 16);       /* Events per aggregate  */
        }
    } else {
        err |= CAEN_DGTZ_WriteRegister(handle, 0x800C, 0x3);          /* Buffer organization   */
        err |= CAEN_DGTZ_WriteRegister(handle, 0x8020, numEvents);      /* Events per aggregate  */
    }
    return err;
}

/* BEGIN inlined patch from CAEN */
/* NOTE: changed to define these two externals directly here in jadaq */
/* NOTE: changed hard-coded group size of 8 to MAX_GROUPS */
/* extern */unsigned int gEquippedGroups;
/* extern */CAEN_DGTZ_DPP_QDC_Event_t *gEventsGrp[MAX_GROUPS];

static CAEN_DGTZ_ErrorCode V1740DPP_QDC_DecodeDPPAggregate(int handle, uint32_t *data, CAEN_DGTZ_DPP_QDC_Event_t *Events, int *NumEvents) 
{
    uint32_t i,size, evsize, nev, et, eq, ew, ee, pnt;
    
    if (!(data[0] & 0x80000000))
        return CAEN_DGTZ_InvalidEvent;
	
    size   = data[0] & 0x3FFFF;                       
    ew     = (data[1]>>27) & 1;                       
    ee     = (data[1]>>28) & 1;                       
    et     = (data[1]>>29) & 1;                       
    eq     = (data[1]>>30) & 1;                       
    evsize = ((data[1] & 0xFFF) << 2) + et + ee + eq; 


    if(evsize==0) 
        return CAEN_DGTZ_InvalidEvent;
    if ((size - 2) % evsize)
        return CAEN_DGTZ_InvalidEvent;
    nev = (size - 2) / evsize;
    pnt = 2;
    for (i=0; i<nev; i++) {
        
        Events[i].isExtendedTimeStamp = (ee) ? 1 : 0;

        Events[i].Format = data[1];
        if (et)
            Events[i].TimeTag = data[pnt++] & 0xFFFFFFFF;
        else
            Events[i].TimeTag = 0;
        
        if (ew) 
        {
            Events[i].gWaveforms = data + pnt;
            pnt += ((data[1] << 2) & 0xFFF); 
        } 
        else
        {
            Events[i].gWaveforms = NULL;
        }
        
        if (ee) {
            Events[i].Extras = ((uint32_t)(data[pnt++] & 0xFFFFFFFF)); 
            Events[i].TimeTag |= ( (uint64_t)(Events[i].Extras & 0xFFFF) << 32) & 0xFFFFFFFF00000000; /* 48 bit timestamp */
            Events[i].Baseline = (uint16_t)((Events[i].Extras & 0xFFFF0000) >> 16);
        }

        

        if (eq) {
            Events[i].Charge       = (uint16_t)(data[pnt] & 0x0000FFFF);
            Events[i].Pur          = (uint16_t)((data[pnt] & 0x08000000) >> 27);
            Events[i].Overrange    = (uint16_t)((data[pnt] & 0x04000000) >> 26);
            Events[i].SubChannel   = (uint16_t)((data[pnt] >> 28) & 0xF); 
            pnt++;
        }
        else{
            Events[i].Charge = 0;
        }
        Events[i].Extras = 0;
    }
    *NumEvents = nev;
    return CAEN_DGTZ_Success;
}

static CAEN_DGTZ_ErrorCode V1740DPP_QDC_GetDPPEvents(int handle, char *buffer, uint32_t BufferSize, void **events, uint32_t *NumEvents)
{
    CAEN_DGTZ_DPP_QDC_Event_t **Events = (CAEN_DGTZ_DPP_QDC_Event_t **)events;
    unsigned int pnt = 0;
    int index[MAX_GROUPS];
    uint32_t endaggr;
    unsigned int grpmax = MAX_GROUPS;
    char grpmask;
    unsigned int grp;
    int nevgrp;
    
        
    uint32_t *buffer32 = (uint32_t *)buffer;

    memset(index, 0, MAX_GROUPS*sizeof(index[0]));

    while(pnt < (uint32_t)(BufferSize/4) - 1) { 
        endaggr = pnt + (buffer32[pnt] & 0x0FFFFFFF);
        grpmask = (char)(buffer32[pnt+1] & 0xFF);
        pnt += 4;
        if (grpmask == 0)
            continue;
        for(grp=0; grp<grpmax; grp++) {
            if (!(grpmask & (1<<grp)))
                continue;
            if (V1740DPP_QDC_DecodeDPPAggregate(handle, buffer32+pnt, Events[grp]+index[grp], &nevgrp))
                return CAEN_DGTZ_InvalidEvent;
            index[grp] += nevgrp;
            pnt += (buffer32[pnt] & 0x7FFFFFFF);
        }    
        if (pnt != endaggr)
            return CAEN_DGTZ_InvalidEvent;
    }
    for(grp=0; grp<grpmax; grp++)
        NumEvents[grp] = index[grp];
    return CAEN_DGTZ_Success;

}

static CAEN_DGTZ_ErrorCode V1740DPP_QDC_DecodeDPPWaveforms(int handle, CAEN_DGTZ_DPP_QDC_Event_t* event, CAEN_DGTZ_DPP_QDC_Waveforms_t *gWaveforms) {

    int i;
    uint32_t format = event->Format;
    uint32_t* WaveIn = event->gWaveforms;
    gWaveforms->Ns = (format & 0xFFF)<<3;
    gWaveforms->anlgProbe = (uint8_t)((format>>22) & 0x3);
    gWaveforms->dgtProbe1 = (uint8_t)((format>>16) & 0x7);
    gWaveforms->dgtProbe2 = (uint8_t)((format>>19) & 0x7);
    gWaveforms->dualTrace = (uint8_t)((format>>31) & 0x1);
    
    for(i=0; i<(int)(gWaveforms->Ns/2); i++) {
        gWaveforms->Trace1[i*2+1] = (uint16_t)((WaveIn[i]>>16) & 0xFFF);
        if (gWaveforms->dualTrace){
            gWaveforms->Trace1[i*2] = gWaveforms->Trace1[i*2+1];
            gWaveforms->Trace2[i*2] = (uint16_t) WaveIn[i] & 0xFFF;
            gWaveforms->Trace2[i*2+1] = gWaveforms->Trace2[i*2];
        }
        else{    
            gWaveforms->Trace1[i*2] = (uint16_t)(WaveIn[i] & 0xFFF);
            gWaveforms->Trace2[i*2] = 0;
            gWaveforms->Trace2[i*2+1] = 0;
        }
        
        gWaveforms->DTrace1[i*2] = (uint8_t)((WaveIn[i]>>12) & 1);
        gWaveforms->DTrace1[i*2+1] = (uint8_t)((WaveIn[i]>>28) & 1);
        gWaveforms->DTrace2[i*2] = (uint8_t)((WaveIn[i]>>13) & 1);
        gWaveforms->DTrace2[i*2+1] = (uint8_t)((WaveIn[i]>>29) & 1);
        gWaveforms->DTrace3[i*2] = (uint8_t)((WaveIn[i]>>14) & 1);
        gWaveforms->DTrace3[i*2+1] = (uint8_t)((WaveIn[i]>>30) & 1);
        gWaveforms->DTrace4[i*2] = (uint8_t)((WaveIn[i]>>15) & 1);
        gWaveforms->DTrace4[i*2+1] = (uint8_t)((WaveIn[i]>>31) & 1);
        
    }    
    return CAEN_DGTZ_Success;

}
/* END inlined patch from CAEN */

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

CAEN_DGTZ_ErrorCode CAENDGTZ_API _CAEN_DGTZ_MallocDPPWaveforms(int handle, void **waveforms, uint32_t *allocatedSize)
{
    QDC_FUNCTION(MallocDPPWaveforms,handle,waveforms,allocatedSize)
}

CAEN_DGTZ_ErrorCode CAENDGTZ_API _CAEN_DGTZ_FreeDPPWaveforms(int handle, void *waveforms)
{
    QDC_FUNCTION(FreeDPPWaveforms,handle,waveforms)
}

CAEN_DGTZ_ErrorCode CAENDGTZ_API _CAEN_DGTZ_GetDPPEvents(int handle, char *buffer, uint32_t buffsize, void** events, uint32_t numEvents[])
{
    QDC_FUNCTION(GetDPPEvents,handle,buffer,buffsize,events,numEvents)
}

CAEN_DGTZ_ErrorCode CAENDGTZ_API _CAEN_DGTZ_DecodeDPPWaveforms(int handle, void *event, void *waveforms)
{
    QDC_FUNCTION(DecodeDPPWaveforms,handle,event,waveforms)
}

CAEN_DGTZ_ErrorCode CAENDGTZ_API _CAEN_DGTZ_SetChannelGroupMask(int handle, uint32_t group, uint32_t channelmask)
{
    QDC_FUNCTION(SetChannelGroupMask,handle,group,channelmask)
}

CAEN_DGTZ_ErrorCode CAENDGTZ_API _CAEN_DGTZ_GetChannelGroupMask(int handle, uint32_t group, uint32_t *channelmask)
{
    QDC_FUNCTION(GetChannelGroupMask,handle,group,channelmask)
}

CAEN_DGTZ_ErrorCode CAENDGTZ_API _CAEN_DGTZ_SetChannelTriggerThreshold(int handle, uint32_t channel, uint32_t Tvalue)
{
    QDC_FUNCTION(SetChannelTriggerThreshold,handle,channel,Tvalue)
}

CAEN_DGTZ_ErrorCode CAENDGTZ_API _CAEN_DGTZ_GetChannelTriggerThreshold(int handle, uint32_t channel, uint32_t* Tvalue)
{
    QDC_FUNCTION(GetChannelTriggerThreshold,handle,channel,Tvalue)
}

CAEN_DGTZ_ErrorCode CAENDGTZ_API _CAEN_DGTZ_GetNumEventsPerAggregate(int handle, uint32_t *numEvents, int channel)
{
    QDC_FUNCTION(GetNumEventsPerAggregate,handle,numEvents,channel)
}

CAEN_DGTZ_ErrorCode CAENDGTZ_API _CAEN_DGTZ_SetNumEventsPerAggregate(int handle, uint32_t numEvents, int channel)
{
    QDC_FUNCTION(SetNumEventsPerAggregate, handle, numEvents, channel)
}
