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
#include <limits>
#include "caen.hpp"
#include "Waveform.hpp"

struct Event
{
    uint32_t* ptr;
    size_t size;
    Event(uint32_t* p, size_t s): ptr(p), size(s) {}
};

struct DPPQCDEvent: Event
{
    DPPQCDEvent(uint32_t* p, size_t s): Event(p,s) {}
    uint32_t timeTag() const { return ptr[0]; }
    uint16_t charge() const { return (uint16_t)(ptr[size-1] & 0x0000ffffu); }
    uint8_t subChannel() const {return (uint8_t)(ptr[size-1] >> 28);}
    uint16_t channel(uint16_t group) const { return (group<<3) | subChannel(); }
};

struct DPPQCDEventExtra: DPPQCDEvent
{
    DPPQCDEventExtra(uint32_t* p, size_t s): DPPQCDEvent(p,s) {}
    uint16_t extendedTimeTag() const { return (uint16_t)(ptr[size-2] & 0x0000ffffu); }
    uint16_t baseline() const { return (uint16_t)(ptr[size-2]>>16); }
    uint64_t fullTime() const { return ((uint64_t)timeTag()) | (((uint64_t)extendedTimeTag())<<32); }
};

#define DVP(V,S) {                              \
    uint32_t v = (ss & (0x10001000u<<(S)));     \
    if ((V).start == 0xffffu)                   \
    {                                           \
        if (v)                                  \
        {                                       \
            (V).start = (i<<1) | (v>>(28+(S))); \
            if (v == 0x1000u<<(S))              \
                (V).end = (i<<1)|1;             \
        }                                       \
    } else {                                    \
        if (v<(0x10001000u<<(S)))               \
            (V).end = (i<<1) | (v>>(28+(S)));   \
    }                                           \
}

struct DPPQCDEventWaveform: DPPQCDEvent
{
    bool extras = false;
    DPPQCDEventWaveform(uint32_t* p, size_t s): DPPQCDEvent(p,s) {}
    void waveform(Waveform& waveform) const
    {
        size_t n = (size-(2+extras))<<1;
        uint16_t trigger = 0xFFFF;
        Interval gate = {0xffff,0xffff};
        Interval holdoff  = {0xffff,0xffff};
        Interval over = {0xffff,0xffff};
        for (uint16_t i = 0; i < (n>>1); ++i)
        {
            uint32_t ss = ptr[i+1];
          waveform.samples[i<<1] = (uint16_t)(ss & 0x0fff);
          waveform.samples[i<<1|1] = (uint16_t)((ss>>16) & 0x0fff);
            // trigger
            if (uint32_t t = (ss & 0x20002000))
            {
                trigger = (i<<1) | (t>>29);
            }
            DVP(gate,0)
            DVP(holdoff,2)
            DVP(over,3)
        }
        waveform.trigger = trigger;
        waveform.gate = gate;
        waveform.holdoff = holdoff;
        waveform.overthreshold = over;
    }
};


/*
 * DPPQDCEventIterator will iterate over a set of Board Aggregates contained in one Data Block
 */
