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
#include <chrono>
#include <thread>
#include <boost/thread/thread.hpp>
#include "trace.hpp"
#include "DataHandler.hpp"
#include "uuid.hpp"
#include "DataWriter.hpp"

class Digitizer
{
private:
    caen::Digitizer* digitizer = nullptr;
    CAEN_DGTZ_DPPFirmware_t firmware;
    uint32_t boardConfiguration = 0;
    uint32_t id;
    bool waveform = false;
    bool extras   = false;
    DataHandler dataHandler;
    std::set<uint32_t> manipulatedRegisters;
    caen::ReadoutBuffer readoutBuffer;

public:
    /* Connection parameters */
    const CAEN_DGTZ_ConnectionType linkType;
    const int linkNum;
    const int conetNode;
    const uint32_t VMEBaseAddress;
    STAT(struct Stats {
             uint32_t bytesRead = 0;
             uint32_t eventsFound = 0;
             uint32_t eventsUnpacked = 0;
             uint32_t eventsDecoded = 0;
             uint32_t eventsSent = 0;
         } stats;)
    Digitizer(CAEN_DGTZ_ConnectionType linkType_, int linkNum_, int conetNode_, uint32_t VMEBaseAddress_);
    const std::string name() { return digitizer->modelName() + "_" + std::to_string(digitizer->serialNumber()); }
    const std::string model() { return digitizer->modelName(); }
    const uint32_t serial() { return digitizer->serialNumber(); }
    uint32_t channels() const { return digitizer->channels(); }
    uint32_t groups() const { return digitizer->groups(); }
    void close(); //TODO: Why do we need close() in stead of using a destructor
    void set(FunctionID functionID, std::string value);
    void set(FunctionID functionID, int index, std::string value);
    std::string get(FunctionID functionID);
    std::string get(FunctionID functionID, int index);
    void acquisition();
    const std::set<uint32_t>& getRegisters() { return manipulatedRegisters; }
    // TODO: Sould we do somthing different than expose these three function?
    void startAcquisition() { digitizer->startAcquisition(); }
    void stopAcquisition() { digitizer->stopAcquisition(); }
    void reset() { digitizer->reset(); }

    template <template<typename...> typename C>
    void initialize(DataWriter& dataWriter)
    {
        boardConfiguration = digitizer->getBoardConfiguration();
        DEBUG(std::cout << "Prepare readout buffer for digitizer " << name() << std::endl;)
        readoutBuffer = digitizer->mallocReadoutBuffer();

        switch ((int)firmware) //Cast to int as long as CAEN_DGTZ_DPPFirmware_QDC is not part of the enumeration
        {
            case CAEN_DGTZ_DPPFirmware_PHA:
                throw std::runtime_error("PHA firmware not supported by Digitizer.");
                break;
            case CAEN_DGTZ_DPPFirmware_PSD:
                throw std::runtime_error("PSD firmware not supported by Digitizer.");
                break;
            case CAEN_DGTZ_DPPFirmware_CI:
                throw std::runtime_error("CI firmware not supported by Digitizer.");
                break;
            case CAEN_DGTZ_DPPFirmware_ZLE:
                throw std::runtime_error("ZLE firmware not supported by Digitizer.");
                break;
            case CAEN_DGTZ_DPPFirmware_QDC:
            {
                caen::Digitizer740DPP::BoardConfiguration bc{boardConfiguration};
                extras = bc.extras();
                waveform = bc.waveform();
                if (waveform)
                {
                    dataHandler.initialize<Data::WaveformElement,C>(dataWriter,serial(),groups());
                }
                else if (extras)
                {
                    dataHandler.initialize<Data::ListElement8222,C>(dataWriter,serial(),groups());
                } else
                {
                    dataHandler.initialize<Data::ListElement422,C>(dataWriter,serial(),groups());
                }
                break;
            }
            case CAEN_DGTZ_NotDPPFirmware:
                throw std::runtime_error("Non DPP firmware not supported by Digitizer.");
                break;
            default:
                throw std::runtime_error("Unknown firmware type. Not supported by Digitizer.");
        }
    }
};


#endif //JADAQ_DIGITIZER_HPP
