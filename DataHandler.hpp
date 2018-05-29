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

#include <functional>
#include "DataFormat.hpp"
#include "uuid.hpp"
#include "EventAccessor.hpp"
#include "container.hpp"
#include "DataWriter.hpp"


class DataHandlerGeneric
{
public:
    static int64_t getTimeMsecs()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
    }

};


template <typename E, template<typename...> typename C>
class DataHandler //: public DataHandlerGeneric
{
    static_assert(std::is_pod<E>::value, "E must be POD");
private:
    uint32_t digitizerID;
    C<E>* current; // event buffer for current global timestamp
    C<E>* next;    // event buffer for next global timestamp

    uint64_t prevMaxLocalTime = 0;//[]; // Array containing MaxLocalTime from the previous insertion needed to detect reset
    uint64_t globalTimeStamp = 0;
    DataWriter* dataWriter;
public:
    DataHandler(uint32_t digID, DataWriter* dw)
            : digitizerID(digID)
            , dataWriter(dw) {}
    size_t operator()(const DPPEventAccessor<E>& accessor)
    {
        uint64_t currentMaxLocalTime = 0;
        uint64_t nextMaxLocalTime = 0;
        size_t events = 0;
        for (uint16_t channel = 0; channel < accessor.channels(); channel++)
        {
            for (uint32_t i = 0; i < accessor.events(channel); ++i)
            {
                events += 1;
                E element = accessor(channel,i);
                if (element.localTime > prevMaxLocalTime)
                {
                    if (element.localTime > currentMaxLocalTime)
                        currentMaxLocalTime = element.localTime;
                    try {
                        current->insert(element);
                    } catch (std::length_error&)
                    {
                        (*dataWriter)(current,digitizerID,globalTimeStamp);
                        current->clear();
                        current->insert(element);
                    }
                } else {
                    if (element.localTime > nextMaxLocalTime)
                        nextMaxLocalTime = element.localTime;
                    try {
                        next->insert(element);
                    } catch (std::length_error&)
                    {
                        (*dataWriter)(next,digitizerID,globalTimeStamp);
                        next->clear();
                        next->insert(element);
                    }
                }
            }
        }
        if (!next->empty())
        {
            (*dataWriter)(current,digitizerID,globalTimeStamp);
            current->clear();
            C<E>* temp = current;
            current = next;
            next = temp;
            globalTimeStamp = DataHandlerGeneric::getTimeMsecs(); //TODO can we get this earlier and what is the cost
            currentMaxLocalTime = nextMaxLocalTime;
        }
        // NOTE: We assume that the smallest time in the next acquisition must be larger than the largest time in the
        //       current acquisition UNLESS there has been a clock reset
        prevMaxLocalTime = currentMaxLocalTime;
        return events;
    }
};
#endif //JADAQ_DATAHANDLER_HPP
