//
// Created by troels on 9/18/17.
//

#ifndef JADAQ_SETTINGS_HPP
#define JADAQ_SETTINGS_HPP

#include <string>
#include <unordered_map>
#include "caen.hpp"

enum FunctionID {
    // Global i.e. no channel/group
    MaxNumEventsBLT,
    ChannelEnableMask,
    GroupEnableMask,
    DecimationFactor,
    PostTriggerSize,
    IOlevel,
    AcquisitionMode,
    ExternalTriggerMode,
    SWTriggerMode,
    RunSynchronizationMode,
    OutputSignalMode,
    DESMode,
    DPPAcquisitionMode,
    DPPTriggerMode,
    // Channel/group setting
    ChannelDCOffset,
    GroupDCOffset,
    ChannelSelfTrigger,
    GroupSelfTrigger,
    ChannelTriggerThreshold,
    GroupTriggerThreshold,
    ChannelGroupMask,
    TriggerPolarity,
    DPPPreTriggerSize,
    ChannelPulsePolarity,
    // Channel/group optional
    RecordLength,
    NumEventsPerAggregate

};

void set(caen::Digitizer* digitizer, FunctionID functionID, std::string value);
void set(caen::Digitizer* digitizer, FunctionID functionID, int index, std::string value);
std::string get(caen::Digitizer* digitizer, FunctionID functionID);
std::string get(caen::Digitizer* digitizer, FunctionID functionID, int index);

#endif //JADAQ_SETTINGS_HPP
