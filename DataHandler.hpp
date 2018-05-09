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
public:
    virtual size_t handle(const DPPEventLE422Accessor& accessor, uint32_t digitizerID) = 0;
    static uint64_t getTimeMsecs() { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count(); }
    static constexpr const char* defaultDataPort = "12345";
    static constexpr const size_t maxBufferSize = 9000;

    template <typename C>
    struct ContainerPair
    {
        C* current;
        C* next;
        ContainerPair()
        {
            current = new C();
            next = new C();
        }
        ~ContainerPair()
        {
            delete[] current;
            delete[] next;
        }
    };
private:
    uint64_t prevMaxLocalTime = 0;
    DataHandler() {}
protected:
    uuid runID;
    uint64_t globalTimeStamp = 0;
    DataHandler(uuid runID_) : runID(runID_) {}
    template <typename C, typename F>
    size_t handle_(const DPPEventLE422Accessor& accessor, ContainerPair<C>* buffers, F write)
    {
        uint64_t currentMaxLocalTime = 0;
        uint64_t nextMaxLocalTime = 0;
        size_t events = 0;
        C* current = buffers->current;
        C* next = buffers->next;
        for (uint16_t channel = 0; channel < accessor.channels(); channel++)
        {
            for (uint32_t i = 0; i < accessor.events(channel); ++i)
            {
                events += 1;
                Data::ListElement422 listElement = accessor.listElement422(channel,i);
                if (listElement.localTime > prevMaxLocalTime)
                {
                    if (listElement.localTime > currentMaxLocalTime)
                        currentMaxLocalTime = listElement.localTime;
                    try {
                        current->push_back(listElement);
                    } catch (std::length_error&)
                    {
                        write(current);
                        current->clear();
                        current->push_back(listElement);
                    }
                } else {
                    if (listElement.localTime > nextMaxLocalTime)
                        nextMaxLocalTime = listElement.localTime;
                    try {
                        next->push_back(listElement);
                    } catch (std::length_error&)
                    {
                        write(next);
                        next->clear();
                        next->push_back(listElement);
                    }
                }
            }
        }
        if (!next->empty())
        {
            write(current);
            current->clear();
            buffers->current = next;
            buffers->next = current;
            globalTimeStamp = getTimeMsecs(); //TODO can we get this earlier and what is the cost
            currentMaxLocalTime = nextMaxLocalTime;
        }
        // NOTE: We assume that the smallest time in the next acquisition must be larger than the largest time in the
        //       current acquisition UNLESS there has been a clock reset
        prevMaxLocalTime = currentMaxLocalTime;
        return events;
    }
};

/* A simple helper to get current time since epoch in milliseconds */

#endif //JADAQ_DATAHANDLER_HPP
