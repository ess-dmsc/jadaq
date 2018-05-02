/**
 * jadaq (Just Another DAQ)
 * Copyright (C) 2018  Troels Blum <troels@blum.dk>
 *
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
 * Interface for the data handlers
 *
 */

#ifndef JADAQ_DATAHANDLER_HPP
#define JADAQ_DATAHANDLER_HPP

#include "DataFormat.hpp"
#include "uuid.hpp"
#include "EventAccessor.hpp"

class DataHandler
{
private:
    DataHandler() {}
protected:
    uuid runID;
    DataHandler(uuid runID_) : runID(runID_) {}
public:
    virtual size_t handle(DPPEventLE422Accessor& accessor, uint32_t digitizerID) = 0;
    static uint64_t getTimeMsecs() { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() ; }
    static constexpr const char* defaultDataPort = "12345";
    static constexpr const size_t maxBufferSize = 9000;
};

/* A simple helper to get current time since epoch in milliseconds */

#endif //JADAQ_DATAHANDLER_HPP
