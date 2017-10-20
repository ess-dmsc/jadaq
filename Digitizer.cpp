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

#include "Digitizer.hpp"
#include "StringConversion.hpp"
#include <regex>
#include <chrono>
#include <thread>

#define MAX_GROUPS 8

#define SET_CASE(D,F,V) \
    case F :            \
        D->set##F(V);   \
        break;

#define GET_CASE(D,F,TS)        \
    case F :                    \
        return TS(D->get##F());

#define SET_ICASE(D,F,C,V)   \
    case F :                 \
        D->set##F(C,V);      \
        break;

#define GET_ICASE(D,F,C,TS)      \
    case F :                     \
        return TS(D->get##F(C));

static void set_(caen::Digitizer* digitizer, FunctionID functionID, const std::string& value)
{
    switch(functionID)
    {
        SET_CASE(digitizer,MaxNumEventsBLT,s2ui(value))
        SET_CASE(digitizer,ChannelEnableMask,bs2ui(value))
        SET_CASE(digitizer,GroupEnableMask,bs2ui(value))
        SET_CASE(digitizer,DecimationFactor,s2ui(value))
        SET_CASE(digitizer,PostTriggerSize,s2ui(value))
        SET_CASE(digitizer,IOlevel,s2iol(value))
        SET_CASE(digitizer,AcquisitionMode,s2am(value))
        SET_CASE(digitizer,ExternalTriggerMode,s2tm(value))
        SET_CASE(digitizer,SWTriggerMode,s2tm(value))
        SET_CASE(digitizer,RunSynchronizationMode,s2rsm(value))
        SET_CASE(digitizer,OutputSignalMode,s2osm(value))
        SET_CASE(digitizer,DESMode,s2ed(value))
        SET_CASE(digitizer,ZeroSuppressionMode,s2zsm(value))
        SET_CASE(digitizer,AnalogMonOutput,s2amom(value))
        SET_CASE(digitizer,AnalogInspectionMonParams,s2aimp(value))
        SET_CASE(digitizer,EventPackaging,s2ed(value))
        SET_CASE(digitizer,FastTriggerDigitizing,s2ed(value))
        SET_CASE(digitizer,FastTriggerMode,s2tm(value))
        SET_CASE(digitizer,DRS4SamplingFrequency,s2drsff(value))
        SET_CASE(digitizer,DPPAcquisitionMode,s2cdam(value))
        SET_CASE(digitizer,DPPTriggerMode,s2dtm(value))
        SET_CASE(digitizer,MaxNumAggregatesBLT,s2ui(value))
        SET_CASE(digitizer,SAMCorrectionLevel,s2samcl(value))
        SET_CASE(digitizer,SAMSamplingFrequency,s2samf(value))
        SET_CASE(digitizer,SAMAcquisitionMode,s2samam(value))
        SET_CASE(digitizer,TriggerLogic,s2tlp(value))
        SET_CASE(digitizer,BoardConfiguration,s2ui(value))
        SET_CASE(digitizer,EasyBoardConfiguration,s2ebc(value))
        SET_CASE(digitizer,EasyDPPBoardConfiguration,s2edbc(value))
        SET_CASE(digitizer,DPPAggregateOrganization,s2ui(value))
        SET_CASE(digitizer,AcquisitionControl,s2ui(value))
        SET_CASE(digitizer,EasyAcquisitionControl,s2eac(value))
        SET_CASE(digitizer,EasyDPPAcquisitionControl,s2edac(value))
        SET_CASE(digitizer,GlobalTriggerMask,s2ui(value))
        SET_CASE(digitizer,EasyGlobalTriggerMask,s2egtm(value))
        SET_CASE(digitizer,EasyDPPGlobalTriggerMask,s2edgtm(value))
        SET_CASE(digitizer,FrontPanelTRGOUTEnableMask,s2ui(value))
        SET_CASE(digitizer,EasyFrontPanelTRGOUTEnableMask,s2efptoem(value))
        SET_CASE(digitizer,EasyDPPFrontPanelTRGOUTEnableMask,s2edfptoem(value))
        SET_CASE(digitizer,FrontPanelIOControl,s2ui(value))
        SET_CASE(digitizer,EasyFrontPanelIOControl,s2efpioc(value))
        SET_CASE(digitizer,EasyDPPFrontPanelIOControl,s2efpioc(value))
        SET_CASE(digitizer,FanSpeedControl,s2ui(value))
        SET_CASE(digitizer,EasyFanSpeedControl,s2efsc(value))
        SET_CASE(digitizer,EasyDPPFanSpeedControl,s2efsc(value))
        SET_CASE(digitizer,DPPDisableExternalTrigger,s2ui(value))
        SET_CASE(digitizer,RunStartStopDelay,s2ui(value))
        SET_CASE(digitizer,ReadoutControl,s2ui(value))
        SET_CASE(digitizer,DPPAggregateNumberPerBLT,s2ui(value))
        SET_CASE(digitizer,RecordLength,s2ui(value))
        SET_CASE(digitizer,NumEventsPerAggregate,s2ui(value))
        SET_CASE(digitizer,DPPGateWidth,s2ui(value))
        SET_CASE(digitizer,DPPGateOffset,s2ui(value))
        SET_CASE(digitizer,DPPFixedBaseline,s2ui(value))
        SET_CASE(digitizer,DPPAlgorithmControl,s2ui(value))
        SET_CASE(digitizer,EasyDPPAlgorithmControl,s2edppac(value))
        SET_CASE(digitizer,DPPTriggerHoldOffWidth,s2ui(value))
        SET_CASE(digitizer,DPPShapedTriggerWidth,s2ui(value))
        default:
            throw std::invalid_argument{"Unknown Function"};

    }
}

static void set_(caen::Digitizer* digitizer, FunctionID functionID, int index, const std::string& value) {
    switch (functionID) {
        SET_ICASE(digitizer,ChannelDCOffset,index,s2ui(value))
        SET_ICASE(digitizer,GroupDCOffset,index,s2ui(value))
        SET_ICASE(digitizer,ChannelSelfTrigger,index,s2tm(value))
        SET_ICASE(digitizer,GroupSelfTrigger,index,s2tm(value))
        SET_ICASE(digitizer,ChannelTriggerThreshold,index,s2ui(value))
        SET_ICASE(digitizer,GroupTriggerThreshold,index,s2ui(value))
        SET_ICASE(digitizer,ChannelGroupMask,index,s2ui(value))
        SET_ICASE(digitizer,TriggerPolarity,index,s2tp(value))
        SET_ICASE(digitizer,GroupFastTriggerThreshold,index,s2ui(value))
        SET_ICASE(digitizer,GroupFastTriggerDCOffset,index,s2ui(value))
        SET_ICASE(digitizer,ChannelZSParams,index,s2zsp(value))
        SET_ICASE(digitizer,SAMPostTriggerSize,index,s2i(value))
        SET_ICASE(digitizer,SAMTriggerCountVetoParam,index,s2samtcvp(value))
        SET_ICASE(digitizer,DPPPreTriggerSize,index,s2ui(value))
        SET_ICASE(digitizer,ChannelPulsePolarity,index,s2pp(value))
        SET_ICASE(digitizer,RecordLength,index,s2ui(value))
        SET_ICASE(digitizer,NumEventsPerAggregate,index,s2ui(value))
        SET_ICASE(digitizer,DPPGateWidth,index,s2ui(value))
        SET_ICASE(digitizer,DPPGateOffset,index,s2ui(value))
        SET_ICASE(digitizer,DPPFixedBaseline,index,s2ui(value))
        SET_ICASE(digitizer,DPPAlgorithmControl,index,s2ui(value))
        SET_ICASE(digitizer,EasyDPPAlgorithmControl,index,s2edppac(value))
        SET_ICASE(digitizer,DPPTriggerHoldOffWidth,index,s2ui(value))
        SET_ICASE(digitizer,DPPShapedTriggerWidth,index,s2ui(value))
        default:
            throw std::invalid_argument{"Unknown Function"};
    }
}

static std::string get_(caen::Digitizer* digitizer, FunctionID functionID)
{
    switch(functionID)
    {
        GET_CASE(digitizer,MaxNumEventsBLT,to_string)
        GET_CASE(digitizer,ChannelEnableMask,bin_string)
        GET_CASE(digitizer,GroupEnableMask,bin_string<MAX_GROUPS>)
        GET_CASE(digitizer,DecimationFactor,to_string)
        GET_CASE(digitizer,PostTriggerSize,to_string)
        GET_CASE(digitizer,IOlevel,to_string)
        GET_CASE(digitizer,AcquisitionMode,to_string)
        GET_CASE(digitizer,ExternalTriggerMode,to_string)
        GET_CASE(digitizer,SWTriggerMode,to_string)
        GET_CASE(digitizer,RunSynchronizationMode,to_string)
        GET_CASE(digitizer,OutputSignalMode,to_string)
        GET_CASE(digitizer,DESMode,to_string)
        GET_CASE(digitizer,ZeroSuppressionMode,to_string)
        GET_CASE(digitizer,AnalogMonOutput,to_string)
        GET_CASE(digitizer,AnalogInspectionMonParams,to_string)
        GET_CASE(digitizer,EventPackaging,to_string)
        GET_CASE(digitizer,FastTriggerDigitizing,to_string)
        GET_CASE(digitizer,FastTriggerMode,to_string)
        GET_CASE(digitizer,DRS4SamplingFrequency,to_string)
        GET_CASE(digitizer,DPPAcquisitionMode,to_string)
        GET_CASE(digitizer,DPPTriggerMode,to_string)
        GET_CASE(digitizer,MaxNumAggregatesBLT,to_string)
        GET_CASE(digitizer,SAMCorrectionLevel,to_string)
        GET_CASE(digitizer,SAMSamplingFrequency,to_string)
        GET_CASE(digitizer,SAMAcquisitionMode,to_string)
        GET_CASE(digitizer,TriggerLogic,to_string)
        GET_CASE(digitizer,BoardConfiguration,to_string)
        GET_CASE(digitizer,EasyBoardConfiguration,to_string)
        GET_CASE(digitizer,EasyDPPBoardConfiguration,to_string)
        GET_CASE(digitizer,DPPAggregateOrganization,to_string)
        GET_CASE(digitizer,AcquisitionControl,to_string)
        GET_CASE(digitizer,EasyAcquisitionControl,to_string)
        GET_CASE(digitizer,EasyDPPAcquisitionControl,to_string)
        GET_CASE(digitizer,AcquisitionStatus,to_string)
        GET_CASE(digitizer,EasyAcquisitionStatus,to_string)
        GET_CASE(digitizer,EasyDPPAcquisitionStatus,to_string)
        GET_CASE(digitizer,GlobalTriggerMask,to_string)
        GET_CASE(digitizer,EasyGlobalTriggerMask,to_string)
        GET_CASE(digitizer,EasyDPPGlobalTriggerMask,to_string)
        GET_CASE(digitizer,FrontPanelTRGOUTEnableMask,to_string)
        GET_CASE(digitizer,EasyFrontPanelTRGOUTEnableMask,to_string)
        GET_CASE(digitizer,EasyDPPFrontPanelTRGOUTEnableMask,to_string)
        GET_CASE(digitizer,FrontPanelIOControl,to_string)
        GET_CASE(digitizer,EasyFrontPanelIOControl,to_string)
        GET_CASE(digitizer,EasyDPPFrontPanelIOControl,to_string)
        GET_CASE(digitizer,ROCFPGAFirmwareRevision,to_string)
        GET_CASE(digitizer,EasyROCFPGAFirmwareRevision,to_string)
        GET_CASE(digitizer,EasyDPPROCFPGAFirmwareRevision,to_string)
        GET_CASE(digitizer,EventSize,to_string)
        GET_CASE(digitizer,FanSpeedControl,to_string)
        GET_CASE(digitizer,EasyFanSpeedControl,to_string)
        GET_CASE(digitizer,EasyDPPFanSpeedControl,to_string)
        GET_CASE(digitizer,DPPDisableExternalTrigger,to_string)
        GET_CASE(digitizer,RunStartStopDelay,to_string)
        GET_CASE(digitizer,ReadoutControl,to_string)
        GET_CASE(digitizer,DPPAggregateNumberPerBLT,to_string)
        GET_CASE(digitizer,RecordLength,to_string)
        GET_CASE(digitizer,NumEventsPerAggregate,to_string)
        default:
            throw std::invalid_argument{"Unknown Function"};

    }
}

static std::string get_(caen::Digitizer* digitizer, FunctionID functionID, int index)
{

    switch (functionID) {
        GET_ICASE(digitizer,ChannelDCOffset,index,to_string)
        GET_ICASE(digitizer,GroupDCOffset,index,to_string)
        GET_ICASE(digitizer,AMCFirmwareRevision,index,to_string)
        GET_ICASE(digitizer,EasyAMCFirmwareRevision,index,to_string)
        GET_ICASE(digitizer,EasyDPPAMCFirmwareRevision,index,to_string)
        GET_ICASE(digitizer,ChannelSelfTrigger,index,to_string)
        GET_ICASE(digitizer,GroupSelfTrigger,index,to_string)
        GET_ICASE(digitizer,ChannelTriggerThreshold,index,to_string)
        GET_ICASE(digitizer,GroupTriggerThreshold,index,to_string)
        GET_ICASE(digitizer,ChannelGroupMask,index,to_string)
        GET_ICASE(digitizer,TriggerPolarity,index,to_string)
        GET_ICASE(digitizer,GroupFastTriggerThreshold,index,to_string)
        GET_ICASE(digitizer,GroupFastTriggerDCOffset,index,to_string)
        GET_ICASE(digitizer,ChannelZSParams,index,to_string)
        GET_ICASE(digitizer,SAMPostTriggerSize,index,to_string)
        GET_ICASE(digitizer,SAMTriggerCountVetoParam,index,to_string)
        GET_ICASE(digitizer,DPPPreTriggerSize,index,to_string)
        GET_ICASE(digitizer,ChannelPulsePolarity,index,to_string)
        GET_ICASE(digitizer,RecordLength,index,to_string)
        GET_ICASE(digitizer,NumEventsPerAggregate,index,to_string)
        GET_ICASE(digitizer,DPPGateWidth,index,to_string)
        GET_ICASE(digitizer,DPPGateOffset,index,to_string)
        GET_ICASE(digitizer,DPPFixedBaseline,index,to_string)
        GET_ICASE(digitizer,DPPAlgorithmControl,index,to_string)
        GET_ICASE(digitizer,EasyDPPAlgorithmControl,index,to_string)
        GET_ICASE(digitizer,DPPTriggerHoldOffWidth,index,to_string)
        GET_ICASE(digitizer,DPPShapedTriggerWidth,index,to_string)
        default:
            throw std::invalid_argument{"Unknown Function"};
    }
}

template <typename R, typename F>
static R backOffRepeat(F fun, int retry=3,  std::chrono::milliseconds grace=std::chrono::milliseconds(1))
{
    while (true) {
        try {
            return fun();
        }
        catch (caen::Error &e) {
            if (e.code() == CAEN_DGTZ_CommError && retry-- > 0) {
                std::this_thread::sleep_for(grace);
                grace *= 10;
            } else throw;
        }
    }
}

std::string Digitizer::get(FunctionID functionID, int index)
{
    return backOffRepeat<std::string>([this,&functionID,&index](){ return get_(digitizer,functionID,index); });
}

std::string Digitizer::get(FunctionID functionID)
{
    return backOffRepeat<std::string>([this,&functionID](){ return get_(digitizer,functionID); });
}

void Digitizer::set(FunctionID functionID, int index, std::string value)
{
    return backOffRepeat<void>([this,&functionID,&index,&value](){ return set_(digitizer,functionID,index,value); });
}

void Digitizer::set(FunctionID functionID, std::string value)
{
    return backOffRepeat<void>([this,&functionID,&value](){ return set_(digitizer,functionID,value); });
}
