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
 * Map JADAQ function names to configuration helpers.
 *
 */

#include "FunctionID.hpp"
#include <unordered_map>
#include <iostream>

#define MAP_ENTRY(F) {#F,(F)}
static std::unordered_map<std::string,FunctionID > functionMap =
        {
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
                MAP_ENTRY(EasyBoardConfiguration),
                MAP_ENTRY(AggregateOrganization),
                MAP_ENTRY(AcquisitionControl),
                MAP_ENTRY(EasyAcquisitionControl),
                MAP_ENTRY(AcquisitionStatus),
                MAP_ENTRY(EasyAcquisitionStatus),
                MAP_ENTRY(GlobalTriggerMask),
                MAP_ENTRY(EasyGlobalTriggerMask),
                MAP_ENTRY(FrontPanelTRGOUTEnableMask),
                MAP_ENTRY(EasyFrontPanelTRGOUTEnableMask),
                MAP_ENTRY(FrontPanelIOControl),
                MAP_ENTRY(EasyFrontPanelIOControl),
                MAP_ENTRY(ROCFPGAFirmwareRevision),
                MAP_ENTRY(EasyROCFPGAFirmwareRevision),
                MAP_ENTRY(FanSpeedControl),
                MAP_ENTRY(DisableExternalTrigger),
                MAP_ENTRY(RunStartStopDelay),
                MAP_ENTRY(ReadoutControl),
                MAP_ENTRY(AggregateNumberPerBLT),
                MAP_ENTRY(ChannelDCOffset),
                MAP_ENTRY(GroupDCOffset),
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
                MAP_ENTRY(GateWidth),
                MAP_ENTRY(GateOffset),
                MAP_ENTRY(FixedBaseline),
                MAP_ENTRY(DPPAlgorithmControl),
                MAP_ENTRY(EasyDPPAlgorithmControl),
                MAP_ENTRY(TriggerHoldOffWidth),
                MAP_ENTRY(ShapedTriggerWidth),
        };


#define CASE_TO_STR(X) case (X) : return std::string(#X);

const std::string to_string(FunctionID id) {
    switch (id) {
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
        CASE_TO_STR(EasyBoardConfiguration)
        CASE_TO_STR(AggregateOrganization)
        CASE_TO_STR(AcquisitionControl)
        CASE_TO_STR(EasyAcquisitionControl)
        CASE_TO_STR(AcquisitionStatus)
        CASE_TO_STR(EasyAcquisitionStatus)
        CASE_TO_STR(GlobalTriggerMask)
        CASE_TO_STR(EasyGlobalTriggerMask)
        CASE_TO_STR(FrontPanelTRGOUTEnableMask)
        CASE_TO_STR(EasyFrontPanelTRGOUTEnableMask)
        CASE_TO_STR(FrontPanelIOControl)
        CASE_TO_STR(EasyFrontPanelIOControl)
        CASE_TO_STR(ROCFPGAFirmwareRevision)
        CASE_TO_STR(EasyROCFPGAFirmwareRevision)
        CASE_TO_STR(FanSpeedControl)
        CASE_TO_STR(DisableExternalTrigger)
        CASE_TO_STR(RunStartStopDelay)
        CASE_TO_STR(ReadoutControl)
        CASE_TO_STR(AggregateNumberPerBLT)
        CASE_TO_STR(ChannelDCOffset)
        CASE_TO_STR(GroupDCOffset)
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
        CASE_TO_STR(GateWidth)
        CASE_TO_STR(GateOffset)
        CASE_TO_STR(FixedBaseline)
        CASE_TO_STR(DPPAlgorithmControl)
        CASE_TO_STR(EasyDPPAlgorithmControl)
        CASE_TO_STR(TriggerHoldOffWidth)
        CASE_TO_STR(ShapedTriggerWidth)
        default :
            throw std::invalid_argument{"Unknown function ID"};
    }
}

FunctionID functionIDbegin() { return (FunctionID)0; }
FunctionID functionIDend() { return (FunctionID)functionMap.size(); }



FunctionID functionID(std::string s)
{
    auto fid = functionMap.find(s);
    if (fid == functionMap.end())
    {
        std::cerr << "Did not find function: " << s << std::endl;
        throw std::invalid_argument{"No function by that name"};
    }
    else
        return fid->second;
}
