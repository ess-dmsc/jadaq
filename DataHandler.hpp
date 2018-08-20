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
    template<typename E>
    void initialize(DataWriter& dataWriter, uint32_t digitizerID, size_t groups, size_t samples, const uint32_t* maxJitter)
    {
        instance.reset(new Implementation<E>(dataWriter,digitizerID,groups,samples,maxJitter));
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
    template <typename E>
    class Implementation: public Interface
    {
        static_assert(std::is_pod<E>::value, "E must be POD");
    private:
        DataWriter& dataWriter;
        uint32_t digitizerID;
        const uint32_t* maxJitter;

        struct Buffer
        {
            size_t groups;
            jadaq::buffer<E>* buffer;
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

            void malloc(DataWriter& dataWriter, size_t samples, bool network)
            {
                if (network)
                    buffer = new jadaq::buffer<E>(Data::maxBufferSize,E::size(samples), sizeof(Data::Header));
                else
                    buffer = new jadaq::buffer<E>(4096*E::size(samples),E::size(samples));
                maxLocalTime = new uint32_t[groups];
                clear();
            }
            void free()
            {
                delete buffer;
                delete[] maxLocalTime;
            }

        } previous, current, next;

        void inline store(Buffer& buffer, typename E::EventType& event, uint16_t group)
        {
            buffer.maxLocalTime[group] = event.timeTag();
            try {
                buffer.buffer->emplace_back(event,group);
            } catch (std::length_error&)
            {
                dataWriter(buffer.buffer, digitizerID, buffer.globalTimeStamp);
                buffer.buffer->clear();
                buffer.buffer->emplace_back(event,group);
            }
        }

    public:
        Implementation(DataWriter& dw, uint32_t digID, size_t groups, size_t samples, const uint32_t* jitter)
                : dataWriter(dw)
                , digitizerID(digID)
                , maxJitter(jitter)
                , previous(groups)
                , current(groups)
                , next(groups)
        {
            previous.malloc(dataWriter, samples, dataWriter.network());
            current.malloc(dataWriter, samples, dataWriter.network());
            next.malloc(dataWriter, samples, dataWriter.network());
        }
        ~Implementation()
        {
            flush();
            previous.free();
            current.free();
            next.free();
        }

        size_t operator()(DPPQDCEventIterator& eventIterator)
        {
            size_t events = 0;
            for (;eventIterator != eventIterator.end(); ++eventIterator)
            {
                events += 1;
                typename E::EventType event = eventIterator.event<typename E::EventType>();
                uint16_t group = eventIterator.group();
                if (current.maxLocalTime[group] < event.timeTag() + maxJitter[group])
                {
                    if (current.maxLocalTime[group] > 0
                        || previous.maxLocalTime[group] == 0
                        || previous.maxLocalTime[group] >= event.timeTag() + maxJitter[group])
                    {
                        store(current, event, group);
                    } else
                    {
                        store(previous, event, group);
                    }
                } else {
                    if (next.globalTimeStamp == 0)
                    {
                        next.globalTimeStamp = DataHandler::getTimeMsecs();
                    }
                    store(next,event,group);
                }
            }
            if (!next.buffer->empty())
            {
                if (previous.buffer->size() > 0)
                {
                    dataWriter(previous.buffer, digitizerID, previous.globalTimeStamp);
                }
                previous.clear();
                std::swap(current,previous);
                std::swap(next,current);
            }
            return events;
        }
        void flush()
        {
            if (previous.buffer->size() > 0)
            {
                dataWriter(previous.buffer, digitizerID, previous.globalTimeStamp);
                previous.clear();
            }
            if (current.buffer->size() > 0)
            {
                dataWriter(current.buffer, digitizerID, current.globalTimeStamp);
                current.clear();
            }
            assert(next.buffer->size() == 0);
        }
    };
    std::unique_ptr<Interface> instance;
};

#endif //JADAQ_DATAHANDLER_HPP
