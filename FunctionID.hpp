//
// Created by troels on 9/21/17.
//

#ifndef JADAQ_FUNCTIONID_HPP
#define JADAQ_FUNCTIONID_HPP

#include <string>

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

static inline bool needIndex(FunctionID id) { return id >= ChannelDCOffset; }
static inline FunctionID functionIDbegin() { return MaxNumEventsBLT; }
static inline FunctionID functionIDend() { return (FunctionID)((int)NumEventsPerAggregate+1); }
static inline FunctionID& operator++(FunctionID& id)
{
    id = (FunctionID)((int)id+1);
    return id;
}
static inline FunctionID operator++(FunctionID& id, int)
{
    FunctionID copy = id;
    id = (FunctionID)((int)id+1);
    return copy;
}

FunctionID functionID(std::string s);
namespace std {
    const string to_string(FunctionID id);
}


#endif //JADAQ_FUNCTIONID_HPP
