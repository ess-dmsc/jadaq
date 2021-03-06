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
 * Map JADAQ function names to configuration helpers.
 *
 */

#include "FunctionID.hpp"
#include <iostream>
#include <unordered_map>

#define MAP_ENTRY(F)                                                           \
  { #F, (F) }
static std::unordered_map<std::string, FunctionID> functionMap = {
    MAP_ENTRY(Register),
    MAP_ENTRY(MaxNumEventsBLT),
    MAP_ENTRY(ChannelEnableMask),
    MAP_ENTRY(GroupEnableMask),
    MAP_ENTRY(DecimationFactor),
    MAP_ENTRY(PostTriggerSize),
    MAP_ENTRY(IOlevel),
    MAP_ENTRY(AcquisitionMode),
    MAP_ENTRY(ExternalTriggerMode),
    MAP_ENTRY(SWTriggerMode),
    MAP_ENTRY(RunSynchronizationMode),
    MAP_ENTRY(OutputSignalMode),
    MAP_ENTRY(DESMode),
    MAP_ENTRY(ZeroSuppressionMode),
    MAP_ENTRY(AnalogMonOutput),
    MAP_ENTRY(AnalogInspectionMonParams),
    MAP_ENTRY(EventPackaging),
    MAP_ENTRY(TriggerCountingMode),
    MAP_ENTRY(FastTriggerDigitizing),
    MAP_ENTRY(FastTriggerMode),
    MAP_ENTRY(DRS4SamplingFrequency),
    MAP_ENTRY(DPPAcquisitionMode),
    MAP_ENTRY(DPPTriggerMode),
    MAP_ENTRY(MaxNumAggregatesBLT),
    MAP_ENTRY(SAMCorrectionLevel),
    MAP_ENTRY(SAMSamplingFrequency),
    MAP_ENTRY(SAMAcquisitionMode),
    MAP_ENTRY(TriggerLogic),
    MAP_ENTRY(BoardConfiguration),
    MAP_ENTRY(DPPAggregateOrganization),
    MAP_ENTRY(AcquisitionControl),
    MAP_ENTRY(AcquisitionStatus),
    MAP_ENTRY(GlobalTriggerMask),
    MAP_ENTRY(FrontPanelTRGOUTEnableMask),
    MAP_ENTRY(FrontPanelIOControl),
    MAP_ENTRY(ROCFPGAFirmwareRevision),
    MAP_ENTRY(EventSize),
    MAP_ENTRY(FanSpeedControl),
    MAP_ENTRY(DPPDisableExternalTrigger),
    MAP_ENTRY(RunStartStopDelay),
    MAP_ENTRY(ReadoutControl),
    MAP_ENTRY(ReadoutStatus),
    MAP_ENTRY(Scratch),
    MAP_ENTRY(DPPAggregateNumberPerBLT),
    MAP_ENTRY(ChannelDCOffset),
    MAP_ENTRY(GroupDCOffset),
    MAP_ENTRY(AMCFirmwareRevision),
    MAP_ENTRY(ChannelSelfTrigger),
    MAP_ENTRY(GroupSelfTrigger),
    MAP_ENTRY(ChannelTriggerThreshold),
    MAP_ENTRY(GroupTriggerThreshold),
    MAP_ENTRY(ChannelGroupMask),
    MAP_ENTRY(TriggerPolarity),
    MAP_ENTRY(GroupFastTriggerThreshold),
    MAP_ENTRY(GroupFastTriggerDCOffset),
    MAP_ENTRY(ChannelZSParams),
    MAP_ENTRY(SAMPostTriggerSize),
    MAP_ENTRY(SAMTriggerCountVetoParam),
    MAP_ENTRY(DPPPreTriggerSize),
    MAP_ENTRY(ChannelPulsePolarity),
    MAP_ENTRY(RecordLength),
    MAP_ENTRY(NumEventsPerAggregate),
    MAP_ENTRY(DPPGateWidth),
    MAP_ENTRY(DPPGateOffset),
    MAP_ENTRY(DPPFixedBaseline),
    MAP_ENTRY(DPPAlgorithmControl),
    MAP_ENTRY(DPPTriggerHoldOffWidth),
    MAP_ENTRY(DPPShapedTriggerWidth),
};

#define CASE_TO_STR(X)                                                         \
  case (X):                                                                    \
    return std::string(#X);

const std::string to_string(FunctionID id) {
  switch (id) {
    CASE_TO_STR(Register)
    CASE_TO_STR(MaxNumEventsBLT)
    CASE_TO_STR(ChannelEnableMask)
    CASE_TO_STR(GroupEnableMask)
    CASE_TO_STR(DecimationFactor)
    CASE_TO_STR(PostTriggerSize)
    CASE_TO_STR(IOlevel)
    CASE_TO_STR(AcquisitionMode)
    CASE_TO_STR(ExternalTriggerMode)
    CASE_TO_STR(SWTriggerMode)
    CASE_TO_STR(RunSynchronizationMode)
    CASE_TO_STR(OutputSignalMode)
    CASE_TO_STR(DESMode)
    CASE_TO_STR(ZeroSuppressionMode)
    CASE_TO_STR(AnalogMonOutput)
    CASE_TO_STR(AnalogInspectionMonParams)
    CASE_TO_STR(EventPackaging)
    CASE_TO_STR(TriggerCountingMode)
    CASE_TO_STR(FastTriggerDigitizing)
    CASE_TO_STR(FastTriggerMode)
    CASE_TO_STR(DRS4SamplingFrequency)
    CASE_TO_STR(DPPAcquisitionMode)
    CASE_TO_STR(DPPTriggerMode)
    CASE_TO_STR(MaxNumAggregatesBLT)
    CASE_TO_STR(SAMCorrectionLevel)
    CASE_TO_STR(SAMSamplingFrequency)
    CASE_TO_STR(SAMAcquisitionMode)
    CASE_TO_STR(TriggerLogic)
    CASE_TO_STR(BoardConfiguration)
    CASE_TO_STR(DPPAggregateOrganization)
    CASE_TO_STR(AcquisitionControl)
    CASE_TO_STR(AcquisitionStatus)
    CASE_TO_STR(GlobalTriggerMask)
    CASE_TO_STR(FrontPanelTRGOUTEnableMask)
    CASE_TO_STR(FrontPanelIOControl)
    CASE_TO_STR(ROCFPGAFirmwareRevision)
    CASE_TO_STR(EventSize)
    CASE_TO_STR(FanSpeedControl)
    CASE_TO_STR(DPPDisableExternalTrigger)
    CASE_TO_STR(RunStartStopDelay)
    CASE_TO_STR(ReadoutControl)
    CASE_TO_STR(ReadoutStatus)
    CASE_TO_STR(Scratch)
    CASE_TO_STR(DPPAggregateNumberPerBLT)
    CASE_TO_STR(ChannelDCOffset)
    CASE_TO_STR(GroupDCOffset)
    CASE_TO_STR(AMCFirmwareRevision)
    CASE_TO_STR(ChannelSelfTrigger)
    CASE_TO_STR(GroupSelfTrigger)
    CASE_TO_STR(ChannelTriggerThreshold)
    CASE_TO_STR(GroupTriggerThreshold)
    CASE_TO_STR(ChannelGroupMask)
    CASE_TO_STR(TriggerPolarity)
    CASE_TO_STR(GroupFastTriggerThreshold)
    CASE_TO_STR(GroupFastTriggerDCOffset)
    CASE_TO_STR(ChannelZSParams)
    CASE_TO_STR(SAMPostTriggerSize)
    CASE_TO_STR(SAMTriggerCountVetoParam)
    CASE_TO_STR(DPPPreTriggerSize)
    CASE_TO_STR(ChannelPulsePolarity)
    CASE_TO_STR(RecordLength)
    CASE_TO_STR(NumEventsPerAggregate)
    CASE_TO_STR(DPPGateWidth)
    CASE_TO_STR(DPPGateOffset)
    CASE_TO_STR(DPPFixedBaseline)
    CASE_TO_STR(DPPAlgorithmControl)
    CASE_TO_STR(DPPTriggerHoldOffWidth)
    CASE_TO_STR(DPPShapedTriggerWidth)
  default:
    std::cerr << "to_string did not find function id: " << id << std::endl;
    throw std::invalid_argument{"Unknown function ID"};
  }
}

FunctionID functionID(std::string s) {
  auto fid = functionMap.find(s);
  if (fid == functionMap.end()) {
    std::cerr << "functionID did not find function: " << s << std::endl;
    throw std::invalid_argument{"No function by that name"};
  } else
    return fid->second;
}
