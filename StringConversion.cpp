/**
 * jadaq (Just Another DAQ)
 * Copyright (C) 2017  Troels Blum <troels@blum.dk>
 * Contributions from  Jonas Bardino <bardino@nbi.ku.dk>
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
 * Convert between strings and various digitizer helper types.
 *
 */

#include "StringConversion.hpp"

#include <regex>

#define STR_MATCH(S,V,R) if (std::regex_search(S, std::regex(#V, std::regex::icase))) return R

int s2i(const std::string& s)
{
    return std::stoi(s,nullptr);
}

unsigned int s2ui(const std::string& s)
{
    return std::stoi(s,nullptr,0);
}
uint16_t s2ui16(const std::string& s)
{
    return (uint16_t)s2ui(s);
}
uint8_t s2ui8(const std::string& s)
{
    return (uint8_t)s2ui(s);
}

// binary string
unsigned int bs2ui(const std::string& s)
{
    return std::stoi(s,nullptr,2);
}

CAEN_DGTZ_IOLevel_t s2iol(const std::string& s)
{
    return (CAEN_DGTZ_IOLevel_t)std::stoi(s,nullptr,0);
}
CAEN_DGTZ_AcqMode_t s2am(const std::string& s)
{
    return (CAEN_DGTZ_AcqMode_t)std::stoi(s,nullptr,0);
}
CAEN_DGTZ_TriggerMode_t s2tm(const std::string& s)
{
    return (CAEN_DGTZ_TriggerMode_t)std::stoi(s,nullptr,0);
}
CAEN_DGTZ_DPP_TriggerMode_t s2dtm(const std::string& s)
{
    return (CAEN_DGTZ_DPP_TriggerMode_t)std::stoi(s,nullptr,0);
}
CAEN_DGTZ_DRS4Frequency_t s2drsff(const std::string& s)
{
    return (CAEN_DGTZ_DRS4Frequency_t)std::stoi(s,nullptr,0);
}
CAEN_DGTZ_RunSyncMode_t s2rsm(const std::string& s)
{
    return (CAEN_DGTZ_RunSyncMode_t)std::stoi(s,nullptr,0);
}
CAEN_DGTZ_OutputSignalMode_t s2osm(const std::string& s)
{
    return (CAEN_DGTZ_OutputSignalMode_t)std::stoi(s,nullptr,0);
}
CAEN_DGTZ_EnaDis_t s2ed(const std::string& s)
{
    return (CAEN_DGTZ_EnaDis_t)std::stoi(s,nullptr,0);
}
CAEN_DGTZ_ZS_Mode_t s2zsm(const std::string& s)
{
    return (CAEN_DGTZ_ZS_Mode_t)std::stoi(s,nullptr,0);
}
CAEN_DGTZ_AnalogMonitorOutputMode_t s2amom(const std::string& s)
{
    return (CAEN_DGTZ_AnalogMonitorOutputMode_t)std::stoi(s,nullptr,0);
}
CAEN_DGTZ_TriggerPolarity_t s2tp(const std::string& s)
{
    return (CAEN_DGTZ_TriggerPolarity_t)std::stoi(s,nullptr,0);
}
CAEN_DGTZ_PulsePolarity_t s2pp(const std::string& s)
{
    return (CAEN_DGTZ_PulsePolarity_t)std::stoi(s,nullptr,0);
}

CAEN_DGTZ_ThresholdWeight_t s2tw(const std::string& s)
{
    STR_MATCH(s,Fine,CAEN_DGTZ_ZS_FINE);
    STR_MATCH(s,Coarse,CAEN_DGTZ_ZS_COARSE);
    return (CAEN_DGTZ_ThresholdWeight_t)std::stoi(s,nullptr,0);
}
std::string to_string(CAEN_DGTZ_ThresholdWeight_t tw)
{
    switch (tw)
    {
        case CAEN_DGTZ_ZS_FINE :
            return("Fine");
        case CAEN_DGTZ_ZS_COARSE :
            return ("Coarse");
        default :
            return std::to_string(tw);
    }
}

std::string to_string(const caen::ZSParams &zsp)
{
    std::stringstream ss;
    ss << '{' << to_string(zsp.weight) << ',' << to_string(zsp.threshold) << ',' << to_string(zsp.nsamp) << '}';
    return ss.str();
}

