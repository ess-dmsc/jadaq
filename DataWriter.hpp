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
 * Data handler that simply throws data away - for performance testing purposes
 *
 */

#ifndef JADAQ_DATAWRITERNULL_HPP
#define JADAQ_DATAWRITERNULL_HPP


#include <cstdint>
#include "DataFormat.hpp"
#include "container.hpp"

class DataWriter
{
public:
    DataWriter() = default;
    template<typename DW>
    DataWriter &operator=(DW* dataWriter)
    {
        instance.reset(new Model<DW>(dataWriter));
        return *this;
    }

    void addDigitizer(uint64_t digitizerID)
    { instance->addDigitizer(digitizerID); }

    bool network() const
    { return instance->network(); }

    template<typename E>
    void operator()(const jadaq::buffer<E>* buffer, uint64_t digitizerID, uint64_t globalTimeStamp)
    { instance->operator()(buffer,digitizerID,globalTimeStamp); }


private:
    struct Concept
    {
        virtual ~Concept() = default;
        virtual void addDigitizer(uint64_t digitizerID) = 0;
        virtual bool network() const = 0;
        virtual void operator()(const jadaq::buffer<Data::ListElement422>* buffer, uint64_t digitizerID, uint64_t globalTimeStamp) = 0;
        virtual void operator()(const jadaq::buffer<Data::ListElement8222>* buffer, uint64_t digitizerID, uint64_t globalTimeStamp) = 0;
        virtual void operator()(const jadaq::buffer<Data::StdElement751>* buffer, uint64_t digitizerID, uint64_t globalTimeStamp) = 0;
        virtual void operator()(const jadaq::buffer<Data::DPPQDCWaveformElement<Data::ListElement422> >* buffer, uint64_t digitizerID, uint64_t globalTimeStamp) = 0;
        virtual void operator()(const jadaq::buffer<Data::DPPQDCWaveformElement<Data::ListElement8222> >* buffer, uint64_t digitizerID, uint64_t globalTimeStamp) = 0;
    };
    template <typename DW>
    struct Model : Concept
    {
        explicit Model(DW* value) : val(value) {}
        ~Model() { delete val; }
        void addDigitizer(uint64_t digitizerID) override
        { val->addDigitizer(digitizerID); }
        bool network() const override
        { return val->network(); }
        void operator()(const jadaq::buffer<Data::ListElement422>* buffer, uint64_t digitizerID, uint64_t globalTimeStamp) final
        { val->operator()(buffer,digitizerID,globalTimeStamp); }
        void operator()(const jadaq::buffer<Data::ListElement8222>* buffer, uint64_t digitizerID, uint64_t globalTimeStamp) final
        { val->operator()(buffer,digitizerID,globalTimeStamp); }
        void operator()(const jadaq::buffer<Data::StdElement751>* buffer, uint64_t digitizerID, uint64_t globalTimeStamp) final
        { val->operator()(buffer,digitizerID,globalTimeStamp); }
        void operator()(const jadaq::buffer<Data::DPPQDCWaveformElement<Data::ListElement422> >* buffer, uint64_t digitizerID, uint64_t globalTimeStamp) final
        { val->operator()(buffer,digitizerID,globalTimeStamp); }
        void operator()(const jadaq::buffer<Data::DPPQDCWaveformElement<Data::ListElement8222> >* buffer, uint64_t digitizerID, uint64_t globalTimeStamp) final
        { val->operator()(buffer,digitizerID,globalTimeStamp); }
        DW* val;
    };

    std::unique_ptr<Concept> instance;
};

class DataWriterNull
{
public:
    DataWriterNull() = default;
    void addDigitizer(uint64_t) {}
    static bool network() { return false; }
    template <typename E>
    void operator()(const jadaq::buffer<E>*, uint64_t, uint64_t) {}
};


#endif //JADAQ_DATAWRITERNULL_HPP
