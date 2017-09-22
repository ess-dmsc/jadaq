//
// Created by troels on 9/21/17.
//

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
                MAP_ENTRY(DPPAcquisitionMode),
                MAP_ENTRY(DPPTriggerMode),
                MAP_ENTRY(ChannelDCOffset),
                MAP_ENTRY(GroupDCOffset),
                MAP_ENTRY(ChannelSelfTrigger),
                MAP_ENTRY(GroupSelfTrigger),
                MAP_ENTRY(ChannelTriggerThreshold),
                MAP_ENTRY(GroupTriggerThreshold),
                MAP_ENTRY(ChannelGroupMask),
                MAP_ENTRY(TriggerPolarity),
                MAP_ENTRY(DPPPreTriggerSize),
                MAP_ENTRY(ChannelPulsePolarity),
                MAP_ENTRY(RecordLength),
                MAP_ENTRY(NumEventsPerAggregate)
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
        CASE_TO_STR(DPPAcquisitionMode)
        CASE_TO_STR(DPPTriggerMode)
        CASE_TO_STR(ChannelDCOffset)
        CASE_TO_STR(GroupDCOffset)
        CASE_TO_STR(ChannelSelfTrigger)
        CASE_TO_STR(GroupSelfTrigger)
        CASE_TO_STR(ChannelTriggerThreshold)
        CASE_TO_STR(GroupTriggerThreshold)
        CASE_TO_STR(ChannelGroupMask)
        CASE_TO_STR(TriggerPolarity)
        CASE_TO_STR(DPPPreTriggerSize)
        CASE_TO_STR(ChannelPulsePolarity)
        CASE_TO_STR(RecordLength)
        CASE_TO_STR(NumEventsPerAggregate)
        default :
            throw std::invalid_argument{"Unknown function ID"};
    }
}


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
