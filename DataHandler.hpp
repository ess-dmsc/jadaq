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

template <typename E> class DataHandler;

class DataHandlerGeneric
{
public:
    virtual void addDigitizer(uint32_t digitizerID) = 0;

    template<typename E>
    size_t handle(const DPPEventAccessor<E> &accessor, uint32_t digitizerID)
    {
        return static_cast<DataHandler<E>* >(this)->handle(accessor,digitizerID);
    }

    static int64_t getTimeMsecs()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
    }

};

template <typename E>
class DataHandler : public DataHandlerGeneric
{
public:
    virtual size_t handle(const DPPEventAccessor<E>& accessor, uint32_t digitizerID) = 0;

    template <template<typename...> typename C>
    struct ContainerPair
    {
        C<E>* current;
        C<E>* next;
        ContainerPair()
        {
            current = new C<E>();
            next = new C<E>();
        }
        ~ContainerPair()
        {
            delete[] current;
            delete[] next;
        }
    };
private:
    uint64_t prevMaxLocalTime = 0;
protected:
    uuid runID;
    uint64_t globalTimeStamp = 0;
    DataHandler(uuid runID_) : runID(runID_) {}
    template <template<typename...> typename C, typename F>
    size_t handle_(const DPPEventAccessor<E>& accessor, ContainerPair<C>* buffers, F write)
    {
        uint64_t currentMaxLocalTime = 0;
        uint64_t nextMaxLocalTime = 0;
        size_t events = 0;
        C<E>* current = buffers->current;
        C<E>* next = buffers->next;
        for (uint16_t channel = 0; channel < accessor.channels(); channel++)
        {
            for (uint32_t i = 0; i < accessor.events(channel); ++i)
            {
                events += 1;
                E element = accessor.element(channel,i);
                if (element.localTime > prevMaxLocalTime)
                {
                    if (element.localTime > currentMaxLocalTime)
                        currentMaxLocalTime = element.localTime;
                    try {
                        current->insert(element);
                    } catch (std::length_error&)
                    {
                        write(current);
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
                        write(next);
                        next->clear();
                        next->insert(element);
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

namespace jadaq
{
    template<typename T>
    class vector : public std::vector<T>
    {
    public:
        void insert(const T &v)
        { this->push_back(v); }

        void insert(const T &&v)
        { this->push_back(v); }
    };
}
#endif //JADAQ_DATAHANDLER_HPP