caen::ZSParams s2zsp(const std::string& s)
{
    std::regex rx("\\{(\\w+),(\\w+),(\\w+)\\}");
    std::smatch match;
    if (std::regex_search(s, match, rx))
    {
        return caen::ZSParams{s2tw(match[1]),s2i(match[2]),s2i(match[3])};
    }
    throw std::invalid_argument{"Invalid ZSParams"};
}

std::string to_string(CAEN_DGTZ_AnalogMonitorMagnify_t mf)
{
    switch (mf)
    {
        case CAEN_DGTZ_AM_MAGNIFY_1X:
            return("1X");
        case CAEN_DGTZ_AM_MAGNIFY_2X:
            return("2X");
        case CAEN_DGTZ_AM_MAGNIFY_4X:
            return("4X");
        case CAEN_DGTZ_AM_MAGNIFY_8X:
            return("8X");
        default :
            return std::to_string(mf);
    }
}
CAEN_DGTZ_AnalogMonitorMagnify_t s2mf(const std::string& s)
{
    STR_MATCH(s,1X,CAEN_DGTZ_AM_MAGNIFY_1X);
    STR_MATCH(s,2X,CAEN_DGTZ_AM_MAGNIFY_2X);
    STR_MATCH(s,4X,CAEN_DGTZ_AM_MAGNIFY_4X);
    STR_MATCH(s,8X,CAEN_DGTZ_AM_MAGNIFY_8X);
    return (CAEN_DGTZ_AnalogMonitorMagnify_t)std::stoi(s,nullptr,0);
}

std::string to_string(CAEN_DGTZ_AnalogMonitorInspectorInverter_t ami)
{
    switch (ami)
    {
        case CAEN_DGTZ_AM_INSPECTORINVERTER_P_1X:
            return("P_1X");
        case CAEN_DGTZ_AM_INSPECTORINVERTER_N_1X:
            return ("N_1X");
        default :
            return std::to_string(ami);
    }
}
CAEN_DGTZ_AnalogMonitorInspectorInverter_t s2ami(const std::string& s)
{
    STR_MATCH(s,P_1X,CAEN_DGTZ_AM_INSPECTORINVERTER_P_1X);
    STR_MATCH(s,N_1X,CAEN_DGTZ_AM_INSPECTORINVERTER_N_1X);
    return (CAEN_DGTZ_AnalogMonitorInspectorInverter_t)std::stoi(s,nullptr,0);
}

std::string to_string(const caen::AIMParams &aimp)
{
    std::stringstream ss;
    ss << '{' << to_string(aimp.channelmask) << ',' << to_string(aimp.offset) << ',' << to_string(aimp.mf) << ',' << to_string(aimp.ami) << '}';
    return ss.str();
}
caen::AIMParams s2aimp(const std::string& s)
{
    std::regex rx("\\{(\\w+),(\\w+),(\\w+),(\\w+)\\}");
    std::smatch match;
    if (std::regex_search(s, match, rx))
    {
        return caen::AIMParams{s2ui(match[1]),s2ui(match[2]),s2mf(match[3]),s2ami(match[4])};
    }
    throw std::invalid_argument{"Invalid AIMParams"};
}

CAEN_DGTZ_TrigerLogic_t s2tl(const std::string& s)
{
    STR_MATCH(s,OR,CAEN_DGTZ_LOGIC_OR);
    STR_MATCH(s,AND,CAEN_DGTZ_LOGIC_AND);
    return (CAEN_DGTZ_TrigerLogic_t)std::stoi(s,nullptr,0);
}
std::string to_string(CAEN_DGTZ_TrigerLogic_t tl)
{
    switch (tl)
    {
        case CAEN_DGTZ_LOGIC_OR:
            return("OR");
        case CAEN_DGTZ_LOGIC_AND:
            return("AND");
        default :
            return std::to_string(tl);
    }
}

std::string to_string(const caen::ChannelPairTriggerLogicParams &cptlp)
{
    std::stringstream ss;
    ss << '{' << to_string(cptlp.logic) << ',' << ui_to_string(cptlp.coincidenceWindow) << '}';
    return ss.str();
}

caen::ChannelPairTriggerLogicParams s2cptlp(const std::string& s)
{
    std::regex rx("\\{(\\w+),(\\w+)\\}");
    std::smatch match;
    if (std::regex_search(s, match, rx))
    {
        return caen::ChannelPairTriggerLogicParams{s2tl(match[1]),s2ui16(match[2])};
    }
    throw std::invalid_argument{"Invalid ChannelPairTriggerLogicParams"};
}

