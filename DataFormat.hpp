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

#define VERSION {1,2}

#define JUMBO_PAYLOAD 9000
#define IP_HEADER       20
#define UDP_HEADER       8

#define MAX(a,b) (((a)>(b))?(a):(b))
#define PRINTD(V) std::setw(MAX(sizeof(V)*3,sizeof(#V))) << V
#define PRINTH(V) std::setw(MAX(sizeof(V)*3,sizeof(#V))) << (#V)

/* TODO: Take care of endianness: https://linux.die.net/man/3/endian
 * We will use little endian for our network protocol since odds
 * are that both ends will be running on intel hardware
 */
namespace Data
{
    const uint16_t currentVersion = *(uint16_t*)(uint8_t[])VERSION;
    enum ElementType: uint16_t
    {
        None,
        List422,
        List8222,
        Waveform8222n2
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
        uint8_t __pad[6];
    };
    struct __attribute__ ((__packed__)) ListElement422
    {
        typedef uint32_t time_t;
        time_t time;
        uint16_t channel;
        uint16_t charge;
        bool operator< (const ListElement422& rhs) const
        {
            return time < rhs.time || (time == rhs.time && channel < rhs.channel) ;
        };
        void printOn(std::ostream& os) const
        {
            os << PRINTD(channel) << " " << PRINTD(time) << " " << PRINTD(charge);
        }
        static ElementType type() { return List422; }
        static H5::CompType h5type()
        {
            H5::CompType datatype(sizeof(ListElement422));
            datatype.insertMember("time", HOFFSET(ListElement422, time), H5::PredType::NATIVE_UINT32);
            datatype.insertMember("channel", HOFFSET(ListElement422, channel), H5::PredType::NATIVE_UINT16);
            datatype.insertMember("charge", HOFFSET(ListElement422, charge), H5::PredType::NATIVE_UINT16);

            return datatype;
        }
        static void headerOn(std::ostream& os)
        {
            os << PRINTH(channel) << " " << PRINTH(time) << " " << PRINTH(charge);
        }
    };
    struct __attribute__ ((__packed__)) ListElement8222
    {
        typedef uint64_t time_t;
        time_t time;
        uint16_t channel;
        uint16_t charge;
        uint16_t baseline;
        bool operator< (const ListElement8222& rhs) const
        {
            return time < rhs.time || (time == rhs.time && channel < rhs.channel) ;
        };
        void printOn(std::ostream& os) const
        {
            os << PRINTD(channel) << " " << PRINTD(time) << " " << PRINTD(charge) << " " << PRINTD(baseline) ;
        }
        static ElementType type() { return List8222; }
        static H5::CompType h5type()
        {
            H5::CompType datatype(sizeof(ListElement8222));
            datatype.insertMember("time", HOFFSET(ListElement8222, time), H5::PredType::NATIVE_UINT64);
            datatype.insertMember("channel", HOFFSET(ListElement8222, channel), H5::PredType::NATIVE_UINT16);
            datatype.insertMember("charge", HOFFSET(ListElement8222, charge), H5::PredType::NATIVE_UINT16);
            datatype.insertMember("baseline", HOFFSET(ListElement8222, baseline), H5::PredType::NATIVE_UINT16);
            return datatype;
        }
        static void headerOn(std::ostream& os)
        {
            os << PRINTH(channel) << " " << PRINTH(time) << " " << PRINTH(charge) << " " << PRINTH(baseline) ;
        }
    };
    template <typename T>
    struct Interval
    {
        T start;
        T end;
        void printOn(std::ostream& os) const
        {
            os << PRINTD(start) << " " << PRINTD(end);
        }
    };

    struct __attribute__ ((__packed__)) WaveformElement8222n2
    {
        typedef uint64_t time_t;
        typedef Interval<uint16_t > interval;
        time_t time;
        uint16_t channel;
        uint16_t charge;
        uint16_t baseline;
        uint16_t trigger;
        interval gate;
        interval holdoff;
        interval overthreshold;
        uint16_t num_samples;
        uint16_t samples[];

        bool operator< (const WaveformElement8222n2& rhs) const
        {
            return time < rhs.time || (time == rhs.time && channel < rhs.channel) ;
        };
        void printOn(std::ostream& os) const
        {

            for (uint16_t i = 0; i < num_samples; ++i)
            {
                os << " " <<  std::setw(5) << samples[i];
            }
        }
        static ElementType type() { return Waveform8222n2; }
        H5::CompType h5type() const
        {
            static const hsize_t n[1] = {num_samples};
            H5::CompType datatype(sizeof(WaveformElement8222n2)+ sizeof(uint16_t)*num_samples);
            H5::CompType h5interval(sizeof(interval));
            h5interval.insertMember("start", HOFFSET(interval, start), H5::PredType::NATIVE_UINT16);
            h5interval.insertMember("end", HOFFSET(interval, end), H5::PredType::NATIVE_UINT16);
            datatype.insertMember("time", HOFFSET(WaveformElement8222n2, time), H5::PredType::NATIVE_UINT64);
            datatype.insertMember("channel", HOFFSET(WaveformElement8222n2, channel), H5::PredType::NATIVE_UINT16);
            datatype.insertMember("charge", HOFFSET(WaveformElement8222n2, charge), H5::PredType::NATIVE_UINT16);
            datatype.insertMember("baseline", HOFFSET(WaveformElement8222n2, baseline), H5::PredType::NATIVE_UINT16);
            datatype.insertMember("trigger", HOFFSET(WaveformElement8222n2, trigger), H5::PredType::NATIVE_UINT16);
            datatype.insertMember("gate", HOFFSET(WaveformElement8222n2, gate), h5interval);
            datatype.insertMember("holdoff", HOFFSET(WaveformElement8222n2, holdoff), h5interval);
            datatype.insertMember("overthreshold", HOFFSET(WaveformElement8222n2, overthreshold), h5interval);
            datatype.insertMember("num_samples", HOFFSET(WaveformElement8222n2, num_samples), H5::PredType::NATIVE_UINT16);
            datatype.insertMember("samples", HOFFSET(WaveformElement8222n2, samples), H5::ArrayType(H5::PredType::NATIVE_UINT16,1,n));
            return datatype;
        }
        static void headerOn(std::ostream& os)
        {
            os << std::setw(10) << "channel" << " " << std::setw(20) << "localTime" <<
               " " << std::setw(10) << "adcValue" << " " << std::setw(10) << "baseline";
        }

    };
    static inline size_t elementSize(ElementType elementType)
    {
        switch (elementType)
        {
            case None:
                return 0;
            case List422:
                return sizeof(ListElement422);
            case List8222:
                return sizeof(ListElement8222);
            default:
                throw std::runtime_error("Unknown element type." );
        }
    }

    static constexpr const char* defaultDataPort = "12345";
    static constexpr const size_t maxBufferSize = JUMBO_PAYLOAD-(UDP_HEADER+IP_HEADER);

} // namespace Data
static inline std::ostream& operator<< (std::ostream& os, const Data::ListElement422& e)
{ e.printOn(os); return os; }
static inline std::ostream& operator<< (std::ostream& os, const Data::ListElement8222& e)
{ e.printOn(os); return os; }
static inline std::ostream& operator<< (std::ostream& os, const Data::WaveformElement8222n2& e)
{ e.printOn(os); return os; }

#endif //JADAQ_DATAFORMAT_HPP
