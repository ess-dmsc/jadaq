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

/** abstract base class for an iterator over the elements contained in a data block */
class DataBlockBaseIterator{
protected:
  const caen::ReadoutBuffer& buffer;
  uint32_t* ptr;
public:
  DataBlockBaseIterator(const caen::ReadoutBuffer& b)
    : buffer(b)
    , ptr((uint32_t*)buffer.data) {}
  virtual uint16_t group() { return 0; }
  virtual void* end() const { return buffer.end(); }
  virtual DataBlockBaseIterator& operator++() { return *this; }
  bool operator==(const DataBlockBaseIterator& other) const { return ptr == other.ptr; }
  bool operator!=(const DataBlockBaseIterator& other) const { return (ptr != other.ptr); }
  bool operator> (const DataBlockBaseIterator& other) const { return ptr > other.ptr; }
  bool operator< (const DataBlockBaseIterator& other) const { return ptr < other.ptr; }
  bool operator>=(const DataBlockBaseIterator& other) const { return ptr >= other.ptr; }
  bool operator<=(const DataBlockBaseIterator& other) const { return ptr <= other.ptr; }
  bool operator==(const void* other) const { return ptr == other; }
  bool operator!=(const void* other) const { return ptr != other; }
  bool operator> (const void* other) const { return ptr > other; }
  bool operator< (const void* other) const { return ptr < other; }
  bool operator>=(const void* other) const { return ptr >= other; }
  bool operator<=(const void* other) const { return ptr <= other; }

  virtual uint32_t* getEventPtr() = 0;
  virtual size_t getEventSize() = 0;
  template <typename T>
  T event() { return T{getEventPtr(), getEventSize()}; }
};




/*
 * StdBLTEventIterator will iterate over a transferred block of events as used in the std FW of XX751.
 * Format is described on p53 in UM3350 - V1751/VX1751 User Manual rev. 16
 */
class StdBLTEventIterator : public DataBlockBaseIterator
{
private:
  size_t eventSize;

public:
    StdBLTEventIterator(const caen::ReadoutBuffer& b)
      : DataBlockBaseIterator(b) { initNextEvent(); }

  void initNextEvent(){
    if ((char*)ptr < buffer.end()){
      eventSize = (ptr[0] & 0x0fffffffu);
      assert(((ptr[0] >> 28) & 0xff) == 0xA); // magic pattern
      assert((char*)(ptr+ ((uint32_t) eventSize)) <= buffer.end());
      // TODO: check for "BoardFail" flag and handle errors: (((ptr[1]>>26) & 1) == 1)
    }
  }


  StdBLTEventIterator& operator++(){
    if ((char*)ptr < buffer.end()){
      ptr += eventSize;
      initNextEvent();
    }
    return *this;
  }
  StdBLTEventIterator operator++(int){
      StdBLTEventIterator tmp(*this);
      ++*this;
      return tmp;
    }

  uint32_t* getEventPtr() { return ptr; }
  size_t getEventSize() { return eventSize; };
};


/*
 * DPPQDCEventIterator will iterate over a set of Board Aggregates contained in one Data Block
 */
  class DPPQDCEventIterator : public DataBlockBaseIterator
{
private:
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

        bool waveformFlag() const
        { return waveform; }

        bool extrasFlag() const
        { return extras; }

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
        DPPQDCEvent operator*() const { return DPPQDCEvent{ptr, elementSize}; }
        uint32_t* getEventPtr() { return ptr; }
        size_t getEventSize() { return elementSize; };
        template <typename T>
        T event() { return T{ptr, elementSize}; }
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
      : DataBlockBaseIterator(b)
      , groupIterator(nextGroupIterator()) {}

    bool waveformFlag() const
    { return groupIterator.waveformFlag(); }

    bool extrasFlag() const
    { return groupIterator.extrasFlag(); }

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
    DPPQDCEvent operator*() const { return *groupIterator; }
    void* end() const { return buffer.end(); }
    uint16_t group() { return groupIterator.currentGroup(); }
    uint32_t* getEventPtr() { return groupIterator.getEventPtr(); }
    size_t getEventSize() { return groupIterator.getEventSize(); };
};

template <>
inline DPPQDCEvent DPPQDCEventIterator::GroupIterator::event<DPPQDCEvent>()
{
    assert(extras == false);
    assert(waveform == false);
    return DPPQDCEvent{ptr, elementSize};
}

template <>
inline DPPQDCEventExtra DPPQDCEventIterator::GroupIterator::event<DPPQDCEventExtra>()
{
    assert(extras == true);
    assert(waveform == false);
    return DPPQDCEventExtra{ptr, elementSize};
}

template <>
inline DPPQDCEventWaveform<DPPQDCEvent> DPPQDCEventIterator::GroupIterator::event<DPPQDCEventWaveform<DPPQDCEvent> >()
{
    assert(extras == false);
    assert(waveform == true);
    return DPPQDCEventWaveform<DPPQDCEvent>{ptr, elementSize};
}

template <>
inline DPPQDCEventWaveform<DPPQDCEventExtra> DPPQDCEventIterator::GroupIterator::event<DPPQDCEventWaveform<DPPQDCEventExtra> >()
{
    assert(extras == true);
    assert(waveform == true);
    return DPPQDCEventWaveform<DPPQDCEventExtra>{ptr, elementSize};
}


#endif //JADAQ_EVENTITERATOR_HPP
