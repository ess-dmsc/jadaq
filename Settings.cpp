//
// Created by troels on 9/18/17.
//


#include "Settings.hpp"
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

static unsigned int s2ui(const std::string& s)
{ return std::stoi (s,nullptr,0); }

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
    if (std::regex_search(s.begin(), s.end(), match, rx))
    {
        return caen::DPPAcquisitionMode{s2dam_(match[1]),s2sp(match[2])};
    }
    throw std::invalid_argument{"Invalid DPPAcquisitionMode"};
}

#define _CASE(GS,D,F,V) GS##_CASE(D,F,V)

#define SET_CASE(D,F,V) \
    case F :            \
        D->set##F(V);   \
        break;

#define GET_CASE(D,F,SF) \
    case F :            \
        return SF(D->get##F());   \


#define SET_ICASE(D,F,C,V)   \
    case F :                 \
        D->set##F(C,V); \
        break;

void set(caen::Digitizer* digitizer, FunctionID functionID, std::string value)
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

void set(caen::Digitizer* digitizer, FunctionID functionID, int index, std::string value) {
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