std::string to_string(const caen::TriggerLogicParams &tlp)
{
    std::stringstream ss;
    ss << '{' << to_string(tlp.logic) << ',' << to_string(tlp.majorityLevel) << '}';
    return ss.str();
}

caen::TriggerLogicParams s2tlp(const std::string& s)
{
    std::regex rx("\\{(\\w+),(\\w+)\\}");
    std::smatch match;
    if (std::regex_search(s, match, rx))
    {
        return caen::TriggerLogicParams{s2tl(match[1]),s2ui(match[2])};
    }
    throw std::invalid_argument{"Invalid TriggerLogicParams"};
}

std::string to_string(const caen::SAMTriggerCountVetoParams &samtcvp)
{
    std::stringstream ss;
    ss << '{' << to_string(samtcvp.enable) << ',' << to_string(samtcvp.vetoWindow) << '}';
    return ss.str();
}

caen::SAMTriggerCountVetoParams s2samtcvp(const std::string& s)
{
    std::regex rx("\\{(\\w+),(\\w+)\\}");
    std::smatch match;
    if (std::regex_search(s, match, rx))
    {
        return caen::SAMTriggerCountVetoParams{s2ed(match[1]),s2ui(match[2])};
    }
    throw std::invalid_argument{"Invalid SAMTriggerCountVetoParams"};
}

/* EasyXHelpers */
std::string to_string(const caen::EasyDPPAlgorithmControlHelper &edppach)
{
    return edppach.toConfString();
}

caen::EasyDPPAlgorithmControlHelper s2edppach(const std::string& s)
{
    std::regex rx("\\{(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+)\\}");
    std::smatch match;
    if (std::regex_search(s, match, rx))
    {
        return caen::EasyDPPAlgorithmControlHelper(s2ui8(match[1]),s2ui8(match[2]),s2ui8(match[3]),s2ui8(match[4]),s2ui8(match[5]),s2ui8(match[6]),s2ui8(match[7]),s2ui8(match[8]),s2ui8(match[9]),s2ui8(match[10]));
    }
    throw std::invalid_argument{"Invalid EasyDPPAlgorithmControlHelper"};
}

std::string to_string(const caen::EasyBoardConfigurationHelper &ebch)
{
    return ebch.toConfString();
}

caen::EasyBoardConfigurationHelper s2ebch(const std::string& s)
{
    std::regex rx("\\{(\\w+),(\\w+),(\\w+)\\}");
    std::smatch match;
    if (std::regex_search(s, match, rx))
    {
        return caen::EasyBoardConfigurationHelper(s2ui8(match[1]),s2ui8(match[2]),s2ui8(match[3]));
    }
    throw std::invalid_argument{"Invalid EasyBoardConfigurationHelper"};
}

std::string to_string(const caen::EasyDPPBoardConfigurationHelper &edbch)
{
    return edbch.toConfString();
}

caen::EasyDPPBoardConfigurationHelper s2edbch(const std::string& s)
{
    std::regex rx("\\{(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+)\\}");
    std::smatch match;
    if (std::regex_search(s, match, rx))
    {
        return caen::EasyDPPBoardConfigurationHelper(s2ui8(match[1]),s2ui8(match[2]),s2ui8(match[3]),s2ui8(match[4]),s2ui8(match[5]),s2ui8(match[6]),s2ui8(match[7]));
    }
    throw std::invalid_argument{"Invalid EasyDPPBoardConfigurationHelper"};
}

std::string to_string(const caen::EasyAcquisitionControlHelper &each)
{
    return each.toConfString();
}

caen::EasyAcquisitionControlHelper s2each(const std::string& s)
{
    std::regex rx("\\{(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+)\\}");
    std::smatch match;
    if (std::regex_search(s, match, rx))
    {
        return caen::EasyAcquisitionControlHelper(s2ui8(match[1]),s2ui8(match[2]),s2ui8(match[3]),s2ui8(match[4]),s2ui8(match[5]),s2ui8(match[6]),s2ui8(match[7]),s2ui8(match[8]));
    }
    throw std::invalid_argument{"Invalid EasyAcquisitionControlHelper"};
}

std::string to_string(const caen::EasyDPPAcquisitionControlHelper &edach)
{
    return edach.toConfString();
}

