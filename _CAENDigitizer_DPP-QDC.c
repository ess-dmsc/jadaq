/******************************************************************************
* 
* CAEN SpA - Front End Division
* Via Vetraia, 11 - 55049 - Viareggio ITALY
* +390594388398 - www.caen.it
*
***************************************************************************/
/** PATCH of the CAENDigitizer for the DPP-QDC in the x740 models **/
#include <CAENDigitizer.h>
#include "_CAENDigitizer_DPP-QDC.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern unsigned int gEquippedGroups;
extern _CAEN_DGTZ_DPP_QDC_Event_t *gEventsGrp[8];

#define MAX(a,b) ((a) > (b) ? a : b)

int _CAEN_DGTZ_DPP_QDC_SetNumEvAggregate(int handle, int NevAggr)
{
    int ret=0;
    uint32_t d32;

    if (NevAggr == 0) {
        CAEN_DGTZ_ReadRegister(handle, 0x8000, &d32);
        if (d32 & (1<<16)) {                                          /* waveform enabled      */
            ret |= CAEN_DGTZ_WriteRegister(handle, 0x800C, 0x3);      /* Buffer organization   */
            ret |= CAEN_DGTZ_WriteRegister(handle, 0x8020, 1);        /* Events per aggregate  */
        } else {
            ret |= CAEN_DGTZ_WriteRegister(handle, 0x800C, 0xA);      /* Buffer organization   */
            ret |= CAEN_DGTZ_WriteRegister(handle, 0x8020, 16);       /* Events per aggregate  */
        }
    } else {
        ret |= CAEN_DGTZ_WriteRegister(handle, 0x800C, 0x3);          /* Buffer organization   */
        ret |= CAEN_DGTZ_WriteRegister(handle, 0x8020, NevAggr);      /* Events per aggregate  */
    }

    return ret;
}

int _CAEN_DGTZ_DPP_QDC_GetDPPEvents(int handle, char *buffer, uint32_t BufferSize, _CAEN_DGTZ_DPP_QDC_Event_t **Events, uint32_t *NumEvents)
{
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
            if (_CAEN_DGTZ_DPP_QDC_DecodeDPPAggregate(buffer32+pnt, Events[grp]+index[grp], &nevgrp))
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


int _CAEN_DGTZ_GetDPPEvents(int handle, char *buffer, uint32_t BufferSize, void **Events, uint32_t *NumEvents)
{
    unsigned int i, j, ch;
    int ret;
    uint32_t NumEventsGrp[8];

    for(ch=0; ch<64; ch++)
        NumEvents[ch] = 0;

    /* allocate memory for group events (first time only) */
    for(i=0; i<8; i++) {
        if (gEventsGrp[i] == NULL)
            if ( (gEventsGrp[i] = (_CAEN_DGTZ_DPP_QDC_Event_t *)malloc(MAX_CHANNELS*MAX_EVENT_QUEUE_DEPTH*sizeof(_CAEN_DGTZ_DPP_QDC_Event_t))) == NULL)
                return CAEN_DGTZ_OutOfMemory;
    }

    if ( (ret = _CAEN_DGTZ_DPP_QDC_GetDPPEvents(handle, buffer, BufferSize, gEventsGrp, NumEventsGrp)) != CAEN_DGTZ_Success) {
        printf("Error during _CAEN_DGTZ_DPP_QDC_GetDPPEvents() call (ret = %d): exiting ... \n", ret);
        exit(-1);
    }

    /* the for loop distributes events belongng to groups into single channel event arrays */
    for(i=0; i<gEquippedGroups; i++) {
        for(j=0; j<(int)NumEventsGrp[i]; j++) {
            ch = i*8 + gEventsGrp[i][j].SubChannel;
            if ((memcpy((_CAEN_DGTZ_DPP_QDC_Event_t *)Events[ch] + NumEvents[ch], &gEventsGrp[i][j], sizeof(_CAEN_DGTZ_DPP_QDC_Event_t))) == NULL) {
                printf("Error during memcpy in _CAEN_DGTZ_GetDPPEvents() call: exiting ...\n");
                exit(-1);
            }
            NumEvents[ch]++;
        }
    }


    return 0;
}

int _CAEN_DGTZ_DPP_QDC_DecodeDPPAggregate(uint32_t *data, _CAEN_DGTZ_DPP_QDC_Event_t *Events, int *NumEvents) 
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

int _CAEN_DGTZ_DecodeDPPWaveforms(_CAEN_DGTZ_DPP_QDC_Event_t* event, _CAEN_DGTZ_DPP_QDC_Waveforms_t *gWaveforms) {

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
