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
    caen::ReadoutBuffer readoutBuffer_;
    /* Standard firmware uses eventInfo and Event while DPP firmware
     * keeps it all in a DPPEvents structure. */
    caen::EventInfo eventInfo_;
    void *event_;
    caen::DPPEvents events_;
    caen::DPPWaveforms waveforms;
    int usb_;
    uint32_t vme_;
public:
    Digitizer(int usb, uint32_t vme) : digitizer(caen::Digitizer::USB(usb,vme)), usb_(usb), vme_(vme) {}
    const std::string name() { return digitizer->modelName() + "_" + std::to_string(digitizer->serialNumber()); }
    const std::string model() { return digitizer->modelName(); }
    const std::string serial() { return std::to_string(digitizer->serialNumber()); }
    const int usb() { return usb_; }
    const int vme() { return vme_; }
    void set(FunctionID functionID, std::string value);
    void set(FunctionID functionID, int index, std::string value);
    std::string get(FunctionID functionID);
    std::string get(FunctionID functionID, int index);
    caen::Digitizer* caen() { return digitizer; }

    /* Wrap the main CAEN acquisiton functions here for convenience */
    bool caenIsDPPFirmware() { return digitizer->getDPPFirmwareType() == CAEN_DGTZ_DPPFirmware_QDC; }
    void caenMallocPrivReadoutBuffer() { readoutBuffer_ = digitizer->mallocReadoutBuffer(); }
    void caenFreePrivReadoutBuffer() { digitizer->freeReadoutBuffer(readoutBuffer_); }
    caen::ReadoutBuffer& caenGetPrivReadoutBuffer() { return readoutBuffer_; }
    /* Additional event buffers */
    void caenMallocPrivEvent() { event_ = digitizer->mallocEvent(); }
    void caenFreePrivEvent() { digitizer->freeEvent(event_); event_ = NULL; }
    void caenMallocPrivDPPEvents() { events_ = digitizer->mallocDPPEvents(); }
    void caenFreePrivDPPEvents() { digitizer->freeDPPEvents(events_); }
    void caenMallocPrivDPPWaveforms() { waveforms = digitizer->mallocDPPWaveforms(); }
    void caenFreePrivDPPWaveforms() { digitizer->freeDPPWaveforms(waveforms); }
    caen::EventInfo& caenGetPrivEventInfo() { return eventInfo_; }
    void *caenGetPrivEvent() { return event_; }
    caen::DPPEvents& caenGetPrivDPPEvents() { return events_; }
    caen::DPPWaveforms& caenGetPrivDPPWaveforms() { return waveforms; }
    std::string caenDumpPrivDPPWaveforms() { return digitizer->dumpDPPWaveforms(waveforms); }
    /* We default to slave terminated mode like in the sample from CAEN
     * Digitizer library docs. */
    caen::ReadoutBuffer& caenReadData(caen::ReadoutBuffer& buffer, int mode=(int)CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT) { return digitizer->readData(buffer, (CAEN_DGTZ_ReadMode_t)mode); }
    uint32_t caenGetNumEvents(caen::ReadoutBuffer& buffer) { return digitizer->getNumEvents(buffer); }
    uint32_t caenGetNumEventsPerAggregate() { return digitizer->getNumEventsPerAggregate(); }
    caen::EventInfo caenGetEventInfo(caen::ReadoutBuffer& buffer, int32_t n) { eventInfo_ = digitizer->getEventInfo(buffer, n); return eventInfo_; }
    void *caenDecodeEvent(caen::EventInfo& buffer, void *event) { event = digitizer->decodeEvent(buffer, event); return event; }
    caen::BasicEvent caenExtractBasicEvent(caen::EventInfo& buffer, void* event, uint32_t channel, uint32_t eventNo) { return digitizer->extractBasicEvent(buffer, event, channel, eventNo); }
    caen::DPPEvents& caenGetDPPEvents(caen::ReadoutBuffer& buffer, caen::DPPEvents& events) { return digitizer->getDPPEvents(buffer, events); }
    void *caenExtractDPPEvent(caen::DPPEvents& events, uint32_t channel, uint32_t eventNo) { return digitizer->extractDPPEvent(events, channel, eventNo); }
    caen::BasicDPPEvent caenExtractBasicDPPEvent(caen::DPPEvents& events, uint32_t channel, uint32_t eventNo) { return digitizer->extractBasicDPPEvent(events, channel, eventNo); }
    caen::DPPWaveforms& caenDecodeDPPWaveforms(void *event, caen::DPPWaveforms& waveforms) { 
        return digitizer->decodeDPPWaveforms(event, waveforms); }
    caen::DPPWaveforms& caenDecodeDPPWaveforms(caen::DPPEvents& events, uint32_t channel, uint32_t eventNo, caen::DPPWaveforms& waveforms) { return digitizer->decodeDPPWaveforms(events, channel, eventNo, waveforms); }
    caen::BasicDPPWaveforms caenExtractBasicDPPWaveforms(caen::DPPWaveforms& waveforms) { return digitizer->extractBasicDPPWaveforms(waveforms); }
    bool caenHasDPPWaveformsEnabled() 
    { 
        if (!caenIsDPPFirmware()) {
            return false;
        }
        caen::EasyDPPBoardConfiguration boardConf = digitizer->getEasyDPPBoardConfiguration();
        uint8_t waveformRecording = boardConf.getValue("waveformRecording");
        return (waveformRecording == 1);
    }
    void caenStartAcquisition() { digitizer->startAcquisition(); }
    void caenStopAcquisition() { digitizer->stopAcquisition(); }
    void caenReset() { digitizer->reset(); }
};


#endif //JADAQ_DIGITIZER_HPP
