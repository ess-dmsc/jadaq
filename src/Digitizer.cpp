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
 * Get and set digitizer configuration values.
 *
 */

#include "Digitizer.hpp"
#include "StringConversion.hpp"
#include <chrono>
#include <iomanip>
#include <regex>
#include <thread>
#include "xtrace.h"

#define MAX_GROUPS 8

#define SET_CASE(D, F, V)                                                      \
  case F:                                                                      \
    D->set##F(V);                                                              \
    break;

#define GET_CASE(D, F, TS)                                                     \
  case F:                                                                      \
    return TS(D->get##F());

#define SET_ICASE(D, F, C, V)                                                  \
  case F:                                                                      \
    D->set##F(C, V);                                                           \
    break;

#define GET_ICASE(D, F, C, TS)                                                 \
  case F:                                                                      \
    return TS(D->get##F(C));

static void set_(caen::Digitizer *digitizer, FunctionID functionID,
                 const std::string &value) {
  switch (functionID) {
    SET_CASE(digitizer, MaxNumEventsBLT, s2ui(value))
    SET_CASE(digitizer, ChannelEnableMask, bs2ui(value))
    SET_CASE(digitizer, GroupEnableMask, bs2ui(value))
    SET_CASE(digitizer, DecimationFactor, s2ui(value))
    SET_CASE(digitizer, PostTriggerSize, s2ui(value))
    SET_CASE(digitizer, IOlevel, s2iol(value))
    SET_CASE(digitizer, AcquisitionMode, s2am(value))
    SET_CASE(digitizer, ExternalTriggerMode, s2tm(value))
    SET_CASE(digitizer, SWTriggerMode, s2tm(value))
    SET_CASE(digitizer, RunSynchronizationMode, s2rsm(value))
    SET_CASE(digitizer, OutputSignalMode, s2osm(value))
    SET_CASE(digitizer, DESMode, s2ed(value))
    SET_CASE(digitizer, ZeroSuppressionMode, s2zsm(value))
    SET_CASE(digitizer, AnalogMonOutput, s2amom(value))
    SET_CASE(digitizer, AnalogInspectionMonParams, s2aimp(value))
    SET_CASE(digitizer, EventPackaging, s2ed(value))
    SET_CASE(digitizer, TriggerCountingMode, s2ui(value))
    SET_CASE(digitizer, FastTriggerDigitizing, s2ed(value))
    SET_CASE(digitizer, FastTriggerMode, s2tm(value))
    SET_CASE(digitizer, DRS4SamplingFrequency, s2drsff(value))
    SET_CASE(digitizer, DPPAcquisitionMode, s2cdam(value))
    SET_CASE(digitizer, DPPTriggerMode, s2dtm(value))
    SET_CASE(digitizer, MaxNumAggregatesBLT, s2ui(value))
    SET_CASE(digitizer, SAMCorrectionLevel, s2samcl(value))
    SET_CASE(digitizer, SAMSamplingFrequency, s2samf(value))
    SET_CASE(digitizer, SAMAcquisitionMode, s2samam(value))
    SET_CASE(digitizer, TriggerLogic, s2tlp(value))
    SET_CASE(digitizer, BoardConfiguration, s2ui(value))
    SET_CASE(digitizer, DPPAggregateOrganization, s2ui(value))
    SET_CASE(digitizer, AcquisitionControl, s2ui(value))
    SET_CASE(digitizer, GlobalTriggerMask, s2ui(value))
    SET_CASE(digitizer, FrontPanelTRGOUTEnableMask, s2ui(value))
    SET_CASE(digitizer, FrontPanelIOControl, s2ui(value))
    SET_CASE(digitizer, FanSpeedControl, s2ui(value))
    SET_CASE(digitizer, DPPDisableExternalTrigger, s2ui(value))
    SET_CASE(digitizer, RunStartStopDelay, s2ui(value))
    SET_CASE(digitizer, ReadoutControl, s2ui(value))
    SET_CASE(digitizer, Scratch, s2ui(value))
    SET_CASE(digitizer, DPPAggregateNumberPerBLT, s2ui(value))
    SET_CASE(digitizer, RecordLength, s2ui(value))
    SET_CASE(digitizer, NumEventsPerAggregate, s2ui(value))
    SET_CASE(digitizer, DPPGateWidth, s2ui(value))
    SET_CASE(digitizer, DPPGateOffset, s2ui(value))
    SET_CASE(digitizer, DPPFixedBaseline, s2ui(value))
    SET_CASE(digitizer, DPPAlgorithmControl, s2ui(value))
    SET_CASE(digitizer, DPPTriggerHoldOffWidth, s2ui(value))
    SET_CASE(digitizer, DPPShapedTriggerWidth, s2ui(value))
  default:
    /* lookup function name to determine if it is missing or just
     * read-only. Automatically throws invalid_argument exception
     * if ID is missing. */
    std::string name = to_string(functionID);
    throw std::runtime_error{"Cannot set read-only variables"};
  }
}

static void set_(caen::Digitizer *digitizer, FunctionID functionID, int index,
                 const std::string &value) {
  switch (functionID) {
  case Register:
    digitizer->writeRegister(index, s2ui(value));
    break;
    SET_ICASE(digitizer, ChannelDCOffset, index, s2ui(value))
    SET_ICASE(digitizer, GroupDCOffset, index, s2ui(value))
    SET_ICASE(digitizer, ChannelSelfTrigger, index, s2tm(value))
    SET_ICASE(digitizer, GroupSelfTrigger, index, s2tm(value))
    SET_ICASE(digitizer, ChannelTriggerThreshold, index, s2ui(value))
    SET_ICASE(digitizer, GroupTriggerThreshold, index, s2ui(value))
    SET_ICASE(digitizer, ChannelGroupMask, index, s2ui(value))
    SET_ICASE(digitizer, TriggerPolarity, index, s2tp(value))
    SET_ICASE(digitizer, GroupFastTriggerThreshold, index, s2ui(value))
    SET_ICASE(digitizer, GroupFastTriggerDCOffset, index, s2ui(value))
    SET_ICASE(digitizer, ChannelZSParams, index, s2zsp(value))
    SET_ICASE(digitizer, SAMPostTriggerSize, index, s2i(value))
    SET_ICASE(digitizer, SAMTriggerCountVetoParam, index, s2samtcvp(value))
    SET_ICASE(digitizer, DPPPreTriggerSize, index, s2ui(value))
    SET_ICASE(digitizer, ChannelPulsePolarity, index, s2pp(value))
    SET_ICASE(digitizer, RecordLength, index, s2ui(value))
    SET_ICASE(digitizer, NumEventsPerAggregate, index, s2ui(value))
    SET_ICASE(digitizer, DPPGateWidth, index, s2ui(value))
    SET_ICASE(digitizer, DPPGateOffset, index, s2ui(value))
    SET_ICASE(digitizer, DPPFixedBaseline, index, s2ui(value))
    SET_ICASE(digitizer, DPPAlgorithmControl, index, s2ui(value))
    SET_ICASE(digitizer, DPPTriggerHoldOffWidth, index, s2ui(value))
    SET_ICASE(digitizer, DPPShapedTriggerWidth, index, s2ui(value))
  default:
    /* lookup function name to determine if it is missing or just
     * read-only. Automatically throws invalid_argument exception
     * if ID is missing. */
    std::string name = to_string(functionID);
    throw std::runtime_error{"Cannot set read-only variables"};
  }
}

static std::string get_(caen::Digitizer *digitizer, FunctionID functionID) {
  switch (functionID) {
    GET_CASE(digitizer, MaxNumEventsBLT, to_string)
    GET_CASE(digitizer, ChannelEnableMask, bin_string)
    GET_CASE(digitizer, GroupEnableMask, bin_string<MAX_GROUPS>)
    GET_CASE(digitizer, DecimationFactor, to_string)
    GET_CASE(digitizer, PostTriggerSize, to_string)
    GET_CASE(digitizer, IOlevel, to_string)
    GET_CASE(digitizer, AcquisitionMode, to_string)
    GET_CASE(digitizer, ExternalTriggerMode, to_string)
    GET_CASE(digitizer, SWTriggerMode, to_string)
    GET_CASE(digitizer, RunSynchronizationMode, to_string)
    GET_CASE(digitizer, OutputSignalMode, to_string)
    GET_CASE(digitizer, DESMode, to_string)
    GET_CASE(digitizer, ZeroSuppressionMode, to_string)
    GET_CASE(digitizer, AnalogMonOutput, to_string)
    GET_CASE(digitizer, AnalogInspectionMonParams, to_string)
    GET_CASE(digitizer, EventPackaging, to_string)
    GET_CASE(digitizer, TriggerCountingMode, to_string)
    GET_CASE(digitizer, FastTriggerDigitizing, to_string)
    GET_CASE(digitizer, FastTriggerMode, to_string)
    GET_CASE(digitizer, DRS4SamplingFrequency, to_string)
    GET_CASE(digitizer, DPPAcquisitionMode, to_string)
    GET_CASE(digitizer, DPPTriggerMode, to_string)
    GET_CASE(digitizer, MaxNumAggregatesBLT, to_string)
    GET_CASE(digitizer, SAMCorrectionLevel, to_string)
    GET_CASE(digitizer, SAMSamplingFrequency, to_string)
    GET_CASE(digitizer, SAMAcquisitionMode, to_string)
    GET_CASE(digitizer, TriggerLogic, to_string)
    GET_CASE(digitizer, BoardConfiguration, to_string)
    GET_CASE(digitizer, DPPAggregateOrganization, to_string)
    GET_CASE(digitizer, AcquisitionControl, to_string)
    GET_CASE(digitizer, AcquisitionStatus, to_string)
    GET_CASE(digitizer, GlobalTriggerMask, to_string)
    GET_CASE(digitizer, FrontPanelTRGOUTEnableMask, to_string)
    GET_CASE(digitizer, FrontPanelIOControl, to_string)
    GET_CASE(digitizer, ROCFPGAFirmwareRevision, to_string)
    GET_CASE(digitizer, EventSize, to_string)
    GET_CASE(digitizer, FanSpeedControl, to_string)
    GET_CASE(digitizer, DPPDisableExternalTrigger, to_string)
    GET_CASE(digitizer, RunStartStopDelay, to_string)
    GET_CASE(digitizer, ReadoutControl, to_string)
    GET_CASE(digitizer, ReadoutStatus, to_string)
    GET_CASE(digitizer, Scratch, to_string)
    GET_CASE(digitizer, DPPAggregateNumberPerBLT, to_string)
    GET_CASE(digitizer, RecordLength, to_string)
    GET_CASE(digitizer, NumEventsPerAggregate, to_string)
    GET_CASE(digitizer, DPPTriggerHoldOffWidth, to_string)
    GET_CASE(digitizer, DPPShapedTriggerWidth, to_string)
  default:
    std::cerr << "get_ found unknown functionid: " << functionID << std::endl;
    throw std::invalid_argument{"Unknown Function"};
  }
}

static std::string get_(caen::Digitizer *digitizer, FunctionID functionID,
                        int index) {

  switch (functionID) {
  case Register:
    return hex_string(digitizer->readRegister(index));
    GET_ICASE(digitizer, ChannelDCOffset, index, to_string)
    GET_ICASE(digitizer, GroupDCOffset, index, to_string)
    GET_ICASE(digitizer, AMCFirmwareRevision, index, to_string)
    GET_ICASE(digitizer, ChannelSelfTrigger, index, to_string)
    GET_ICASE(digitizer, GroupSelfTrigger, index, to_string)
    GET_ICASE(digitizer, ChannelTriggerThreshold, index, to_string)
    GET_ICASE(digitizer, GroupTriggerThreshold, index, to_string)
    GET_ICASE(digitizer, ChannelGroupMask, index, to_string)
    GET_ICASE(digitizer, TriggerPolarity, index, to_string)
    GET_ICASE(digitizer, GroupFastTriggerThreshold, index, to_string)
    GET_ICASE(digitizer, GroupFastTriggerDCOffset, index, to_string)
    GET_ICASE(digitizer, ChannelZSParams, index, to_string)
    GET_ICASE(digitizer, SAMPostTriggerSize, index, to_string)
    GET_ICASE(digitizer, SAMTriggerCountVetoParam, index, to_string)
    GET_ICASE(digitizer, DPPPreTriggerSize, index, to_string)
    GET_ICASE(digitizer, ChannelPulsePolarity, index, to_string)
    GET_ICASE(digitizer, RecordLength, index, to_string)
    GET_ICASE(digitizer, NumEventsPerAggregate, index, to_string)
    GET_ICASE(digitizer, DPPGateWidth, index, to_string)
    GET_ICASE(digitizer, DPPGateOffset, index, to_string)
    GET_ICASE(digitizer, DPPFixedBaseline, index, to_string)
    GET_ICASE(digitizer, DPPAlgorithmControl, index, to_string)
    GET_ICASE(digitizer, DPPTriggerHoldOffWidth, index, to_string)
    GET_ICASE(digitizer, DPPShapedTriggerWidth, index, to_string)
  default:
    std::cerr << "get_ indexed found unknown functionid: " << functionID
              << std::endl;
    throw std::invalid_argument{"Unknown Function"};
  }
}

template <typename R, typename F>
static R
backOffRepeat(F fun, int retry = 3,
              std::chrono::milliseconds grace = std::chrono::milliseconds(1)) {
  while (true) {
    try {
      return fun();
    } catch (caen::Error &e) {
      if (e.code() == CAEN_DGTZ_CommError && retry-- > 0) {
        std::this_thread::sleep_for(grace);
        grace *= 10;
      } else
        throw;
    }
  }
}

std::string Digitizer::get(FunctionID functionID, int index) {
  return backOffRepeat<std::string>([this, &functionID, &index]() {
    return get_(digitizer, functionID, index);
  });
}

std::string Digitizer::get(FunctionID functionID) {
  return backOffRepeat<std::string>(
      [this, &functionID]() { return get_(digitizer, functionID); });
}

void Digitizer::set(FunctionID functionID, int index, std::string value) {
  try {
    backOffRepeat<void>([this, &functionID, &index, &value]() {
      return set_(digitizer, functionID, index, value);
    });
  } catch (std::invalid_argument &e) {
    if (functionID != Register)
      throw;
  }
  if (functionID == Register) {
    manipulatedRegisters.insert(index);
  }
}

void Digitizer::set(FunctionID functionID, std::string value) {
  backOffRepeat<void>([this, &functionID, &value]() {
    return set_(digitizer, functionID, value);
  });
}

Digitizer::Digitizer(CAEN_DGTZ_ConnectionType linkType_, int linkNum_, int conetNode_, uint32_t VMEBaseAddress_)
        : digitizer(caen::Digitizer::open(linkType_, linkNum_, conetNode_, VMEBaseAddress_))
        , linkType(linkType_)
        , linkNum(linkNum_)
        , conetNode(conetNode_)
        , VMEBaseAddress(VMEBaseAddress_)
{
  XTRACE(DIGIT, DEB, "Digitizer::Digitizer()");
  // NULL digitizer
  if (linkType == (CAEN_DGTZ_ConnectionType)ECDC_NULL_CONNECTION) {
    id = 0xaaaabbbb;
    return;
  }
    firmware = digitizer->getDPPFirmwareType();
    /* Generate an unique ID based on model and serial number.
       Despite casting to 32bit, the result _is_ unique:
       - internally, the serial no is 16bit wide (see Common.c:827)
       - model no is set from enum CAEN_DGTZ_BoardModel_t with max val 41 (CAEN library 2.9.1, see CAENDigitizerType.h:349->CAENDigitizer.c:519ff->Common.c:838)
     */
    id = (digitizer->modelNo()<<16)|digitizer->serialNumber();
}

void Digitizer::initialize(DataWriter& dataWriter)
{
  XTRACE(DIGIT, DEB, "Digitizer::initialize()");
  XTRACE(DIGIT, DEB, "Prepare readout buffer for digitizer %s", name().c_str());

  // ECDC_NULL_CONNECTION
  if (id == 0xaaaabbbb) {
    readoutBuffer.size = 9000;
    readoutBuffer.data = (char *)malloc(9000);
    uint32_t groups = 16;
    acqWindowSize = new uint32_t[groups];
    dataWriter.addDigitizer(digitizerID());
    dataHandler.initialize<Data::ListElement422>(dataWriter, digitizerID(), groups,
                                                 waveforms, acqWindowSize);
    return;
  }


    readoutBuffer = digitizer->mallocReadoutBuffer();
    dataWriter.addDigitizer(digitizerID());
    // model- and firmware-dependent initialization
    switch (digitizer->familyCode()){
    case CAEN_DGTZ_XX751_FAMILY_CODE:
      boardConfiguration = digitizer->getBoardConfiguration();
      switch ((int)firmware) //Cast to int as long as CAEN_DGTZ_DPPFirmware_QDC is not part of the enumeration
        {
        case CAEN_DGTZ_DPPFirmware_PHA:
          throw std::runtime_error("PHA firmware not supported by jadaq::Digitizer on " + digitizer->modelName());
          break;
        case CAEN_DGTZ_DPPFirmware_PSD:
          throw std::runtime_error("PSD firmware not supported by jadaq::Digitizer on " + digitizer->modelName());
          break;
        case CAEN_DGTZ_DPPFirmware_CI:
          throw std::runtime_error("CI firmware not supported by jadaq::Digitizer on " + digitizer->modelName());
          break;
        case CAEN_DGTZ_DPPFirmware_ZLE:
          throw std::runtime_error("ZLE firmware not supported by jadaq::Digitizer on " + digitizer->modelName());
          break;
        case CAEN_DGTZ_DPPFirmware_QDC:
          throw std::runtime_error("QDC firmware not supported by jadaq::Digitizer on " + digitizer->modelName());
          break;
        case CAEN_DGTZ_NotDPPFirmware:{
          waveforms = digitizer->getRecordLength()*digitizer->getNChannelEnabled();
          acqWindowSize = new uint32_t[groups()];
          for (uint32_t i = 0; i < groups(); ++i){
            // TODO: initialize acqWindowSize elsewhere for all digitizer types
            acqWindowSize[i] = 0; // no "jitter" expected
          }
          dataHandler.initialize<Data::StdElement751>(dataWriter,digitizerID(), groups(), waveforms, acqWindowSize);
          break;
        }
        default:
          throw std::runtime_error("Unknown firmware type. Not supported by jadaq::Digitizer on " + digitizer->modelName());
        }
      break;
    case CAEN_DGTZ_XX740_FAMILY_CODE:
      boardConfiguration = digitizer->getBoardConfiguration();
      switch ((int)firmware) //Cast to int as long as CAEN_DGTZ_DPPFirmware_QDC is not part of the enumeration
        {
        case CAEN_DGTZ_DPPFirmware_PHA:
          throw std::runtime_error("PHA firmware not supported by jadaq::Digitizer on " + digitizer->modelName());
          break;
        case CAEN_DGTZ_DPPFirmware_PSD:
          throw std::runtime_error("PSD firmware not supported by jadaq::Digitizer on " + digitizer->modelName());
          break;
        case CAEN_DGTZ_DPPFirmware_CI:
          throw std::runtime_error("CI firmware not supported by jadaq::Digitizer on " + digitizer->modelName());
          break;
        case CAEN_DGTZ_DPPFirmware_ZLE:
          throw std::runtime_error("ZLE firmware not supported by jadaq::Digitizer on " + digitizer->modelName());
          break;
        case CAEN_DGTZ_DPPFirmware_QDC:
          {
            caen::Digitizer740DPP::BoardConfiguration bc{boardConfiguration};
            uint32_t groups = this->groups();
            acqWindowSize = new uint32_t[groups];
            extras = bc.extras();
            if (bc.waveform())
                waveforms = digitizer->getRecordLength(0);
            for (uint32_t i = 0; i < groups; ++i)
            {
                acqWindowSize[i] = std::max({digitizer->getRecordLength(i)*bc.waveform(),
                                             digitizer->getDPPPreTriggerSize(i) + digitizer->getDPPTriggerHoldOffWidth(i),
                      digitizer->getDPPGateWidth(i) - digitizer->getDPPGateOffset(i)+ digitizer->getDPPPreTriggerSize(i)}) * 2; // Lets be conservative :P
            }

            if (waveforms)
              {
                if (extras)
                    dataHandler.initialize<Data::DPPQDCWaveformElement<Data::ListElement8222> >(dataWriter,digitizerID(),groups,waveforms,acqWindowSize);
                else
                    dataHandler.initialize<Data::DPPQDCWaveformElement<Data::ListElement422> >(dataWriter,digitizerID(),groups,waveforms,acqWindowSize);
            }
            else if (extras)
            {
                dataHandler.initialize<Data::ListElement8222>(dataWriter,digitizerID(),groups,waveforms,acqWindowSize);
            } else
            {
                dataHandler.initialize<Data::ListElement422>(dataWriter,digitizerID(),groups,waveforms,acqWindowSize);
            }
            break;
          }
        case CAEN_DGTZ_NotDPPFirmware:
          throw std::runtime_error("Standard firmware not supported by jadaq::Digitizer on " + digitizer->modelName());
          break;
        default:
          throw std::runtime_error("Unknown firmware type. Not supported by jadaq::Digitizer on " + digitizer->modelName());
        } // 740D
      break;
    default:
      throw std::runtime_error("Unknown digitizer type. Not supported by jadaq::Digitizer on " + digitizer->modelName());
    } // familyCode

    // flush read out buffer on the digitizer
    // NOTE: it turns out there is no indication that this is necessary; see issue #26; remove at will
    uint32_t bytesRead = 1;
    uint32_t bytesFlushed = 0;
    while (bytesRead > 0){
      digitizer->readData(readoutBuffer, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT);
      bytesRead = readoutBuffer.dataSize;
      bytesFlushed += bytesRead;
    }
    if (bytesFlushed>0){
      XTRACE(DIGIT, WAR, "Flushed %db of data still in digitizer buffer", bytesRead);
    }
}

void Digitizer::close() {
  XTRACE(DIGIT, DEB, "Closing digitizer %s", name().c_str());
  if (id == 0xaaaabbbb)  {
    return;
  }
  digitizer->freeReadoutBuffer(readoutBuffer);
  if (digitizer) {
    delete digitizer;
    digitizer = nullptr;
  }
}

bool Digitizer::ready() {
  caen::Digitizer::AcquisitionStatus acqStatus{
      digitizer->getAcquisitionStatus()};
  return acqStatus.boardReady() && acqStatus.PLLready();
}

void Digitizer::startAcquisition() {
  if (id == 0xaaaabbbb) {
    return;
  }
  // fixed minimum wait for digitizer to be ready (value determined experimentally):
  std::this_thread::sleep_for(std::chrono::milliseconds(175));
    // additional wait if necessary:
  while (!ready())
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  digitizer->startAcquisition();
}

void Digitizer::acquisition() {
  XTRACE(DIGIT, DEB, "Read at most %db data from %s", readoutBuffer.size, name().c_str());

  // NULL Digitizer "readout"
  if (id == 0xaaaabbbb) {
    memset(readoutBuffer.data, 0x00, 2048); // emulate readData() function
    (*(uint32_t *)(readoutBuffer.data +  0)) = 0xa0000030;  // magic value 0xa + size of
    (*(uint32_t *)(readoutBuffer.data +  4)) = 0x00000001;  // group mask 1
    (*(uint32_t *)(readoutBuffer.data +  8)) = 0x00000000;  // unused ?
    (*(uint32_t *)(readoutBuffer.data + 12)) = 0x00000000; // unused ?

    // Group 0 - channels 0 - 15
    (*(uint32_t *)(readoutBuffer.data + 16)) = 0x80000020; // MSB 1 + data size 32 bytes
    (*(uint32_t *)(readoutBuffer.data + 20)) = 0x60000001; // 0110 0 ....

    (*(uint32_t *)(readoutBuffer.data + 24)) = 0x01020304; // Time
    (*(uint32_t *)(readoutBuffer.data + 28)) = 0x00001000; // subch 0, charge 4096

    (*(uint32_t *)(readoutBuffer.data + 32)) = 0x01020305; // Time
    (*(uint32_t *)(readoutBuffer.data + 36)) = 0x00001000; // subch 0, charge 4096

    (*(uint32_t *)(readoutBuffer.data + 40)) = 0x01020306; // Time
    (*(uint32_t *)(readoutBuffer.data + 44)) = 0xf0001000; // subch 15, charge 4096

    readoutBuffer.dataSize = 48; // emulate readData() function
    stats.bytesRead += readoutBuffer.dataSize;

    DPPQDCEventIterator iterator{readoutBuffer};
    size_t events = dataHandler(iterator);
    stats.eventsFound += events;
    //usleep(1000000);
    return;
  }

  /* We use slave terminated mode like in the sample from CAEN Digitizer library
   * docs. */
  digitizer->readData(readoutBuffer, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT);
  uint32_t bytesRead = readoutBuffer.dataSize;
  XTRACE(DIGIT, DEB, "Read %db of acquired data", bytesRead);
  stats.readouts++;

  /* NOTE: check and skip if there's no actual events to handle */
  if (bytesRead < 1) {
    XTRACE(DIGIT, DEB, "No data to read - skip further handling.");
    return;
  }
  stats.bytesRead += bytesRead;

    // model- and firmware-dependent acquisition
    switch (digitizer->familyCode()){
    case CAEN_DGTZ_XX751_FAMILY_CODE:
      switch ((int)firmware) //Cast to int as long as CAEN_DGTZ_DPPFirmware_QDC is not part of the enumeration
        {
        case CAEN_DGTZ_NotDPPFirmware:
          {
          StdBLTEventIterator iterator{readoutBuffer};
          size_t events = dataHandler(iterator);
          stats.eventsFound += events;
          break;
          }
        default:
          throw std::runtime_error("Data acquisition not implemented by jadaq::Digitizer for firmware present on " + digitizer->modelName());
        }
      break;
    case CAEN_DGTZ_XX740_FAMILY_CODE:
      switch ((int)firmware) //Cast to int as long as CAEN_DGTZ_DPPFirmware_QDC is not part of the enumeration
        {
        case CAEN_DGTZ_DPPFirmware_PHA:
          throw std::runtime_error("PHA firmware not supported by jadaq::Digitizer on " + digitizer->modelName());
          break;
        case CAEN_DGTZ_DPPFirmware_PSD:
          throw std::runtime_error("PSD firmware not supported by jadaq::Digitizer on " + digitizer->modelName());
          break;
        case CAEN_DGTZ_DPPFirmware_CI:
          throw std::runtime_error("CI firmware not supported by jadaq::Digitizer on " + digitizer->modelName());
          break;
        case CAEN_DGTZ_DPPFirmware_ZLE:
          throw std::runtime_error("ZLE firmware not supported by jadaq::Digitizer on " + digitizer->modelName());
          break;
        case CAEN_DGTZ_DPPFirmware_QDC:
          {
            DPPQDCEventIterator iterator{readoutBuffer};
            size_t events = dataHandler(iterator);
            stats.eventsFound += events;
            break;
          }
        case CAEN_DGTZ_NotDPPFirmware:
          throw std::runtime_error("Non DPP firmware not supported by jadaq::Digitizer on " + digitizer->modelName());
          break;
        default:
          throw std::runtime_error("Unknown firmware type. Not supported by jadaq::Digitizer on " + digitizer->modelName());
        }
      break;
    default:
      throw std::runtime_error("Unknown digitizer type. Not supported by jadaq::Digitizer on " + digitizer->modelName());
    }

}
