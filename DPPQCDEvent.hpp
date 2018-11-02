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

#ifndef JADAQ_DPPQCDEVENT_HPP
#define JADAQ_DPPQCDEVENT_HPP

#include <cstddef>
#include <cstdint>
#include "xtrace.h"

struct Event {
  uint32_t *ptr;
  size_t size;
  Event(uint32_t *p, size_t s) : ptr(p), size(s) {}
};

struct DPPQCDEvent : Event {
  DPPQCDEvent(uint32_t *p, size_t s) : Event(p, s) {}
  uint32_t timeTag() const { return ptr[0]; }
  uint16_t charge() const { return (uint16_t)(ptr[size - 1] & 0x0000ffffu); }
  uint8_t subChannel() const { return (uint8_t)(ptr[size - 1] >> 28); }
  uint16_t channel(uint16_t group) const { return (group << 3) | subChannel(); }
  static constexpr const bool extras = false;
};

struct DPPQCDEventExtra : DPPQCDEvent {
  DPPQCDEventExtra(uint32_t *p, size_t s) : DPPQCDEvent(p, s) {}
  uint16_t extendedTimeTag() const {
    return (uint16_t)(ptr[size - 2] & 0x0000ffffu);
  }
  uint16_t baseline() const { return (uint16_t)(ptr[size - 2] >> 16); }
  uint64_t fullTime() const {
    return ((uint64_t)timeTag()) | (((uint64_t)extendedTimeTag()) << 32);
  }
  static constexpr const bool extras = true;
};

struct Waveform;

template <typename DPPQCDEventType>
struct DPPQCDEventWaveform : DPPQCDEventType {
  DPPQCDEventWaveform(uint32_t *p, size_t s) : DPPQCDEventType(p, s) {}
  void waveform(Waveform &waveform) const;
};

#endif // JADAQ_DPPQCDEVENT_HPP
