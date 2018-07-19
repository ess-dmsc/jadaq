/**
 * jadaq (Just Another DAQ)
 * Copyright (C) 2017  Troels Blum <troels@blum.dk>
 * Contributions from  Jonas Bardino <bardino@nbi.ku.dk>
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
 *
 */

#include <cassert>
#include <bitset>
#include "DPPQCDEvent.hpp"
#include "Waveform.hpp"

#define DVP(V,S) {                              \
    uint32_t v = (ss & (0x10001000u<<(S)));     \
    if ((V).start == 0xffffu)                   \
    {                                           \
        if (v)                                  \
        {                                       \
            (V).start = (i<<1) | (v>>(28+(S))); \
            if (v == 0x1000u<<(S))              \
                (V).end = (i<<1)|1;             \
        }                                       \
    } else {                                    \
        if (v<(0x10001000u<<(S)))               \
            (V).end = (i<<1) | (v>>(28+(S)));   \
    }                                           \
}

/** DPP QDC on XX740 digitizer mixed-mode waveform decoding */
template <typename DPPQDCEventType>
static inline void waveform_(const DPPQDCEventWaveform<DPPQDCEventType>& event, DPPQDCWaveform& waveform)
{
    size_t n = (event.size-(2+event.extras))<<1;
    uint16_t trigger = 0xFFFF;
    Interval gate = {0xffff,0xffff};
    Interval holdoff  = {0xffff,0xffff};
    Interval over = {0xffff,0xffff};
    for (uint16_t i = 0; i < (n>>1); ++i)
    {
        uint32_t ss = event.ptr[i+1];
        waveform.samples[i<<1] = (uint16_t)(ss & 0x0fff);
        waveform.samples[i<<1|1] = (uint16_t)((ss>>16) & 0x0fff);
        // trigger
        if (uint32_t t = (ss & 0x20002000))
        {
            trigger = (i<<1) | (t>>29);
        }
        DVP(gate,0)
        DVP(holdoff,2)
        DVP(over,3)
    }
    waveform.num_samples = n;
    waveform.trigger = trigger;
    waveform.gate = gate;
    waveform.holdoff = holdoff;
    waveform.overthreshold = over;
}

template <>
void DPPQDCEventWaveform<DPPQDCEvent>::waveform(DPPQDCWaveform &waveform) const
{
    waveform_(*this,waveform);
}

template <>
void DPPQDCEventWaveform<DPPQDCEventExtra>::waveform(DPPQDCWaveform &waveform) const
{
    waveform_(*this,waveform);
}

template <>
void StdEventWaveform<StdEvent751>::waveform(StdWaveform &waveform) const
{
  //  waveform_(*this,waveform);
  size_t nActiveChannel = std::bitset<8>(channelMask()).count(); // # of active channels given by mask
  size_t nWords = (size - 4); // number of words with samples: (event size - header)
  assert((nWords % nActiveChannel) == 0); // double-check that total size adds up

  // determine the number of trailing samples in each channel data block from last word of first channel data block:
  // NOTE: assumed to be the same for all channels
  // uint8_t nTrailingSamples = (uint8_t)((event.prt[4+((uint16_t) nWords/nActiveChannel) - 1] >> 30) & 0x0002 );

  uint16_t idx = 0;

  for (uint16_t i = 0; i < (nWords); ++i)
    {
      uint32_t ss = ptr[i+4]; // current word after header
      // current sample index calculated from 3 samples per word minus those
      // possibly missing in the trailing word of each channel block
      // uint16_t idx = i * 3 - ((uint16_t) i / (nWords / nActiveChannel))*( 3 - nTrailingSamples );

      uint8_t nSamples = (uint8_t)((ss >> 30) & 0x03 ); // # of samples in this word, max three (2-bit value)

      for (uint8_t s = 0; s < nSamples; ++s){
        waveform.samples[idx++] = (uint16_t)((ss>>(s*10)) & 0x03ff); // 10-bit samples
      }
    }
  waveform.num_samples = idx;
}
