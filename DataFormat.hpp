/**
 * jadaq (Just Another DAQ)
 * Copyright (C) 2018  Troels Blum <troels@blum.dk>
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
 */

#ifndef JADAQ_DATAFORMAT_HPP
#define JADAQ_DATAFORMAT_HPP

#include <chrono>
#include <cstdint>
#include <cstddef>
#include <iostream>

/* A simple helper to get current time since epoch in milliseconds */
#define getTimeMsecs() (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count())

namespace Data {
    enum ElementType: uint16_t
    {
        List422,
        Waweform
    };
    /* Shared meta data for the entire data package */
    struct Header { // 24 bytes
        uint64_t runID;
        uint64_t globalTime;
        uint16_t digitizerID;
        uint16_t version;
        uint16_t elementType;
        uint16_t numElements;

    };
    struct ListElement422
    {
        uint32_t localTime;
        uint16_t adcValue;
        uint16_t channel;
    };
} // namespace Data
#endif //JADAQ_DATAFORMAT_HPP
