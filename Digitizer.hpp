/**
 * jadaq (Just Another DAQ)
 * Copyright (C) 2017  Troels Blum <troels@blum.dk>
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
 * Digitizer access, used to get/set configuration values and to
 * do actual acquisition.
 *
 */

#ifndef JADAQ_DIGITIZER_HPP
#define JADAQ_DIGITIZER_HPP

#include "FunctionID.hpp"
#include "caen.hpp"

#include <string>
#include <unordered_map>

class Digitizer
{
private:
    caen::Digitizer* digitizer;
    /* We bind a ReadoutBuffer to each digitizer for ease of use */
    caen::ReadoutBuffer readoutBuffer;
    /* Standard firmware uses eventInfo and Event while DPP firmware
     * keeps it all in a DPPEvents structure. */
    caen::EventInfo eventInfo;
    void *event;
    caen::DPPEvents events;
    int usb_;
    uint32_t vme_;
public:
    Digitizer(int usb, uint32_t vme) : digitizer(caen::Digitizer::USB(usb,vme)), usb_(usb), vme_(vme) {}
    const std::string name() { return digitizer->modelName() + "_" + std::to_string(digitizer->serialNumber()); }
    const int usb() { return usb_; }
    const int vme() { return vme_; }
    void set(FunctionID functionID, std::string value);
    void set(FunctionID functionID, int index, std::string value);
    std::string get(FunctionID functionID);
    std::string get(FunctionID functionID, int index);
    caen::Digitizer* caen() { return digitizer; }

    /* Wrap the main CAEN acquisiton functions here for convenience */
    bool caenIsDPPFirmware() { return digitizer->getDPPFirmwareType() == CAEN_DGTZ_DPPFirmware_QDC; }
    void caenMallocPrivReadoutBuffer() { readoutBuffer = digitizer->mallocReadoutBuffer(); }
    void caenFreePrivReadoutBuffer() { digitizer->freeReadoutBuffer(readoutBuffer); }
    caen::ReadoutBuffer& caenGetPrivReadoutBuffer() { return readoutBuffer; }
    /* Additional event buffers */
    void caenMallocPrivEvent() { event = digitizer->mallocEvent(); }
    void caenFreePrivEvent() { digitizer->freeEvent(event); }
    void caenMallocPrivDPPEvents() { events = digitizer->mallocDPPEvents(); }
    void caenFreePrivDPPEvents() { digitizer->freeDPPEvents(events); }
    caen::EventInfo& caenGetPrivEventInfo() { return eventInfo; }
    void * caenGetPrivEvent() { return event; }
    caen::DPPEvents& caenGetPrivDPPEvents() { return events; }
    /* We default to slave terminated mode like in the sample from CAEN
     * Digitizer library docs. */
    caen::ReadoutBuffer& caenReadData(caen::ReadoutBuffer& buffer, int mode=(int)CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT) { return digitizer->readData(buffer, (CAEN_DGTZ_ReadMode_t)mode); }
    uint32_t caenGetNumEvents(caen::ReadoutBuffer& buffer) { return digitizer->getNumEvents(buffer); }
    caen::EventInfo caenGetEventInfo(caen::ReadoutBuffer& buffer, int32_t n) { eventInfo = digitizer->getEventInfo(buffer, n); return eventInfo; }
    void *caenDecodeEvent(caen::EventInfo& buffer, void *event) { event = digitizer->decodeEvent(buffer, event); return event; }
    caen::DPPEvents& caenGetDPPEvents(caen::ReadoutBuffer& buffer, caen::DPPEvents& events) { return digitizer->getDPPEvents(buffer, events); }
    void caenStartAcquisition() { digitizer->startAcquisition(); }
    void caenStopAcquisition() { digitizer->stopAcquisition(); }
};


#endif //JADAQ_DIGITIZER_HPP
