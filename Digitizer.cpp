//
// Created by troels on 9/19/17.
//

#include "Digitizer.hpp"
#include <regex>

using std::to_string;

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
    if (std::regex_search(s, match, rx))
    {
        return caen::DPPAcquisitionMode{s2dam_(match[1]),s2sp(match[2])};
    }
    throw std::invalid_argument{"Invalid DPPAcquisitionMode"};
}

static std::string to_string(const caen::DPPAcquisitionMode &dam) {
    std::stringstream ss;
    ss << "{" << dam.param << "," << dam.mode << "}";
    return ss.str();
}

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
        GET_CASE(digitizer,RecordLength)
        GET_CASE(digitizer,NumEventsPerAggregate)
        default:
            throw std::invalid_argument{"Unknown Function"};

    }
}

std::string Digitizer::get(FunctionID functionID, int index)
{
    switch(functionID)
    {
        GET_ICASE(digitizer,ChannelDCOffset,index)
        GET_ICASE(digitizer,GroupDCOffset,index)
        GET_ICASE(digitizer,ChannelSelfTrigger,index)
        GET_ICASE(digitizer,GroupSelfTrigger,index)
        GET_ICASE(digitizer,ChannelTriggerThreshold,index)
        GET_ICASE(digitizer,GroupTriggerThreshold,index)
        GET_ICASE(digitizer,ChannelGroupMask,index)
        GET_ICASE(digitizer,TriggerPolarity,index)
        GET_ICASE(digitizer,DPPPreTriggerSize,index)
        GET_ICASE(digitizer,ChannelPulsePolarity,index)
        GET_ICASE(digitizer,RecordLength,index)
        GET_ICASE(digitizer,NumEventsPerAggregate,index)
        default:
            throw std::invalid_argument{"Unknown Function"};
    }
}