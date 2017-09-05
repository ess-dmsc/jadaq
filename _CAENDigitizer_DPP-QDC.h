/******************************************************************************
* 
* CAEN SpA - Front End Division
* Via Vetraia, 11 - 55049 - Viareggio ITALY
* +390594388398 - www.caen.it
*
******************************************************************************/

/** @file _CAENDigitizer_DPP-QDC.h
 * @brief DQQ-QDC functions specific for the CAEN x740 family of digitizers boards
 *
 * @author Luca Colombini <l.colombini@caen.it>
 */

/*
** Library function for x740 DPP-QDC firmware
*/

#ifndef _CAENDIGITIZER_DPP_QDC_H
#define _CAENDIGITIZER_DPP_QDC_H

#include <CAENDigitizer.h>

#define MAX_CHANNELS 64
#define MAX_GROUPS    8

#define V1740_MAX_CHANNELS                        64
#define MAX_EVENT_QUEUE_DEPTH                   512
#define MAX_ALLOCATED_MEM_PER_GROUP    (18*1024*1024)

typedef struct
{
    uint32_t Ns;
    uint8_t  dualTrace;
    uint8_t  anlgProbe;
    uint8_t  dgtProbe1;
    uint8_t  dgtProbe2;
    uint16_t *Trace1;
    uint16_t *Trace2;
    uint8_t  *DTrace1;
    uint8_t  *DTrace2;
    uint8_t  *DTrace3;
    uint8_t  *DTrace4;
} _CAEN_DGTZ_DPP_QDC_Waveforms_t;

typedef struct 
{
  uint8_t  isExtendedTimeStamp;
  uint32_t Format;
  uint32_t TimeTag;
  uint16_t Charge;
  int16_t  Baseline;
  uint16_t Pur;
  uint16_t Overrange;
  uint32_t Extras;
  uint32_t *gWaveforms;
  uint16_t SubChannel;
} _CAEN_DGTZ_DPP_QDC_Event_t;

static int _CAEN_DGTZ_DPP_QDC_GetRecordLength(int handle, uint32_t* recordLength, int ch);
static int _CAEN_DGTZ_DPP_QDC_GetDPPEvents(int handle, char *buffer, uint32_t bsize, _CAEN_DGTZ_DPP_QDC_Event_t **Events, uint32_t *numEvents);
static int _CAEN_DGTZ_DPP_QDC_DecodeDPPAggregate(uint32_t *data, _CAEN_DGTZ_DPP_QDC_Event_t *Events, int *NumEvents);
int        _CAEN_DGTZ_DPP_QDC_SetNumEvAggregate(int handle, int NevAggr);

/* Function prototypes */
int _CAEN_DGTZ_SetChannelTriggerThreshold(int handle, uint32_t channel, uint32_t Tvalue);
int _CAEN_DGTZ_SetRecordLength(int handle, uint32_t RecordLength);
int _CAEN_DGTZ_GetDPPEvents(int handle, char *buffer, uint32_t BufferSize, void **Events, uint32_t *NumEvents);
int _CAEN_DGTZ_DecodeDPPWaveforms(_CAEN_DGTZ_DPP_QDC_Event_t* event, _CAEN_DGTZ_DPP_QDC_Waveforms_t *gWaveforms);
int _CAEN_DGTZ_MallocDPPWaveforms(int handle, _CAEN_DGTZ_DPP_QDC_Waveforms_t **gWaveforms, uint32_t *AllocatedSize);
int _CAEN_DGTZ_FreeReadoutBuffer(char **buffer);
int _CAEN_DGTZ_MallocReadoutBuffer(int handle, char **buffer, uint32_t *size);
int _CAEN_DGTZ_SetChannelGroupMask(int handle, uint32_t group, uint32_t channelmask);


#endif