caen::EasyDPPAcquisitionControlHelper s2edach(const std::string& s)
{
    std::regex rx("\\{(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+)\\}");
    std::smatch match;
    if (std::regex_search(s, match, rx))
    {
        return caen::EasyDPPAcquisitionControlHelper(s2ui8(match[1]),s2ui8(match[2]),s2ui8(match[3]),s2ui8(match[4]),s2ui8(match[5]),s2ui8(match[6]),s2ui8(match[7]));
    }
    throw std::invalid_argument{"Invalid EasyDPPAcquisitionControlHelper"};
}

std::string to_string(const caen::EasyAcquisitionStatusHelper &eash)
{
    return eash.toConfString();
}

caen::EasyAcquisitionStatusHelper s2eash(const std::string& s)
{
    std::regex rx("\\{(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+)\\}");
    std::smatch match;
    if (std::regex_search(s, match, rx))
    {
        return caen::EasyAcquisitionStatusHelper(s2ui8(match[1]),s2ui8(match[2]),s2ui8(match[3]),s2ui8(match[4]),s2ui8(match[5]),s2ui8(match[6]),s2ui8(match[7]),s2ui8(match[8]),s2ui8(match[9]));
    }
    throw std::invalid_argument{"Invalid EasyAcquisitionStatusHelper"};
}

std::string to_string(const caen::EasyDPPAcquisitionStatusHelper &edash)
{
    return edash.toConfString();
}

caen::EasyDPPAcquisitionStatusHelper s2edash(const std::string& s)
{
    std::regex rx("\\{(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+)\\}");
    std::smatch match;
    if (std::regex_search(s, match, rx))
    {
        return caen::EasyDPPAcquisitionStatusHelper(s2ui8(match[1]),s2ui8(match[2]),s2ui8(match[3]),s2ui8(match[4]),s2ui8(match[5]),s2ui8(match[6]),s2ui8(match[7]),s2ui8(match[8]));
    }
    throw std::invalid_argument{"Invalid EasyDPPAcquisitionStatusHelper"};
}

std::string to_string(const caen::EasyGlobalTriggerMaskHelper &egtmh)
{
    return egtmh.toConfString();
}

caen::EasyGlobalTriggerMaskHelper s2egtmh(const std::string& s)
{
    std::regex rx("\\{(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+)\\}");
    std::smatch match;
    if (std::regex_search(s, match, rx))
    {
        return caen::EasyGlobalTriggerMaskHelper(s2ui8(match[1]),s2ui8(match[2]),s2ui8(match[3]),s2ui8(match[4]),s2ui8(match[5]),s2ui8(match[6]));
    }
    throw std::invalid_argument{"Invalid EasyGlobalTriggerMaskHelper"};
}

std::string to_string(const caen::EasyDPPGlobalTriggerMaskHelper &edgtmh)
{
    return edgtmh.toConfString();
}

caen::EasyDPPGlobalTriggerMaskHelper s2edgtmh(const std::string& s)
{
    std::regex rx("\\{(\\w+),(\\w+),(\\w+)\\}");
    std::smatch match;
    if (std::regex_search(s, match, rx))
    {
        return caen::EasyDPPGlobalTriggerMaskHelper(s2ui8(match[1]),s2ui8(match[2]),s2ui8(match[3]));
    }
    throw std::invalid_argument{"Invalid EasyDPPGlobalTriggerMaskHelper"};
}

std::string to_string(const caen::EasyFrontPanelTRGOUTEnableMaskHelper &efptoemh)
{
    return efptoemh.toConfString();
}

caen::EasyFrontPanelTRGOUTEnableMaskHelper s2efptoemh(const std::string& s)
{
    std::regex rx("\\{(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+)\\}");
    std::smatch match;
    if (std::regex_search(s, match, rx))
    {
        return caen::EasyFrontPanelTRGOUTEnableMaskHelper(s2ui8(match[1]),s2ui8(match[2]),s2ui8(match[3]),s2ui8(match[4]),s2ui8(match[5]),s2ui8(match[6]));
    }
    throw std::invalid_argument{"Invalid EasyFrontPanelTRGOUTEnableMaskHelper"};
}

std::string to_string(const caen::EasyDPPFrontPanelTRGOUTEnableMaskHelper &edfptoemh)
{
    return edfptoemh.toConfString();
}

