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


#ifndef JADAQ_CAENDIGITIZER_H
#define JADAQ_CAENDIGITIZER_H

#include <CAENDigitizer.h>


#define V1740_DPP_QDC_CODE    (0x87)
#define CAEN_DGTZ_DPPFirmware_QDC (CAEN_DGTZ_DPPFirmware_ZLE+1)
#define MAX_V1740_DPP_GROUP_SIZE   (8)                         // Number of groups - poorly named
#define V1740_MAX_CHANNELS                        64
#define V1740_MAX_EVENT_QUEUE_DEPTH                   512


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
} CAEN_DGTZ_DPP_QDC_Waveforms_t;

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
} CAEN_DGTZ_DPP_QDC_Event_t;


CAEN_DGTZ_ErrorCode CAENDGTZ_API _CAEN_DGTZ_GetDPPFirmwareType(int handle, CAEN_DGTZ_DPPFirmware_t* firmware);
CAEN_DGTZ_ErrorCode CAENDGTZ_API _CAEN_DGTZ_MallocDPPEvents(int handle, void **events, uint32_t *allocatedSize);

#endif //JADAQ_CAENDIGITIZER_H
