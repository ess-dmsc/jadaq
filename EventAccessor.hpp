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

#ifndef JADAQ_EVENTACCESSOR_HPP
#define JADAQ_EVENTACCESSOR_HPP

#include "caen.hpp"
#include "DataFormat.hpp"

class DPPEventAccessor
{
public:
    virtual uint16_t channels() const = 0;
    virtual uint32_t events(uint16_t channel) const = 0;
    virtual Data::ElementType elementType() const = 0;
};

class DPPEventLE422Accessor : public DPPEventAccessor
{
public:
    virtual Data::ListElement422 listElement422(uint16_t channel, size_t i) const = 0;
    Data::ElementType elementType() const final { return Data::List422;}
};

class DPPQDCEventAccessor : public DPPEventLE422Accessor
{
private:
    const caen::DPPEvents<_CAEN_DGTZ_DPP_QDC_Event_t>& container;
    const uint16_t numChannels;
public:
    DPPQDCEventAccessor(const caen::DPPEvents<_CAEN_DGTZ_DPP_QDC_Event_t>& events, uint16_t channels)
            : container(events)
            , numChannels(channels){}
    uint16_t channels() const override { return numChannels; }
    uint32_t events(uint16_t channel) const override { return container.nEvents[channel]; }
    Data::ListElement422 listElement422(uint16_t channel, size_t i) const override
    {
        Data::ListElement422 res;
        res.localTime = container.ptr[channel][i].TimeTag;
        res.adcValue = container.ptr[channel][i].Charge;
        res.channel = channel;
        return res;
    }

};

#endif //JADAQ_ACCESSOR_HPP
