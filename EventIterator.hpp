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

#include "Waveform.hpp"
#include "caen.hpp"
#include <iterator>
#include <limits>

/*
 * DPPQDCEventIterator will iterate over a set of Board Aggregates contained in
 * one Data Block
 */
class DPPQDCEventIterator {
private:
  const caen::ReadoutBuffer &buffer;
  uint32_t *ptr;
  uint32_t *boardAggregateEnd;
  /*
   * Group Iterator will iterate over a set of Group Aggregates contained in one
   * Board Aggregate
   */
  class GroupIterator {
  private:
    uint32_t *ptr;
    uint32_t *end;
    uint8_t groupMask = 0;
    size_t elementSize = 0;
    int group = -1;
    bool waveform = false;
    bool extras = false;
    static const uint channelsPerGroup = 8;

  public:
    GroupIterator(void *p) : ptr((uint32_t *)p) {}
    GroupIterator(uint32_t *p, uint16_t gm)
        : ptr(p), groupMask(gm), elementSize(2) {
      nextGroup();
    }
    void nextGroup() {
      while (!(groupMask & (1 << ++group))) {
        if (group == sizeof(groupMask) * CHAR_BIT) {
          return; // End of Board Aggregate
        }
      }
      assert(((ptr[0] >> 31) & 1) == 1);
      uint32_t size = ptr[0] & 0x7fffffff;
      uint32_t format = ptr[1];
      assert(((format >> 31) & 1) == 0);
      assert(((format >> 30) & 1) == 1);
      assert(((format >> 29) & 1) == 1);
      extras = ((format >> 28) & 1) == 1;
      waveform = ((format >> 27) & 1) == 1;
      end = ptr + size;
      ptr += 2; // point to first event
      elementSize = 2;
      if (extras)
        elementSize += 1;
      if (waveform)
        elementSize += (format & 0xFFF) << 2;
      assert((size - 2) % elementSize == 0);
    }

    bool waveformFlag() const { return waveform; }

    bool extrasFlag() const { return extras; }

    uint16_t currentGroup() {
      assert(group >= 0);
      return (uint16_t)group;
    }

    GroupIterator &operator++() {
      ptr += elementSize;
      if (ptr == end) {
        nextGroup();
      }
      assert(ptr <= end);
      return *this;
    }
    GroupIterator operator++(int) {
      GroupIterator tmp(*this);
      ++*this;
      return tmp;
    }
    bool operator==(const GroupIterator &other) const {
      return ptr == other.ptr;
    }
    bool operator!=(const GroupIterator &other) const {
      return (ptr != other.ptr);
    }
    bool operator>(const GroupIterator &other) const { return ptr > other.ptr; }
    bool operator<(const GroupIterator &other) const { return ptr < other.ptr; }
    bool operator>=(const GroupIterator &other) const {
      return ptr >= other.ptr;
    }
    bool operator<=(const GroupIterator &other) const {
      return ptr <= other.ptr;
    }
    bool operator==(const void *other) const { return ptr == other; }
    bool operator!=(const void *other) const { return ptr != other; }
    bool operator>(const void *other) const { return ptr > other; }
    bool operator<(const void *other) const { return ptr < other; }
    bool operator>=(const void *other) const { return ptr >= other; }
    bool operator<=(const void *other) const { return ptr <= other; }
    DPPQCDEvent operator*() const { return DPPQCDEvent{ptr, elementSize}; }
    template <typename T> T event() { return T{ptr, elementSize}; }
  };
  GroupIterator groupIterator;
  GroupIterator nextGroupIterator() {
    if ((char *)ptr < buffer.end()) {
      size_t size = (ptr[0] & 0x0fffffff);
      assert((ptr[0] & 0xf0000000) == 0xa0000000); // Magic value
      boardAggregateEnd = ptr + size;
      uint8_t groupMask = (uint8_t)(ptr[1] & 0xFF);
      ptr += 4; // point to first group aggregate
      return GroupIterator(ptr, groupMask);
    } else {
      return GroupIterator(buffer.end());
    }
  }

public:
  DPPQDCEventIterator(const caen::ReadoutBuffer &b)
      : buffer(b), ptr((uint32_t *)buffer.data),
        groupIterator(nextGroupIterator()) {}

  bool waveformFlag() const { return groupIterator.waveformFlag(); }

  bool extrasFlag() const { return groupIterator.extrasFlag(); }

  DPPQDCEventIterator &operator++() {
    if (++groupIterator == boardAggregateEnd) {
      ptr = boardAggregateEnd;
      groupIterator = nextGroupIterator();
    }
    return *this;
  }
  DPPQDCEventIterator operator++(int) {
    DPPQDCEventIterator tmp(*this);
    ++*this;
    return tmp;
  }
  bool operator==(const DPPQDCEventIterator &other) const {
    return groupIterator == other.groupIterator;
  }
  bool operator!=(const DPPQDCEventIterator &other) const {
    return groupIterator != other.groupIterator;
  }
  bool operator>(const DPPQDCEventIterator &other) const {
    return groupIterator > other.groupIterator;
  }
  bool operator<(const DPPQDCEventIterator &other) const {
    return groupIterator < other.groupIterator;
  }
  bool operator>=(const DPPQDCEventIterator &other) const {
    return groupIterator >= other.groupIterator;
  }
  bool operator<=(const DPPQDCEventIterator &other) const {
    return groupIterator <= other.groupIterator;
  }
  bool operator==(const void *other) const { return groupIterator == other; }
  bool operator!=(const void *other) const { return groupIterator != other; }
  bool operator>(const void *other) const { return groupIterator > other; }
  bool operator<(const void *other) const { return groupIterator < other; }
  bool operator>=(const void *other) const { return groupIterator >= other; }
  bool operator<=(const void *other) const { return groupIterator <= other; }
  DPPQCDEvent operator*() const { return *groupIterator; }
  void *end() const { return buffer.end(); }
  uint16_t group() { return groupIterator.currentGroup(); }
  template <typename T> T event() { return groupIterator.event<T>(); }
};

template <>
inline DPPQCDEvent DPPQDCEventIterator::GroupIterator::event<DPPQCDEvent>() {
  assert(extras == false);
  assert(waveform == false);
  return DPPQCDEvent{ptr, elementSize};
}

template <>
inline DPPQCDEventExtra
DPPQDCEventIterator::GroupIterator::event<DPPQCDEventExtra>() {
  assert(extras == true);
  assert(waveform == false);
  return DPPQCDEventExtra{ptr, elementSize};
}

template <>
inline DPPQCDEventWaveform<DPPQCDEvent>
DPPQDCEventIterator::GroupIterator::event<DPPQCDEventWaveform<DPPQCDEvent>>() {
  assert(extras == false);
  assert(waveform == true);
  return DPPQCDEventWaveform<DPPQCDEvent>{ptr, elementSize};
}

template <>
inline DPPQCDEventWaveform<DPPQCDEventExtra>
DPPQDCEventIterator::GroupIterator::event<
    DPPQCDEventWaveform<DPPQCDEventExtra>>() {
  assert(extras == true);
  assert(waveform == true);
  return DPPQCDEventWaveform<DPPQCDEventExtra>{ptr, elementSize};
}

#endif // JADAQ_EVENTITERATOR_HPP
