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

void DPPQCDEventWaveform::waveform(Waveform& waveform) const
{
    size_t n = (size-(2+extras))<<1;
    uint16_t trigger = 0xFFFF;
    Interval gate = {0xffff,0xffff};
    Interval holdoff  = {0xffff,0xffff};
    Interval over = {0xffff,0xffff};
    for (uint16_t i = 0; i < (n>>1); ++i)
    {
        uint32_t ss = ptr[i+1];
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


