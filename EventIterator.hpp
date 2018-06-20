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

template <typename E> class DPPQDCEventIterator;

class EventIterator
{
private:
    void* ptr;

public:
    template <typename T>
    EventIterator(DPPQDCEventIterator<T>& b): ptr(&b) {}
    template<typename T>
    EventIterator& operator=(DPPQDCEventIterator<T>& b)
    {
        ptr = &b;
        return *this;
    }
    template<typename T>
    DPPQDCEventIterator<T>& base() const
    {
        return *static_cast<DPPQDCEventIterator<T>*>(ptr);
    }
};

template <typename E>
class DPPQDCEventIterator
{
private:
    caen::ReadoutBuffer buffer;
    uint32_t* ptr;
    uint32_t* aggregateEnd;
    uint8_t groupMask;

    class AggregateIterator
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
            assert (((format>>30) & 1) == 1);
            assert (((format>>29) & 1) == 1);
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
        bool operator==(const AggregateIterator& other) const { return ptr == other.ptr; }
        bool operator!=(const AggregateIterator& other) const { return (ptr != other.ptr); }
        bool operator>(const AggregateIterator& other) const { return ptr > other.ptr; }
        bool operator<(const AggregateIterator& other) const { return ptr < other.ptr; }
        bool operator>=(const AggregateIterator& other) const { return ptr >= other.ptr; }
        bool operator<=(const AggregateIterator& other) const { return ptr <= other.ptr; }
        bool operator==(const void* other) const { return ptr == other; }
        bool operator!=(const void* other) const { return ptr != other; }
        bool operator>(const void* other) const { return ptr > other; }
        bool operator<(const void* other) const { return ptr < other; }
        bool operator>=(const void* other) const { return ptr >= other; }
        bool operator<=(const void* other) const { return ptr <= other; }
        E operator*() const;
    };
    AggregateIterator aggregateIterator;
    AggregateIterator nextAggregateIterator()
    {
        while (groupMask == 0) // New Board Aggregate
        {
            size_t size = (ptr[0]) & 0x0FFFFFFF;
            aggregateEnd = ptr + size;
            groupMask = (uint8_t)(ptr[1] & 0xFF);
            ptr += 4;
            if ((char*)ptr >= buffer.end())
            {
                return AggregateIterator(buffer.end());
            }
        }
        uint16_t group;
        for (group = 0;;++group)
        {
            if (groupMask & (1<<group))
            {
                groupMask = groupMask & ~(1<<group);
                break;
            }
        }
        return AggregateIterator(ptr,group);
    }
public:
    DPPQDCEventIterator(caen::ReadoutBuffer b)
            : buffer(b)
            , ptr((uint32_t*)buffer.data)
            , groupMask(0)
            , aggregateIterator(nextAggregateIterator()) {}
    DPPQDCEventIterator& operator++()
    {
        if (++aggregateIterator >= aggregateEnd)
        {
            aggregateIterator = nextAggregateIterator();
        }
        return *this;
    }
    DPPQDCEventIterator operator++(int)
    {
        DPPQDCEventIterator tmp(*this);
        ++*this;
        return tmp;
    }
    bool operator==(const DPPQDCEventIterator& other) const { return aggregateIterator == other.aggregateIterator; }
    bool operator!=(const DPPQDCEventIterator& other) const { return aggregateIterator != other.aggregateIterator; }
    bool operator>(const DPPQDCEventIterator& other) const { return aggregateIterator > other.aggregateIterator; }
    bool operator<(const DPPQDCEventIterator& other) const { return aggregateIterator < other.aggregateIterator; }
    bool operator>=(const DPPQDCEventIterator& other) const { return aggregateIterator >= other.aggregateIterator; }
    bool operator<=(const DPPQDCEventIterator& other) const { return aggregateIterator <= other.aggregateIterator; }
    bool operator==(const void* other) const { return aggregateIterator == other; }
    bool operator!=(const void* other) const { return aggregateIterator != other; }
    bool operator>(const void* other) const { return aggregateIterator > other; }
    bool operator<(const void* other) const { return aggregateIterator < other; }
    bool operator>=(const void* other) const { return aggregateIterator >= other; }
    bool operator<=(const void* other) const { return aggregateIterator <= other; }
    E operator*() const { return *aggregateIterator; }
    void* end() const { return buffer.end(); }
};

template <>
inline Data::ListElement422 DPPQDCEventIterator<Data::ListElement422>::AggregateIterator::operator*() const
{
    Data::ListElement422 res;
    res.localTime = ptr[0];
    uint32_t data = ptr[elementSize-1];
    res.adcValue  = (uint16_t)(data & 0x0000ffffu);
    res.channel   = group | (uint16_t)(data >> 28);
    return res;
}

template <>
inline Data::ListElement8222 DPPQDCEventIterator<Data::ListElement8222>::AggregateIterator::operator*() const
{
    uint64_t time = ptr[0];
    uint32_t extra = 0;
    if (extras)
        extra = ptr[elementSize-2];
    uint32_t data = ptr[elementSize-1];
    Data::ListElement8222 res;
    res.localTime = ((uint64_t)(extra & 0x0000ffffu)<<32) | time;
    res.adcValue  = (uint16_t)(data & 0x0000ffffu);
    res.baseline  = (uint16_t)(extra>>16);
    res.channel   = group | (uint16_t)(data >> 28);
    return res;
}

#endif //JADAQ_EVENTITERATOR_HPP
