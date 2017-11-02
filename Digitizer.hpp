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
 * Get and set digitizer configuration values.
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
    void caenMallocReadoutBuffer() { readoutBuffer = digitizer->mallocReadoutBuffer(); }
    void caenFreeReadoutBuffer() { digitizer->freeReadoutBuffer(readoutBuffer); }
    caen::ReadoutBuffer caenGetReadoutBuffer() { return readoutBuffer; }
    void caenStartAcquisition() { digitizer->startAcquisition(); }
    void caenStopAcquisition() { digitizer->stopAcquisition(); }
};


#endif //JADAQ_DIGITIZER_HPP
