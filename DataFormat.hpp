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
 * Data format for packages and HDF5 file dumps.
 *
 */

#ifndef MULTIBLADEDATAHANDLER_DATAFORMAT_HPP
#define MULTIBLADEDATAHANDLER_DATAFORMAT_HPP

#include <cstdint>
#include <cstddef>

/* Every Data set contains meta data with the digitizer and format
 * followed by the actual List and/or waveform data points. */
namespace Data {
    /* Shared meta data for the entire data package */
    struct Meta {
        uint8_t version;
        char digitizerModel[12];
        uint8_t digitizerID;
        uint8_t pad[2];
        uint32_t globalTime;
    };
    namespace List{
        struct Element // 56 bit
        {
            uint32_t localTime;
            uint16_t adcValue;
            uint8_t channel;
        };
    }; // namespace List
    namespace Waveform{
        struct Element // variable size
        {
            uint32_t localTime;
            uint16_t adcValue;
            uint8_t channel;
            uint16_t WFlength;
            uint16_t waveform[];
        };
    }; // namespace Waveform
    /* Actual payload of elements */
    struct Buffer
    {
        void* data;
        size_t size;      // Allocated size
        size_t dataSize;  // Size of current data
    };
} // namespace Data
#endif //MULTIBLADEDATAHANDLER_DATAFORMAT_HPP