caen::EasyDPPFrontPanelTRGOUTEnableMaskHelper s2edfptoemh(const std::string& s)
{
    std::regex rx("\\{(\\w+),(\\w+),(\\w+)\\}");
    std::smatch match;
    if (std::regex_search(s, match, rx))
    {
        return caen::EasyDPPFrontPanelTRGOUTEnableMaskHelper(s2ui8(match[1]),s2ui8(match[2]),s2ui8(match[3]));
    }
    throw std::invalid_argument{"Invalid EasyDPPFrontPanelTRGOUTEnableMaskHelper"};
}

std::string to_string(const caen::EasyFrontPanelIOControlHelper &efpioch)
{
    return efpioch.toConfString();
}

caen::EasyFrontPanelIOControlHelper s2efpioch(const std::string& s)
{
    std::regex rx("\\{(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+)\\}");
    std::smatch match;
    if (std::regex_search(s, match, rx))
    {
        return caen::EasyFrontPanelIOControlHelper(s2ui8(match[1]),s2ui8(match[2]),s2ui8(match[3]),s2ui8(match[4]),s2ui8(match[5]),s2ui8(match[6]),s2ui8(match[7]),s2ui8(match[8]),s2ui8(match[9]),s2ui8(match[10]),s2ui8(match[11]),s2ui8(match[12]),s2ui8(match[13]),s2ui8(match[14]),s2ui8(match[15]),s2ui8(match[16]),s2ui8(match[17]));
    }
    throw std::invalid_argument{"Invalid EasyFrontPanelIOControlHelper"};
}

std::string to_string(const caen::EasyDPPFrontPanelIOControlHelper &edfpioch)
{
    return edfpioch.toConfString();
}

caen::EasyDPPFrontPanelIOControlHelper s2edfpioch(const std::string& s)
{
    std::regex rx("\\{(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+)\\}");
    std::smatch match;
    if (std::regex_search(s, match, rx))
    {
        return caen::EasyDPPFrontPanelIOControlHelper(s2ui8(match[1]),s2ui8(match[2]),s2ui8(match[3]),s2ui8(match[4]),s2ui8(match[5]),s2ui8(match[6]),s2ui8(match[7]),s2ui8(match[8]),s2ui8(match[9]),s2ui8(match[10]),s2ui8(match[11]),s2ui8(match[12]),s2ui8(match[13]),s2ui8(match[14]),s2ui8(match[15]),s2ui8(match[16]),s2ui8(match[17]));
    }
    throw std::invalid_argument{"Invalid EasyDPPFrontPanelIOControlHelper"};
}

std::string to_string(const caen::EasyROCFPGAFirmwareRevisionHelper &erffrh)
{
    /* NOTE: we use a simpler format for revisionDate internally so we
     mangle it to the original clamped format here */
    std::stringstream ss;
    std::string raw = erffrh.toConfString();
    std::regex rx("\\{(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+)\\} # \\{(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+)\\}");
    std::smatch match;
    if (std::regex_search(raw, match, rx))
    {
        ss << '{' << match[1] << "," << match[2] << "," << match[6] << match[5] << match[4] << match[3] << "} # {" << match[7] << "," << match[8] << "," << "revisionDate}";
        return ss.str();
    }
    std::cerr << "EasyROCFPGAFirmwareRevisionHelper failed conversion" << std::endl;
    throw std::invalid_argument{"Invalid EasyROCFPGAFirmwareRevisionHelper"};
}

caen::EasyROCFPGAFirmwareRevisionHelper s2erffrh(const std::string& s)
{
    std::regex rx("\\{(\\w+),(\\w+),(\\w+)\\}");
    std::smatch match;
    if (std::regex_search(s, match, rx))
    {
        /* NOTE: original date is encoded as four individual 4-bit
         * integer values concatenated. Thus, we unpack as int and force
         * to uint16 to fit the corresponding constructor. */
        return caen::EasyROCFPGAFirmwareRevisionHelper(s2ui8(match[1]),s2ui8(match[2]),s2ui16(match[3]));
    }
    throw std::invalid_argument{"Invalid EasyROCFPGAFirmwareRevisionHelper"};
}

std::string to_string(const caen::EasyDPPROCFPGAFirmwareRevisionHelper &edrffrh)
{
    /* NOTE: we use a simpler format for revisionDate internally so we
     mangle it to the original clamped format here */
    std::stringstream ss;
    std::string raw = edrffrh.toConfString();
    std::regex rx("\\{(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+)\\} # \\{(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+)\\}");
    std::smatch match;
    if (std::regex_search(raw, match, rx))
    {
        ss << '{' << match[1] << "," << match[2] << "," << match[6] << match[5] << match[4] << match[3] << "} # {" << match[7] << "," << match[8] << "," << "revisionDate}";
        return ss.str();
    }
    std::cerr << "EasyDPPROCFPGAFirmwareRevisionHelper failed conversion" << std::endl;
    throw std::invalid_argument{"Invalid EasyDPPROCFPGAFirmwareRevisionHelper"};
}

