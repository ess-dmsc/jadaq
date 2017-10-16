//
// Created by troels on 9/27/17.
//

#ifndef JADAQ_STRINGCONVERSION_HPP
#define JADAQ_STRINGCONVERSION_HPP

#include <string>
#include <sstream>
#include <bitset>
#include <climits>
#include "caen.hpp"

using std::to_string;

template<typename T>
std::string hex_string(T v)
{
    std::stringstream ss;
    ss << "0x" << std::hex << v;
    return ss.str();
}

template<typename T>
std::string oct_string(T v)
{
    std::stringstream ss;
    ss << '0' << std::oct << v;
    return ss.str();
}

template<size_t w=0, typename T>
std::string bin_string(T v)
{
    std::bitset<w?w:(sizeof(T)*CHAR_BIT)> bs(v);
    return bs.to_string();
}
// binary string
unsigned int bs2ui(const std::string& s);
int s2i(const std::string& s);
unsigned int s2ui(const std::string& s);
uint16_t s2ui16(const std::string& s);
uint8_t s2ui8(const std::string& s);
CAEN_DGTZ_IOLevel_t s2iol(const std::string& s);
CAEN_DGTZ_AcqMode_t s2am(const std::string& s);
CAEN_DGTZ_TriggerMode_t s2tm(const std::string& s);
CAEN_DGTZ_DPP_TriggerMode_t s2dtm(const std::string& s);
CAEN_DGTZ_DRS4Frequency_t s2drsff(const std::string& s);
CAEN_DGTZ_RunSyncMode_t s2rsm(const std::string& s);
CAEN_DGTZ_OutputSignalMode_t s2osm(const std::string& s);
CAEN_DGTZ_EnaDis_t s2ed(const std::string& s);
CAEN_DGTZ_ZS_Mode_t s2zsm(const std::string& s);
CAEN_DGTZ_AnalogMonitorOutputMode_t s2amom(const std::string& s);
CAEN_DGTZ_AnalogMonitorMagnify_t s2mf(const std::string& s);
CAEN_DGTZ_AnalogMonitorInspectorInverter_t s2ami(const std::string& s);
CAEN_DGTZ_TriggerPolarity_t s2tp(const std::string& s);
CAEN_DGTZ_PulsePolarity_t s2pp(const std::string& s);

std::string to_string(const caen::ZSParams &zsp);
caen::ZSParams s2zsp(const std::string& s);
std::string to_string(const caen::AIMParams &aimp);
caen::AIMParams s2aimp(const std::string& s);
std::string to_string(const caen::ChannelPairTriggerLogicParams &cptlp);
caen::ChannelPairTriggerLogicParams s2cptlp(const std::string& s);
std::string to_string(const caen::TriggerLogicParams &tlp);
caen::TriggerLogicParams s2tlp(const std::string& s);
std::string to_string(const caen::SAMTriggerCountVetoParams &samtcvp);
caen::SAMTriggerCountVetoParams s2samtcvp(const std::string& s);
std::string to_string(const caen::EasyDPPAlgorithmControl &edppac);
caen::EasyDPPAlgorithmControl s2edppac(const std::string& s);
std::string to_string(const caen::EasyBoardConfiguration &ebc);
caen::EasyBoardConfiguration s2ebc(const std::string& s);

std::string to_string(CAEN_DGTZ_DPP_AcqMode_t mode);
CAEN_DGTZ_DPP_AcqMode_t s2dam(const std::string& s);
std::string to_string(CAEN_DGTZ_DPP_SaveParam_t sp);
CAEN_DGTZ_DPP_SaveParam_t s2sp(const std::string& s);
std::string to_string(const caen::DPPAcquisitionMode &dam);
caen::DPPAcquisitionMode s2cdam(const std::string& s);
std::string to_string(CAEN_DGTZ_SAM_CORRECTION_LEVEL_t level);
CAEN_DGTZ_SAM_CORRECTION_LEVEL_t s2samcl(const std::string& s);
std::string to_string(CAEN_DGTZ_SAMFrequency_t frequency);
CAEN_DGTZ_SAMFrequency_t s2samf(const std::string& s);
std::string to_string(CAEN_DGTZ_AcquisitionMode_t mode);
CAEN_DGTZ_AcquisitionMode_t s2samam(const std::string& s);


#endif //JADAQ_STRINGCONVERSION_HPP