class DPPQDCEventIterator
{
private:
    const caen::ReadoutBuffer& buffer;
    uint32_t* ptr;
    uint32_t* boardAggregateEnd;
    /*
     * Group Iterator will iterate over a set of Group Aggregates contained in one Board Aggregate
     */
    class GroupIterator
    {
    private:
        uint32_t* ptr;
        uint32_t* end;
        uint8_t groupMask = 0;
        size_t elementSize = 0 ;
        int group = -1;
        bool waveform = false;
        bool extras = false;
        static const uint channelsPerGroup = 8;
    public:
        GroupIterator(void* p) : ptr((uint32_t*)p) {}
        GroupIterator(uint32_t* p, uint16_t gm)
                : ptr(p)
                , groupMask(gm)
                , elementSize(2)
        { nextGroup(); }
        void nextGroup()
        {
            while (!(groupMask & (1<<++group)))
            {
                if (group == sizeof(groupMask)*CHAR_BIT)
                {
                    return; // End of Board Aggregate
                }
            }
            assert(((ptr[0] >> 31) & 1) == 1);
            uint32_t size = ptr[0] & 0x7fffffff;
            uint32_t format = ptr[1];
            assert (((format>>31) & 1) == 0);
            assert (((format>>30) & 1) == 1);
            assert (((format>>29) & 1) == 1);
            extras = ((format>>28) & 1) == 1;
            waveform = ((format>>27) & 1) == 1;
            end = ptr+size;
            ptr += 2; //point to first event
            if (extras)
                elementSize += 1;
            if (waveform)
                elementSize += (format & 0xFFF) << 2;
            assert((size - 2) % elementSize == 0);

        }
        uint16_t currentGroup()
        {
            assert (group >= 0);
            return (uint16_t)group;
        }
        GroupIterator& operator++()
        {
            ptr += elementSize;
            if (ptr == end)
            {
                nextGroup();
            }
            assert(ptr <= end);
            return *this;
        }
        GroupIterator operator++(int)
        {
            GroupIterator tmp(*this);
            ++*this;
            return tmp;
        }
        bool operator==(const GroupIterator& other) const { return ptr == other.ptr; }
        bool operator!=(const GroupIterator& other) const { return (ptr != other.ptr); }
        bool operator>(const GroupIterator& other) const { return ptr > other.ptr; }
        bool operator<(const GroupIterator& other) const { return ptr < other.ptr; }
        bool operator>=(const GroupIterator& other) const { return ptr >= other.ptr; }
        bool operator<=(const GroupIterator& other) const { return ptr <= other.ptr; }
        bool operator==(const void* other) const { return ptr == other; }
        bool operator!=(const void* other) const { return ptr != other; }
        bool operator>(const void* other) const { return ptr > other; }
        bool operator<(const void* other) const { return ptr < other; }
        bool operator>=(const void* other) const { return ptr >= other; }
        bool operator<=(const void* other) const { return ptr <= other; }
        DPPQCDEvent operator*() const { return DPPQCDEvent{ptr, elementSize}; }
    };
    GroupIterator groupIterator;
    GroupIterator nextGroupIterator()
    {
        if ((char*)ptr < buffer.end())
        {
            size_t size = (ptr[0] & 0x0fffffff);
            assert((ptr[0] & 0xf0000000) == 0xa0000000); // Magic value
            boardAggregateEnd = ptr + size;
            uint8_t groupMask = (uint8_t) (ptr[1] & 0xFF);
            ptr += 4; // point to first group aggregate
            return GroupIterator(ptr, groupMask);
        } else
        {
            return GroupIterator(buffer.end());
        }
    }
public:
    DPPQDCEventIterator(const caen::ReadoutBuffer& b)
            : buffer(b)
            , ptr((uint32_t*)buffer.data)
            , groupIterator(nextGroupIterator()) {}
    DPPQDCEventIterator& operator++()
    {
        if (++groupIterator == boardAggregateEnd)
        {
            ptr = boardAggregateEnd;
            groupIterator = nextGroupIterator();
        }
        return *this;
    }
    DPPQDCEventIterator operator++(int)
    {
        DPPQDCEventIterator tmp(*this);
        ++*this;
        return tmp;
    }
    bool operator==(const DPPQDCEventIterator& other) const { return groupIterator == other.groupIterator; }
    bool operator!=(const DPPQDCEventIterator& other) const { return groupIterator != other.groupIterator; }
    bool operator>(const DPPQDCEventIterator& other) const { return groupIterator > other.groupIterator; }
    bool operator<(const DPPQDCEventIterator& other) const { return groupIterator < other.groupIterator; }
    bool operator>=(const DPPQDCEventIterator& other) const { return groupIterator >= other.groupIterator; }
    bool operator<=(const DPPQDCEventIterator& other) const { return groupIterator <= other.groupIterator; }
    bool operator==(const void* other) const { return groupIterator == other; }
    bool operator!=(const void* other) const { return groupIterator != other; }
    bool operator>(const void* other) const { return groupIterator > other; }
    bool operator<(const void* other) const { return groupIterator < other; }
    bool operator>=(const void* other) const { return groupIterator >= other; }
    bool operator<=(const void* other) const { return groupIterator <= other; }
    DPPQCDEvent operator*() const { return *groupIterator; }
    void* end() const { return buffer.end(); }
    uint16_t group() { return groupIterator.currentGroup(); }
};

#endif //JADAQ_EVENTITERATOR_HPP