caen::EasyDPPROCFPGAFirmwareRevisionHelper s2edrffrh(const std::string& s)
{
    std::regex rx("\\{(\\w+),(\\w+),(\\w+)\\}");
    std::smatch match;
    if (std::regex_search(s, match, rx))
    {
        /* NOTE: original date is encoded as four individual 4-bit
         * integer values concatenated. Thus, we unpack as int and force
         * to uint16 to fit the corresponding constructor. */
        return caen::EasyDPPROCFPGAFirmwareRevisionHelper(s2ui8(match[1]),s2ui8(match[2]),s2ui16(match[3]));
    }
    throw std::invalid_argument{"Invalid EasyDPPROCFPGAFirmwareRevisionHelper"};
}

std::string to_string(const caen::EasyFanSpeedControlHelper &efsch)
{
    return efsch.toConfString();
}

caen::EasyFanSpeedControlHelper s2efsch(const std::string& s)
{
    std::regex rx("\\{(\\w+)\\}");
    std::smatch match;
    if (std::regex_search(s, match, rx))
    {
        return caen::EasyFanSpeedControlHelper(s2ui8(match[1]));
    }
    throw std::invalid_argument{"Invalid EasyFanSpeedControlHelper"};
}

std::string to_string(const caen::EasyDPPFanSpeedControlHelper &edfsch)
{
    return edfsch.toConfString();
}

caen::EasyDPPFanSpeedControlHelper s2edfsch(const std::string& s)
{
    std::regex rx("\\{(\\w+)\\}");
    std::smatch match;
    if (std::regex_search(s, match, rx))
    {
        return caen::EasyDPPFanSpeedControlHelper(s2ui8(match[1]));
    }
    throw std::invalid_argument{"Invalid EasyDPPFanSpeedControlHelper"};
}

std::string to_string(const caen::EasyReadoutControlHelper &erch)
{
    return erch.toConfString();
}

caen::EasyReadoutControlHelper s2erch(const std::string& s)
{
    std::regex rx("\\{(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+)\\}");
    std::smatch match;
    if (std::regex_search(s, match, rx))
    {
        return caen::EasyReadoutControlHelper(s2ui8(match[1]),s2ui8(match[2]),s2ui8(match[3]),s2ui8(match[4]),s2ui8(match[5]),s2ui8(match[6]),s2ui8(match[7]));
    }
    throw std::invalid_argument{"Invalid EasyReadoutControlHelper"};
}

std::string to_string(const caen::EasyDPPReadoutControlHelper &edrch)
{
    return edrch.toConfString();
}

caen::EasyDPPReadoutControlHelper s2edrch(const std::string& s)
{
    std::regex rx("\\{(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+)\\}");
    std::smatch match;
    if (std::regex_search(s, match, rx))
    {
        return caen::EasyDPPReadoutControlHelper(s2ui8(match[1]),s2ui8(match[2]),s2ui8(match[3]),s2ui8(match[4]),s2ui8(match[5]),s2ui8(match[6]),s2ui8(match[7]));
    }
    throw std::invalid_argument{"Invalid EasyDPPReadoutControlHelper"};
}

std::string to_string(const caen::EasyReadoutStatusHelper &ersh)
{
    return ersh.toConfString();
}

caen::EasyReadoutStatusHelper s2ersh(const std::string& s)
{
    std::regex rx("\\{(\\w+),(\\w+),(\\w+)\\}");
    std::smatch match;
    if (std::regex_search(s, match, rx))
    {
        return caen::EasyReadoutStatusHelper(s2ui8(match[1]),s2ui8(match[2]),s2ui8(match[3]));
    }
    throw std::invalid_argument{"Invalid EasyReadoutStatusHelper"};
}

std::string to_string(const caen::EasyDPPReadoutStatusHelper &edrsh)
{
    return edrsh.toConfString();
}

