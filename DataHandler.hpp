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
#include "EventIterator.hpp"
#include "container.hpp"
#include "DataWriter.hpp"

class DataHandler
{
public:
    template<typename E, template<typename...> typename C>
    void initialize(DataWriter& dataWriter, uint32_t digitizerID, size_t groups)
    {
        instance.reset(new Implementation<E,C>(dataWriter,digitizerID,groups));
    }
    void flush() { instance->flush(); }
    size_t operator()(DPPQDCEventIterator& it) { return instance->operator()(it); }
    static int64_t getTimeMsecs()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
    }
private:
    struct Interface
    {
        virtual ~Interface() = default;
        virtual size_t operator()(DPPQDCEventIterator& it) = 0;
        virtual void flush() = 0;
    };
    /* E is element type e.g. Data::ListElementxxx
     * C is containertype i.e. jadaq::vector, jadaq::set, jadaq::buffer
    */
    template <typename E, template<typename...> typename C>
    class Implementation: public Interface
    {
        static_assert(std::is_pod<E>::value, "E must be POD");
    private:
        DataWriter& dataWriter;
        uint32_t digitizerID;
        struct Buffer
        {
            size_t groups;
            C<E>* buffer;
            uint32_t* maxLocalTime; // Array containing MaxLocalTime from the previous insertion needed to detect reset
            uint64_t globalTimeStamp = 0;
            void clear()
            {
                buffer->clear();
                for (size_t i = 0; i < groups; ++i)
                {
                    maxLocalTime[i] = 0;
                }
                globalTimeStamp = 0;
            }
            Buffer(size_t numGroups) : groups(numGroups) {}

            void malloc()
            {
                buffer = new C<E>();
                maxLocalTime = new uint32_t[groups];
                clear();
            }
            void free()
            {
                delete buffer;
                delete[] maxLocalTime;
            }

        } current, next;
    public:
        Implementation(DataWriter& dw, uint32_t digID, size_t groups)
                : dataWriter(dw)
                , digitizerID(digID)
                , current(groups)
                , next(groups)
        {
            current.malloc();
            next.malloc();
        }
        ~Implementation()
        {
            flush();
            current.free();
            next.free();
        }
        size_t operator()(DPPQDCEventIterator& eventIterator)
        {
            size_t events = 0;
            for (;eventIterator != eventIterator.end(); ++eventIterator)
            {
                events += 1;
                DPPQCDEvent event = *eventIterator;
                uint32_t timeTag = event.timeTag();
                uint16_t group = eventIterator.group();

                if (timeTag > current.maxLocalTime[group])
                {
                    current.maxLocalTime[group] = timeTag;
                    try {
                        current.buffer->emplace(event,group);
                    } catch (std::length_error&)
                    {
                        dataWriter(current.buffer,digitizerID,current.globalTimeStamp);
                        current.buffer->clear();
                        current.buffer->emplace(event,group);
                    }
                } else {
                    next.maxLocalTime[group] = timeTag;
                    if (next.globalTimeStamp == 0)
                    {
                        next.globalTimeStamp = DataHandler::getTimeMsecs();
                    }
                    try {
                        next.buffer->emplace(event,group);
                    } catch (std::length_error&)
                    {
                        dataWriter(next.buffer,digitizerID,next.globalTimeStamp);
                        next.buffer->clear();
                        next.buffer->emplace(event,group);
                    }
                }
            }
            if (!next.buffer->empty())
            {
                if (current.buffer->size() > 0)
                {
                    dataWriter(current.buffer, digitizerID, current.globalTimeStamp);
                }
                current.clear();
                std::swap(current,next);
            }
            return events;
        }
        void flush()
        {
            if (current.buffer->size() > 0)
            {
                dataWriter(current.buffer, digitizerID, current.globalTimeStamp);
                current.clear();
            }
            if (next.buffer->size() > 0)
            {
                dataWriter(next.buffer, digitizerID, next.globalTimeStamp);
                next.clear();
            }
        }
    };
    std::unique_ptr<Interface> instance;
};

#endif //JADAQ_DATAHANDLER_HPP
