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
    ZeroSuppressionMode,
    DPPAcquisitionMode,
    DPPTriggerMode,
    RunDelay,
    // Channel/group setting
    ChannelDCOffset,
    GroupDCOffset,
    ChannelSelfTrigger,
    GroupSelfTrigger,
    ChannelTriggerThreshold,
    GroupTriggerThreshold,
    ChannelGroupMask,
    TriggerPolarity,
    ChannelPulsePolarity,
    // Channel/group optional
    DPPPreTriggerSize,
    RecordLength,
    NumEventsPerAggregate,
    GateWidth,
    FixedBaseline,
};

static inline bool needIndex(FunctionID id) { return id >= ChannelDCOffset; }
FunctionID functionIDbegin();
FunctionID functionIDend();
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
static inline FunctionID& operator--(FunctionID& id)
{
    id = (FunctionID)((int)id-1);
    return id;
}
static inline FunctionID operator--(FunctionID& id, int)
{
    FunctionID copy = id;
    id = (FunctionID)((int)id-1);
    return copy;
}

FunctionID functionID(std::string s);
const std::string to_string(FunctionID id);



#endif //JADAQ_FUNCTIONID_HPP
