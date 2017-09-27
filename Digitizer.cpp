//
// Created by troels on 9/19/17.
//

#include "Digitizer.hpp"
#include "StringConversion.hpp"
#include <regex>
#include <chrono>
#include <thread>

#define SET_CASE(D,F,V) \
    case F :            \
        D->set##F(V);   \
        break;

#define GET_CASE(D,F)        \
    case F :                    \
        return to_string(D->get##F());

#define SET_ICASE(D,F,C,V)   \
    case F :                 \
        D->set##F(C,V);      \
        break;

#define GET_ICASE(D,F,C)      \
    case F :                     \
        return to_string(D->get##F(C));

static void set_(caen::Digitizer* digitizer, FunctionID functionID, const std::string& value)
{
    switch(functionID)
    {
        SET_CASE(digitizer,MaxNumEventsBLT,s2ui(value))
        SET_CASE(digitizer,ChannelEnableMask,s2ui(value))
        SET_CASE(digitizer,GroupEnableMask,s2ui(value))
        SET_CASE(digitizer,DecimationFactor,s2ui(value))
        SET_CASE(digitizer,PostTriggerSize,s2ui(value))
        SET_CASE(digitizer,IOlevel,s2iol(value))
        SET_CASE(digitizer,AcquisitionMode,s2am(value))
        SET_CASE(digitizer,ExternalTriggerMode,s2tm(value))
        SET_CASE(digitizer,SWTriggerMode,s2tm(value))
        SET_CASE(digitizer,RunSynchronizationMode,s2rsm(value))
        SET_CASE(digitizer,OutputSignalMode,s2osm(value))
        SET_CASE(digitizer,DESMode,s2ed(value))
        SET_CASE(digitizer,DPPAcquisitionMode,s2cdam(value))
        SET_CASE(digitizer,DPPTriggerMode,s2dtm(value))
        SET_CASE(digitizer,RunDelay,s2ui(value))
        SET_CASE(digitizer,RecordLength,s2ui(value))
        SET_CASE(digitizer,NumEventsPerAggregate,s2ui(value))
        SET_CASE(digitizer,FixedBaseline,s2ui(value))
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
        SET_ICASE(digitizer,ChannelTriggerThreshold,index,s2tm(value))
        SET_ICASE(digitizer,GroupTriggerThreshold,index,s2tm(value))
        SET_ICASE(digitizer,ChannelGroupMask,index,s2tm(value))
        SET_ICASE(digitizer,TriggerPolarity,index,s2tp(value))
        SET_ICASE(digitizer,DPPPreTriggerSize,index,s2ui(value))
        SET_ICASE(digitizer,ChannelPulsePolarity,index,s2pp(value))
        SET_ICASE(digitizer,RecordLength,s2ui(value),index)
        SET_ICASE(digitizer,NumEventsPerAggregate,s2ui(value),index)
        SET_ICASE(digitizer,FixedBaseline,s2ui(value),index)
        default:
            throw std::invalid_argument{"Unknown Function"};
    }
}

static std::string get_(caen::Digitizer* digitizer, FunctionID functionID)
{
    switch(functionID)
    {
        GET_CASE(digitizer,MaxNumEventsBLT)
        GET_CASE(digitizer,ChannelEnableMask)
        GET_CASE(digitizer,GroupEnableMask)
        GET_CASE(digitizer,DecimationFactor)
        GET_CASE(digitizer,PostTriggerSize)
        GET_CASE(digitizer,IOlevel)
        GET_CASE(digitizer,AcquisitionMode)
        GET_CASE(digitizer,ExternalTriggerMode)
        GET_CASE(digitizer,SWTriggerMode)
        GET_CASE(digitizer,RunSynchronizationMode)
        GET_CASE(digitizer,OutputSignalMode)
        GET_CASE(digitizer,DESMode)
        GET_CASE(digitizer,DPPAcquisitionMode)
        GET_CASE(digitizer,DPPTriggerMode)
        GET_CASE(digitizer,RunDelay)
        GET_CASE(digitizer,RecordLength)
        GET_CASE(digitizer,NumEventsPerAggregate)
        default:
            throw std::invalid_argument{"Unknown Function"};

    }
}

static std::string get_(caen::Digitizer* digitizer, FunctionID functionID, int index)
{

    switch (functionID) {
        GET_ICASE(digitizer, ChannelDCOffset, index)
        GET_ICASE(digitizer, GroupDCOffset, index)
        GET_ICASE(digitizer, ChannelSelfTrigger, index)
        GET_ICASE(digitizer, GroupSelfTrigger, index)
        GET_ICASE(digitizer, ChannelTriggerThreshold, index)
        GET_ICASE(digitizer, GroupTriggerThreshold, index)
        GET_ICASE(digitizer, ChannelGroupMask, index)
        GET_ICASE(digitizer, TriggerPolarity, index)
        GET_ICASE(digitizer, DPPPreTriggerSize, index)
        GET_ICASE(digitizer, ChannelPulsePolarity, index)
        GET_ICASE(digitizer, RecordLength, index)
        GET_ICASE(digitizer, NumEventsPerAggregate, index)
        GET_ICASE(digitizer, FixedBaseline, index)
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