caen::EasyDPPReadoutStatusHelper s2edrsh(const std::string& s)
{
    std::regex rx("\\{(\\w+),(\\w+),(\\w+)\\}");
    std::smatch match;
    if (std::regex_search(s, match, rx))
    {
        return caen::EasyDPPReadoutStatusHelper(s2ui8(match[1]),s2ui8(match[2]),s2ui8(match[3]));
    }
    throw std::invalid_argument{"Invalid EasyDPPReadoutStatusHelper"};
}

std::string to_string(const caen::EasyAMCFirmwareRevisionHelper &eafrh)
{
    /* NOTE: we use a simpler format for revisionDate internally so we
     mangle it to the original clamped format here */
    std::stringstream ss;
    std::string raw = eafrh.toConfString();
    std::regex rx("\\{(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+)\\} # \\{(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+)\\}");
    std::smatch match;
    if (std::regex_search(raw, match, rx))
    {
        ss << '{' << match[1] << "," << match[2] << "," << match[6] << match[5] << match[4] << match[3] << "} # {" << match[7] << "," << match[8] << "," << "revisionDate}";
        return ss.str();
    }
    throw std::invalid_argument{"Invalid EasyAMCFirmwareRevisionHelper"};
}

caen::EasyAMCFirmwareRevisionHelper s2eafrh(const std::string& s)
{
    std::regex rx("\\{(\\w+),(\\w+),(\\w+)\\}");
    std::smatch match;
    if (std::regex_search(s, match, rx))
    {
        /* NOTE: original date is encoded as four individual 4-bit
         * integer values concatenated. Thus, we unpack as int and force
         * to uint16 to fit the corresponding constructor. */
        return caen::EasyAMCFirmwareRevisionHelper(s2ui8(match[1]),s2ui8(match[2]),s2ui16(match[3]));
    }
    throw std::invalid_argument{"Invalid EasyAMCFirmwareRevisionHelper"};
}

std::string to_string(const caen::EasyDPPAMCFirmwareRevisionHelper &edafrh)
{
    return edafrh.toConfString();
}

caen::EasyDPPAMCFirmwareRevisionHelper s2edafrh(const std::string& s)
{
    std::regex rx("\\{(\\w+),(\\w+),(\\w+),(\\w+),(\\w+),(\\w+)\\}");
    std::smatch match;
    if (std::regex_search(s, match, rx))
    {
        return caen::EasyDPPAMCFirmwareRevisionHelper(s2ui8(match[1]),s2ui8(match[2]),s2ui8(match[3]),s2ui8(match[4]),s2ui8(match[5]),s2ui8(match[6]));
    }
    throw std::invalid_argument{"Invalid EasyDPPAMCFirmwareRevisionHelper"};
}

std::string to_string(CAEN_DGTZ_DPP_AcqMode_t mode)
{
    switch (mode)
    {
        case CAEN_DGTZ_DPP_ACQ_MODE_Oscilloscope :
            return("Oscilloscope");
        case CAEN_DGTZ_DPP_ACQ_MODE_List :
            return ("List");
        case CAEN_DGTZ_DPP_ACQ_MODE_Mixed :
            return ("Mixed");
        default :
            return std::to_string(mode);
    }
}
CAEN_DGTZ_DPP_AcqMode_t s2dam(const std::string& s)
{
    STR_MATCH(s,Oscilloscope,CAEN_DGTZ_DPP_ACQ_MODE_Oscilloscope);
    STR_MATCH(s,List,CAEN_DGTZ_DPP_ACQ_MODE_List);
    STR_MATCH(s,Mixed,CAEN_DGTZ_DPP_ACQ_MODE_Mixed);
    return (CAEN_DGTZ_DPP_AcqMode_t)std::stoi(s,nullptr,0);
}

std::string to_string(CAEN_DGTZ_DPP_SaveParam_t sp)
{
    switch (sp)
    {
        case CAEN_DGTZ_DPP_SAVE_PARAM_EnergyOnly :
            return("EnergyOnly");
        case CAEN_DGTZ_DPP_SAVE_PARAM_TimeOnly :
            return ("TimeOnly");
        case CAEN_DGTZ_DPP_SAVE_PARAM_EnergyAndTime :
            return ("EnergyAndTime");
        case CAEN_DGTZ_DPP_SAVE_PARAM_None :
            return ("None");
        default :
            return std::to_string(sp);
    }
}
CAEN_DGTZ_DPP_SaveParam_t s2sp(const std::string& s)
{
    STR_MATCH(s,EnergyOnly,CAEN_DGTZ_DPP_SAVE_PARAM_EnergyOnly);
    STR_MATCH(s,TimeOnly,CAEN_DGTZ_DPP_SAVE_PARAM_TimeOnly);
    STR_MATCH(s,EnergyAndTime,CAEN_DGTZ_DPP_SAVE_PARAM_EnergyAndTime);
    STR_MATCH(s,None,CAEN_DGTZ_DPP_SAVE_PARAM_None);
    return (CAEN_DGTZ_DPP_SaveParam_t)std::stoi(s,nullptr,0);
}

