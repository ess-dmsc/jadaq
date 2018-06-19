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

#ifndef JADAQ_EVENTITERATOR_HPP
#define JADAQ_EVENTITERATOR_HPP

#include <iterator>
#include "caen.hpp"
#include "DataFormat.hpp"

template <typename T>
class EventIterator : public std::iterator<std::input_iterator_tag , T > {};

template <typename T>
class DPPQDCEventIterator<T> : public EventIterator<T>
{
private:
    caen::ReadoutBuffer buffer;
    uint32_t* ptr;
    size_t size;
    uint8_t groupMask;
    template <typename T>
    class AggregateIterator<T> : public EventIterator<T>
    {
    private:
        uint32_t* ptr;
        size_t elementSize = 0 ;
        uint16_t group = 0;
        bool waveform = false;
        bool extras = false;
    public:
        AggregateIterator(void* p) : ptr((uint32_t*)p) {}
        AggregateIterator(uint32_t* p, uint16_t g)
                : ptr(p)
                , elementSize(2)
                , group(g<<3)
        {
            uint32_t size = *ptr++;
            uint32_t format = *ptr++;
            assert (((format>>30) & 1) == 0);
            assert (((format>>29) & 1) == 0);
            extras = ((format>>28) & 1) == 1;
            waveform = ((format>>27) & 1) == 1;
            if (extras)
                elementSize += 1;
            if (waveform)
                elementSize += (format & 0xFFF) << 2;
            assert((size - 2) % elementSize == 0);
        }
        AggregateIterator& operator++()
        {
            ptr += elementSize;
            return *this;
        }
        AggregateIterator operator++(int)
        {
            AggregateIterator tmp(*this);
            ptr += elementSize;
            return tmp;
        }
        bool operator==(const AggregateIterator& other) const
        { return ptr == other.ptr; }
        bool operator!=(const AggregateIterator& other) const
        { return !(ptr == other.ptr); }
        T operator*() const;
    };
    template <>
    inline Data::ListElement422 AggregateIterator<Data::ListElement422>::operator*() const
    {
        Data::ListElement422 res;
        res.localTime = ptr[0];
        uint32_t data = ptr[elementSize-1];
        res.adcValue  = (uint16_t)(data & 0x0000FFFF);
        res.channel   = (uint16_t)((data >> 28) & 0xF) | group;
        return res;
    }
    AggregateIterator<T> aggregateIterator;
    AggregateIterator<T> nextAggregateIterator()
    {
        while (groupMask == 0) // New Board Aggregate
        {
            size = (*ptr++) & 0x0FFFFFFF;
            groupMask = (uint8_t)(*ptr++ & 0xFF);
            ptr+=2;
            if ((char*)ptr >= buffer.end())
            {
                return AggregateIterator<T>(buffer.end());
            }
        }
        uint16_t group = 0;
        // You are here - Finding the next group
        // Consider end of aggregate
        AggregateIterator<T> res(ptr,0);
        return res;
    }
public:
    DPPQDCEventIterator(caen::ReadoutBuffer b)
            : buffer(b)
            , ptr((uint32_t*)buffer.data)
            , groupMask(0)
            , aggregateIterator(nextAggregateIterator())
    {
        size = (*ptr++) & 0x0FFFFFFF;
        groupMask = (uint8_t)(*ptr++ & 0xFF);
        ptr+=2;
        aggregateIterator(ptr,);
    }
    DPPQDCEventIterator& operator++()
    {
        ptr += elementSize;
        return *this;
    }

};

#endif //JADAQ_EVENTITERATOR_HPP
