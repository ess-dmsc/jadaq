/**
 * jadaq (Just Another DAQ)
 * Copyright (C) 2018  Troels Blum <troels@blum.dk>
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
 */

#ifndef JADAQ_DATAFORMAT_HPP
#define JADAQ_DATAFORMAT_HPP

#include <chrono>
#include <cstdint>
#include <cstddef>
#include <iostream>
#include <H5Cpp.h>
#include <iomanip>
#include "Waveform.hpp"
#include "EventIterator.hpp"

#define VERSION {1,2}

#define JUMBO_PAYLOAD 9000
#define IP_HEADER       20
#define UDP_HEADER       8

/* TODO: Take care of endianness: https://linux.die.net/man/3/endian
 * We will use little endian for our network protocol since odds
 * are that both ends will be running on intel hardware
 */
namespace Data
{
    const uint16_t currentVersion = *(uint16_t*)(uint8_t[])VERSION;
    const constexpr uint16_t WaveformBase = 1<<8;
    enum ElementType: uint16_t
    {
        None,
        List422,
        List8222,
        Waveform422 = WaveformBase | List422,
        Waveform8222 = WaveformBase | List8222,
    };
    /* Shared meta data for the entire data package */
    struct __attribute__ ((__packed__)) Header // 32 bytes
    {
        uint64_t runID;
        uint64_t globalTime;
        uint32_t digitizerID;
        uint16_t elementType;
        uint16_t numElements;
        uint16_t version;
        uint32_t seqNum;
        uint8_t __pad[2];
    };
    static_assert(std::is_pod<Header>::value, "Data::Header must be POD");

    struct __attribute__ ((__packed__)) ListElement422
    {
        typedef uint32_t time_t;
        typedef DPPQCDEvent EventType;
        time_t time;
        uint16_t channel;
        uint16_t charge;
        ListElement422() = default;
        ListElement422(const EventType& event, uint16_t group)
        {
            time = event.timeTag();
            channel = event.channel(group);
            charge = event.charge();
        }
        bool operator< (const ListElement422& rhs) const
        {
            return time < rhs.time || (time == rhs.time && channel < rhs.channel) ;
        };
        void printOn(std::ostream& os) const
        {
            os << PRINTD(channel) << " " << PRINTD(time) << " " << PRINTD(charge);
        }
        static ElementType type() { return List422; }
        static void insertMembers(H5::CompType& datatype)
        {
            datatype.insertMember("time", HOFFSET(ListElement422, time), H5::PredType::NATIVE_UINT32);
            datatype.insertMember("channel", HOFFSET(ListElement422, channel), H5::PredType::NATIVE_UINT16);
            datatype.insertMember("charge", HOFFSET(ListElement422, charge), H5::PredType::NATIVE_UINT16);
        }
        static size_t size() { return sizeof(ListElement422); }
        static size_t size(size_t) { return size(); }
        static H5::CompType h5type()
        {
            H5::CompType datatype(size());
            insertMembers(datatype);
            return datatype;
        }
        static void headerOn(std::ostream& os)
        {
            os << PRINTH(channel) << " " << PRINTH(time) << " " << PRINTH(charge);
        }
    };
    static_assert(std::is_pod<ListElement422>::value, "Data::ListElement422 must be POD");

    struct __attribute__ ((__packed__)) ListElement8222
    {
        typedef uint64_t time_t;
        typedef DPPQCDEventExtra EventType;
        time_t time;
        uint16_t channel;
        uint16_t charge;
        uint16_t baseline;
        ListElement8222() = default;
        ListElement8222(const EventType& event, uint16_t group)
        {
            time = event.fullTime();
            channel = event.channel(group);
            charge = event.charge();
            baseline = event.baseline();
        }
        bool operator< (const ListElement8222& rhs) const
        {
            return time < rhs.time || (time == rhs.time && channel < rhs.channel) ;
        };
        void printOn(std::ostream& os) const
        {
            os << PRINTD(channel) << " " << PRINTD(time) << " " << PRINTD(charge) << " " << PRINTD(baseline) ;
        }
        static ElementType type() { return List8222; }
        static void insertMembers(H5::CompType& datatype)
        {
            datatype.insertMember("time", HOFFSET(ListElement8222, time), H5::PredType::NATIVE_UINT64);
            datatype.insertMember("channel", HOFFSET(ListElement8222, channel), H5::PredType::NATIVE_UINT16);
            datatype.insertMember("charge", HOFFSET(ListElement8222, charge), H5::PredType::NATIVE_UINT16);
            datatype.insertMember("baseline", HOFFSET(ListElement8222, baseline), H5::PredType::NATIVE_UINT16);
        }
        static size_t size() { return sizeof(ListElement8222); }
        static size_t size(size_t) { return size(); }
        static H5::CompType h5type()
        {
            H5::CompType datatype(size());
            insertMembers(datatype);
            return datatype;
        }
        static void headerOn(std::ostream& os)
        {
            os << PRINTH(channel) << " " << PRINTH(time) << " " << PRINTH(charge) << " " << PRINTH(baseline) ;
        }
    };
    static_assert(std::is_pod<ListElement8222>::value, "Data::ListElement8222 must be POD");

    template <typename ListElementType>
    struct __attribute__ ((__packed__)) WaveformElement
    {
        typedef DPPQCDEventWaveform<typename ListElementType::EventType> EventType;
        ListElementType listElement;
        Waveform waveform;
        WaveformElement() = default;
        WaveformElement(const EventType& event, uint16_t group)
                : listElement(event,group)
                , waveform{event} {}
        bool operator< (const WaveformElement& rhs) const
        { return listElement < rhs.listElement; }
        void printOn(std::ostream& os) const
        {
            listElement.printOn(os); os << " ";
            waveform.printOn(os);
        }
        static void headerOn(std::ostream& os)
        {
            ListElementType::headerOn(os);
            Waveform::headerOn(os);
        }
        static ElementType type() { return (ElementType)(WaveformBase | ListElementType::type()); }
        void insertMembers(H5::CompType& datatype) const
        {
            listElement.insertMembers(datatype);
            waveform.insertMembers(datatype,offsetof(WaveformElement,waveform));
        }
        static size_t size(size_t samples) { return ListElementType::size() + Waveform::size(samples); }
        H5::CompType h5type() const
        {
            H5::CompType datatype(size(waveform.num_samples));
            insertMembers(datatype);
            return datatype;
        }
    };
    static_assert(std::is_pod<WaveformElement<Data::ListElement422> >::value, "Data::WaveformElement<Data::ListElement422> > must be POD");
    static_assert(std::is_pod<WaveformElement<Data::ListElement8222> >::value, "Data::WaveformElement<Data::ListElement8222> > must be POD");

    static constexpr const char* defaultDataPort = "12345";
    static constexpr const size_t maxBufferSize = JUMBO_PAYLOAD-(UDP_HEADER+IP_HEADER);

} // namespace Data
static inline std::ostream& operator<< (std::ostream& os, const Data::ListElement422& e)
{ e.printOn(os); return os; }
static inline std::ostream& operator<< (std::ostream& os, const Data::ListElement8222& e)
{ e.printOn(os); return os; }
static inline std::ostream& operator<< (std::ostream& os, const Data::WaveformElement<Data::ListElement422>& e)
{ e.printOn(os); return os; }
static inline std::ostream& operator<< (std::ostream& os, const Data::WaveformElement<Data::ListElement8222>& e)
{ e.printOn(os); return os; }

#endif //JADAQ_DATAFORMAT_HPP
