//
// Created by troels on 9/19/17.
//

#include "Digitizer.hpp"
#include <regex>

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

FunctionID Digitizer::functionID(std::string s)
{
    auto fid = functionMap.find(s);
    if (fid == functionMap.end())
        throw std::invalid_argument{"No function by that name"};
    else
        return fid->second;
}

const char* Digitizer::functionName(FunctionID id) {
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

static unsigned int s2ui(const std::string& s)
{ return std::stoi (s,nullptr,0); }
static std::string ui2s(const unsigned int v)
{ return std::to_string(v); }

static CAEN_DGTZ_IOLevel_t s2iol(const std::string& s)
{
    return (CAEN_DGTZ_IOLevel_t)std::stoi(s,nullptr,0);
}
static CAEN_DGTZ_AcqMode_t s2am(const std::string& s)
{
    return (CAEN_DGTZ_AcqMode_t)std::stoi(s,nullptr,0);
}
static CAEN_DGTZ_TriggerMode_t s2tm(const std::string& s)
{
    return (CAEN_DGTZ_TriggerMode_t)std::stoi(s,nullptr,0);
}
static CAEN_DGTZ_DPP_TriggerMode_t s2dtm(const std::string& s)
{
    return (CAEN_DGTZ_DPP_TriggerMode_t)std::stoi(s,nullptr,0);
}
static CAEN_DGTZ_RunSyncMode_t s2rsm(const std::string& s)
{
    return (CAEN_DGTZ_RunSyncMode_t)std::stoi(s,nullptr,0);
}
static CAEN_DGTZ_OutputSignalMode_t s2osm(const std::string& s)
{
    return (CAEN_DGTZ_OutputSignalMode_t)std::stoi(s,nullptr,0);
}
static CAEN_DGTZ_EnaDis_t s2ed(const std::string& s)
{
    return (CAEN_DGTZ_EnaDis_t)std::stoi(s,nullptr,0);
}
static CAEN_DGTZ_TriggerPolarity_t s2tp(const std::string& s)
{
    return (CAEN_DGTZ_TriggerPolarity_t)std::stoi(s,nullptr,0);
}
static CAEN_DGTZ_PulsePolarity_t s2pp(const std::string& s)
{
    return (CAEN_DGTZ_PulsePolarity_t)std::stoi(s,nullptr,0);
}
static CAEN_DGTZ_DPP_AcqMode_t s2dam_(const std::string& s)
{
    return (CAEN_DGTZ_DPP_AcqMode_t)std::stoi(s,nullptr,0);
}
static CAEN_DGTZ_DPP_SaveParam_t s2sp(const std::string& s)
{
    return (CAEN_DGTZ_DPP_SaveParam_t)std::stoi(s,nullptr,0);
}
static caen::DPPAcquisitionMode s2dam(const std::string& s)
{
    std::regex rx("\\{(\\w+),(\\w+)\\}");
    std::smatch match;
    if (std::regex_search(s, match, rx))
    {
        return caen::DPPAcquisitionMode{s2dam_(match[1]),s2sp(match[2])};
    }
    throw std::invalid_argument{"Invalid DPPAcquisitionMode"};
}
static std::string dam2s(const caen::DPPAcquisitionMode& dam)
{
    std::stringstream ss;
    ss << "{" << ui2s(dam.param) << "," << ui2s(dam.mode) << "}";
    return ss.str();
}

#define SET_CASE(D,F,V) \
    case F :            \
        D->set##F(V);   \
        break;

#define GET_CASE(D,F,SF)        \
    case F :                    \
        return SF(D->get##F());

#define SET_ICASE(D,F,C,V)   \
    case F :                 \
        D->set##F(C,V);      \
        break;

#define GET_ICASE(D,F,C,SF)      \
    case F :                     \
        return SF(D->get##F(C));

void Digitizer::set(FunctionID functionID, std::string value)
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
        SET_CASE(digitizer,DPPAcquisitionMode,s2dam(value))
        SET_CASE(digitizer,DPPTriggerMode,s2dtm(value))
        SET_CASE(digitizer,RecordLength,s2ui(value))
        SET_CASE(digitizer,NumEventsPerAggregate,s2ui(value))
        default:
            throw std::invalid_argument{"Unknown Function"};

    }
}

void Digitizer::set(FunctionID functionID, int index, std::string value) {
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
        default:
            throw std::invalid_argument{"Unknown Function"};
    }
}

std::string Digitizer::get(FunctionID functionID)
{
    switch(functionID)
    {
        GET_CASE(digitizer,MaxNumEventsBLT,ui2s)
        GET_CASE(digitizer,ChannelEnableMask,ui2s)
        GET_CASE(digitizer,GroupEnableMask,ui2s)
        GET_CASE(digitizer,DecimationFactor,ui2s)
        GET_CASE(digitizer,PostTriggerSize,ui2s)
        GET_CASE(digitizer,IOlevel,ui2s)
        GET_CASE(digitizer,AcquisitionMode,ui2s)
        GET_CASE(digitizer,ExternalTriggerMode,ui2s)
        GET_CASE(digitizer,SWTriggerMode,ui2s)
        GET_CASE(digitizer,RunSynchronizationMode,ui2s)
        GET_CASE(digitizer,OutputSignalMode,ui2s)
        GET_CASE(digitizer,DESMode,ui2s)
        GET_CASE(digitizer,DPPAcquisitionMode,dam2s)
        GET_CASE(digitizer,DPPTriggerMode,ui2s)
        GET_CASE(digitizer,RecordLength,ui2s)
        GET_CASE(digitizer,NumEventsPerAggregate,ui2s)
        default:
            throw std::invalid_argument{"Unknown Function"};

    }
}

std::string Digitizer::get(FunctionID functionID, int index)
{
    switch(functionID)
    {
        GET_ICASE(digitizer,ChannelDCOffset,index,ui2s)
        GET_ICASE(digitizer,GroupDCOffset,index,ui2s)
        GET_ICASE(digitizer,ChannelSelfTrigger,index,ui2s)
        GET_ICASE(digitizer,GroupSelfTrigger,index,ui2s)
        GET_ICASE(digitizer,ChannelTriggerThreshold,index,ui2s)
        GET_ICASE(digitizer,GroupTriggerThreshold,index,ui2s)
        GET_ICASE(digitizer,ChannelGroupMask,index,ui2s)
        GET_ICASE(digitizer,TriggerPolarity,index,ui2s)
        GET_ICASE(digitizer,DPPPreTriggerSize,index,ui2s)
        GET_ICASE(digitizer,ChannelPulsePolarity,index,ui2s)
        GET_ICASE(digitizer,RecordLength,index,ui2s)
        GET_ICASE(digitizer,NumEventsPerAggregate,index,ui2s)
        default:
            throw std::invalid_argument{"Unknown Function"};
    }
}