std::string to_string(const caen::DPPAcquisitionMode &dam)
{
    std::stringstream ss;
    ss << '{' << to_string(dam.mode) << ',' << to_string(dam.param) << '}';
    return ss.str();
}

caen::DPPAcquisitionMode s2cdam(const std::string& s)
{
    std::regex rx("\\{(\\w+),(\\w+)\\}");
    std::smatch match;
    if (std::regex_search(s, match, rx))
    {
        return caen::DPPAcquisitionMode{s2dam(match[1]),s2sp(match[2])};
    }
    throw std::invalid_argument{"Invalid DPPAcquisitionMode"};
}

std::string to_string(CAEN_DGTZ_SAM_CORRECTION_LEVEL_t samcl)
{
    switch (samcl)
    {
        case CAEN_DGTZ_SAM_CORRECTION_DISABLED :
            return("DISABLED");
        case CAEN_DGTZ_SAM_CORRECTION_PEDESTAL_ONLY :
            return("PEDESTAL_ONLY");
        case CAEN_DGTZ_SAM_CORRECTION_INL :
            return("INL");
        case CAEN_DGTZ_SAM_CORRECTION_ALL :
            return("ALL");
        default :
            return std::to_string(samcl);
    }
}
CAEN_DGTZ_SAM_CORRECTION_LEVEL_t s2samcl(const std::string& s)
{
    STR_MATCH(s,DISABLED,CAEN_DGTZ_SAM_CORRECTION_DISABLED);
    STR_MATCH(s,PEDESTAL_ONLY,CAEN_DGTZ_SAM_CORRECTION_PEDESTAL_ONLY);
    STR_MATCH(s,INL,CAEN_DGTZ_SAM_CORRECTION_INL);
    STR_MATCH(s,ALL,CAEN_DGTZ_SAM_CORRECTION_ALL);
    return (CAEN_DGTZ_SAM_CORRECTION_LEVEL_t)std::stoi(s,nullptr,0);
}

std::string to_string(CAEN_DGTZ_SAMFrequency_t samf)
{
    switch (samf)
    {
        case CAEN_DGTZ_SAM_3_2GHz :
            return("3_2GHz");
        case CAEN_DGTZ_SAM_1_6GHz :
            return("1_6GHz");
        case CAEN_DGTZ_SAM_800MHz :
            return("800MHz");
        case CAEN_DGTZ_SAM_400MHz :
            return("400MHz");
        default :
            return std::to_string(samf);
    }
}
CAEN_DGTZ_SAMFrequency_t s2samf(const std::string& s)
{
    STR_MATCH(s,3_2GHz,CAEN_DGTZ_SAM_3_2GHz);
    STR_MATCH(s,1_6GHz,CAEN_DGTZ_SAM_1_6GHz);
    STR_MATCH(s,800MHz,CAEN_DGTZ_SAM_800MHz);
    STR_MATCH(s,400MHz,CAEN_DGTZ_SAM_400MHz);
    return (CAEN_DGTZ_SAMFrequency_t)std::stoi(s,nullptr,0);
}

std::string to_string(CAEN_DGTZ_AcquisitionMode_t mode)
{
    switch (mode)
    {
        case CAEN_DGTZ_AcquisitionMode_STANDARD:
            return("STANDARD");
        case CAEN_DGTZ_AcquisitionMode_DPP_CI:
            return("DPP_CI");
        default :
            return std::to_string(mode);
    }
}
CAEN_DGTZ_AcquisitionMode_t s2samam(const std::string& s)
{
    STR_MATCH(s,STANDARD,CAEN_DGTZ_AcquisitionMode_STANDARD);
    STR_MATCH(s,DPP_CI,CAEN_DGTZ_AcquisitionMode_DPP_CI);
    return (CAEN_DGTZ_AcquisitionMode_t)std::stoi(s,nullptr,0);
}

