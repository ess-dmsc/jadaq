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
 * Convenient wrapping of the official CAEN Digitizer library functions
 * and some additional functionality otherwise only exposed through low-level
 * register access.
 * This file contains most of the actual implementation.
 *
 */

#ifndef _CAEN_HPP
#define _CAEN_HPP

#include "_CAENDigitizer.h"
#include <CAENDigitizerType.h>
#include <string>
#include <sstream>
#include <cstdint>
#include <cassert>
#include <iostream>
#include <map>
#include <vector>
#include <tuple>
#include <boost/any.hpp>
#include <stdexcept>

/* TODO: add doxygen comments to all important functions and structs */

namespace caen {
    class Error : public std::exception
    {
    private:
        CAEN_DGTZ_ErrorCode code_;
    public:
#define CASE_TO_STR(X) case (X) : return (#X);
        static const char* digitizerErrorString(CAEN_DGTZ_ErrorCode code)
        {
            switch (code) {
                CASE_TO_STR(CAEN_DGTZ_Success)
                CASE_TO_STR(CAEN_DGTZ_CommError)
                CASE_TO_STR(CAEN_DGTZ_GenericError)
                CASE_TO_STR(CAEN_DGTZ_InvalidParam)
                CASE_TO_STR(CAEN_DGTZ_InvalidLinkType)
                CASE_TO_STR(CAEN_DGTZ_InvalidHandle)
                CASE_TO_STR(CAEN_DGTZ_MaxDevicesError)
                CASE_TO_STR(CAEN_DGTZ_BadBoardType)
                CASE_TO_STR(CAEN_DGTZ_BadInterruptLev)
                CASE_TO_STR(CAEN_DGTZ_BadEventNumber)
                CASE_TO_STR(CAEN_DGTZ_ReadDeviceRegisterFail)
                CASE_TO_STR(CAEN_DGTZ_WriteDeviceRegisterFail)
                CASE_TO_STR(CAEN_DGTZ_InvalidChannelNumber)
                CASE_TO_STR(CAEN_DGTZ_ChannelBusy)
                CASE_TO_STR(CAEN_DGTZ_FPIOModeInvalid)
                CASE_TO_STR(CAEN_DGTZ_WrongAcqMode)
                CASE_TO_STR(CAEN_DGTZ_FunctionNotAllowed)
                CASE_TO_STR(CAEN_DGTZ_Timeout)
                CASE_TO_STR(CAEN_DGTZ_InvalidBuffer)
                CASE_TO_STR(CAEN_DGTZ_EventNotFound)
                CASE_TO_STR(CAEN_DGTZ_InvalidEvent)
                CASE_TO_STR(CAEN_DGTZ_OutOfMemory)
                CASE_TO_STR(CAEN_DGTZ_CalibrationError)
                CASE_TO_STR(CAEN_DGTZ_DigitizerNotFound)
                CASE_TO_STR(CAEN_DGTZ_DigitizerAlreadyOpen)
                CASE_TO_STR(CAEN_DGTZ_DigitizerNotReady)
                CASE_TO_STR(CAEN_DGTZ_InterruptNotConfigured)
                CASE_TO_STR(CAEN_DGTZ_DigitizerMemoryCorrupted)
                CASE_TO_STR(CAEN_DGTZ_DPPFirmwareNotSupported)
                CASE_TO_STR(CAEN_DGTZ_InvalidLicense)
                CASE_TO_STR(CAEN_DGTZ_InvalidDigitizerStatus)
                CASE_TO_STR(CAEN_DGTZ_UnsupportedTrace)
                CASE_TO_STR(CAEN_DGTZ_InvalidProbe)
                CASE_TO_STR(CAEN_DGTZ_NotYetImplemented)
                default:
                    std::cerr << "Unknown CAEN error code: " << code << std::endl;
                   return "Unknown Error";
            }
        }
        Error(CAEN_DGTZ_ErrorCode code) : code_(code) {}
        virtual const char * what() const noexcept
        {
            return digitizerErrorString(code_);
        }

        CAEN_DGTZ_ErrorCode code() const { return code_; }
    };
    static inline void errorHandler(CAEN_DGTZ_ErrorCode code)
    {
        if (code != CAEN_DGTZ_Success) {
            throw Error(code);
        }
    }

    /**
     * @struct BasicEvent
     * @brief For a basic shared event readout - essentially merges main
     * parts of EventInfo and decoded Event data.
     * @var BasicEvent::boardId
     * ID of the board.
     * @var BasicEvent::channel
     * Which channel where the event came from.
     * @var BasicEvent::eventIndex
     * Index of the event for given channel.
     * @var BasicEvent::timestamp
     * Time stamp for the event.
     * @var BasicWaveform::count
     * Waveform sample values.
     * @var BasicWaveform::samples
     * Waveform sample values.
     */

    struct BasicEvent {
        uint32_t boardId;
        uint32_t channel;
        uint32_t eventIndex;
        uint32_t timestamp;
        /* NOTE: we use 32 bit counter and 16 bit array to accomodate
         * CAEN_DGTZ_UINT16_EVENT_t . Less is needed in case a
         * CAEN_DGTZ_UINT8_EVENT_t is wrapped up underneath. */
        uint32_t count;
        uint16_t *samples;
    };

    /* Alias BasicWaveform to BasicEvent as they are merged */
    typedef BasicEvent BasicWaveform;

    /**
     * @struct BasicDPPEvent
     * @brief For a very basic shared DPP event readout
     * @var BasicDPPEvent::timestamp
     * Time stamp for the event.
     * @var BasicDPPEvent::format
     * Internal format for the event - a bitmask packing multiple values.
     * @var BasicDPPEvent::charge
     * Measured integrated charge for the event.
     */
    struct BasicDPPEvent {
        /* NOTE: we use 64 bit timestamp here to accomodate PHA TimeTag */
        uint64_t timestamp;
        uint32_t format;
        /* Used to hold charge or energy in the PHA case. 
         * For PSD with 16 bitChareLong and ChargeShort we pack them.
        */
        uint32_t charge;
    };

    /**
     * @struct BasicDPPWaveforms
     * @brief For a very basic shared DPP waveform readout
     * @var BasicDPPWaveforms::Ns
     * Array entry counter.
     * @var BasicDPPWaveforms::Sample1
     * First waveform array - samples.
     * @var BasicDPPWaveforms::Sample2
     * Second waveform array - ?.
     * @var BasicDPPWaveforms::DSample1
     * First waveform array - gate.
     * @var BasicDPPWaveforms::DSample2
     * Second waveform array - trigger.
     * @var BasicDPPWaveforms::DSample3
     * Third waveform array - trigger hold off if available or NULL.
     * @var BasicDPPWaveforms::DSample4
     * Fourth waveform array - overthreshold if available or NULL.
     */
    struct BasicDPPWaveforms {
        uint32_t Ns;
        uint16_t *Sample1;
        uint16_t *Sample2;
        uint8_t *DSample1;
        uint8_t *DSample2;
        uint8_t *DSample3;
        uint8_t *DSample4;
    };

    /**
     * @struct ReadoutBuffer
     * @brief For parameter handling in ReadoutBuffer handling
     * @var ReadoutBuffer::data
     * The allocated memory buffer
     * @var ReadoutBuffer::size
     * The size (in byte) of the buffer allocated
     * @var ReadoutBuffer::dataSize
     * The size (in byte) of the buffer actually used
     */
    struct ReadoutBuffer {
        char* data;
        uint32_t size;
        uint32_t dataSize;
    };

    /**
     * @struct InterruptConfig
     * @brief For parameter handling in Set / GetInterruptConfig
     * @var InterruptConfig::state
     * Enable/Disable
     * @var InterruptConfig::level
     * VME IRQ Level (from 1 to 7). Must be 1 for direct connection
     * through CONET
     * @var InterruptConfig::status_id
     * 32-bit number assigned to the device and returned by the device
     * during the Interrupt Acknowledge
     * @var InterruptConfig::event_number
     * If the number of events ready for the readout is equal to or
     * greater than event_number, then the digitizer asserts the
     * interrupt request
     * @var InterruptConfig::mode
     * Interrupt release mode: CAEN_DGTZ_IRQ_MODE_RORA (release on
     * register access) or CAEN_DGTZ_IRQ_MODE_ROAK (release on
     * acknowledge)
     */
    struct InterruptConfig
    {
        CAEN_DGTZ_EnaDis_t state;
        uint8_t level;
        uint32_t status_id;
        uint16_t event_number;
        CAEN_DGTZ_IRQMode_t mode;
    };

    /**
     * @struct ZSParams
     * @brief For parameter handling in Set / GetChannelZSParams
     * @var ZSParams::weight
     * Zero Suppression weight*. Used in “Full Suppression based on the
     * integral of the signal” supported only by x724 series.\n
     * CAEN_DGTZ_ZS_FINE = 0 (Fine threshold step; the threshold is the
     * threshold parameter), CAEN_DGTZ_ZS_COARSE = 1 (Coarse threshold
     * step; the threshold is threshold × 64)\n
     * For “Full Suppression based on the signal amplitude” and “Zero
     * Length Encoding” algorithms, the value of weight doesn’t affect
     * the function working.
     * @var ZSParams::threshold
     * Zero Suppression Threshold to set/get depending on the ZS algorithm.
     * @var ZSParams::nsamp
     * Number of samples of the ZS algorithm to set/get.
     */
    struct ZSParams {
        CAEN_DGTZ_ThresholdWeight_t weight; //enum
        int32_t threshold;
        int32_t nsamp;
    };

    /**
     * @struct AIMParams
     * @brief For parameter handling in Set / GetAnalogInspectionMonParams
     * @var AIMParams::channelmask
     * Channel enable mask
     * @var AIMParams::offset
     * DC Offset for the analog output signal
     * @var AIMParams::mf
     * Multiply factor (see definition of CAEN_DGTZ_AnalogMonitorMagnify_t)
     * @var AIMParams::ami
     * Invert Output (see definition of
     * CAEN_DGTZ_AnalogMonitorInspectorInverter_t) 
     */
    struct AIMParams {
        uint32_t channelmask;
        uint32_t offset;
        CAEN_DGTZ_AnalogMonitorMagnify_t mf; //enum
        CAEN_DGTZ_AnalogMonitorInspectorInverter_t ami; //enum
    };

    /**
     * @struct DPPAcquisitionMode
     * @brief For parameter handling in Set / GetDPPAcquisitionMode
     * @var DPPAcquisitionMode::mode
     * The DPP acquisition mode to set/get.\n
     * CAEN_DGTZ_DPP_ACQ_MODE_Oscilloscope = 0L: enables the acquisition
     * of the samples of the digitized waveforms.\n
     * Note: Oscilloscope mode is not supported by DPP-PSD firmware of
     * the 730 digitizer family.\n
     * CAEN_DGTZ_DPP_ACQ_MODE_List = 1L: enables the acquisition of time
     * stamps and energy values for each DPP firmware\n
     * CAEN_DGTZ_DPP_ACQ_MODE_Mixed = 2L: enables the acquisition of
     * both waveforms, energies or charges, and time stamps.
     * @var DPPAcquisitionMode::param
     * The acquisition data to retrieve in the acquisition.
     * Note: CAEN_DGTZ_DPP_SAVE_PARAM_ChargeAndTime is NOT USED
     */
    struct DPPAcquisitionMode {
        CAEN_DGTZ_DPP_AcqMode_t mode;    // enum
        CAEN_DGTZ_DPP_SaveParam_t param; // enum
    };

    /**
     * @struct DPP_SupportedVirtualProbes
     * @brief For parameter handling in GetDPP_SupportedVirtualProbes
     * @var DPP_SupportedVirtualProbes::probes
     * The list of Virtual Probes supported by the trace.\n
     * Note: it must be an array of length MAX_SUPPORTED_PROBES
     * @var DPP_SupportedVirtualProbes::numProbes
     * The number of probes supported by the trace 
     */
    struct DPP_SupportedVirtualProbes {
        /* TODO: should this be changed to pointers like in DPPEvents?
         * if so we should use DPP_SupportedVirtualProbes& supported as
         * argument in getDPP_SupportedVirtualProbes rather than explicit
         * init there.
         */
        int probes[MAX_SUPPORTED_PROBES];
        int numProbes;
    };

    struct EventInfo : CAEN_DGTZ_EventInfo_t {char* data;};

    /**
     * @struct DPPEvents
     * @brief For parameter handling in DPPEvents handling
     * @var DPPEvents::ptr
     * The pointer to the event matrix, which shall be of type:\n
     * CAEN_DGTZ_DPP_PHA_Event_t, for DPP-PHA,\n
     * CAEN_DGTZ_DPP_PSD_Event_t, for DPP-PSD\n
     * CAEN_DGTZ_DPP_CI_Event_t, for DPP-CI\n
     * Note: please refer to the DPP User Manual for the event format
     * description.
     * @var DPPEvents::nEvents
     * Number of events in the events list
     * @var DPPEvents::allocatedSize
     * The size in bytes of the events list
     * @var DPPEvents::elemSize
     * The size in bytes of each element in the events list
     */
    struct DPPEvents
    {
        void** ptr;             // CAEN_DGTZ_DPP_XXX_Event_t* ptr[MAX_DPP_XXX_CHANNEL_SIZE]
        uint32_t* nEvents;      // Number of events pr channel
        uint32_t allocatedSize;
        uint32_t elemSize;
    };

    /**
     * @struct DPPWaveforms
     * @brief For parameter handling in DPPWaveforms handling
     * @var DPPWaveforms::ptr
     * The pointer to the waveform buffer, which shall be of type:\n
     * CAEN_DGTZ_DPP_PHA_Waveforms_t, for DPP-PHA\n
     * CAEN_DGTZ_DPP_PSD_Waveforms_t, for DPP-PSD\n
     * CAEN_DGTZ_DPP_CI_Waveforms_t, for DPP-CI
     * @var DPPWaveforms::allocatedSize
     * The size in bytes of the waveform buffer
     */
    struct DPPWaveforms
    {
        void* ptr;
        uint32_t allocatedSize;
    };

    /**
     * @struct ChannelPairTriggerLogicParams
     * @brief For parameter handling in Set / GetChannelPairTriggerLogic
     * @var ChannelPairTriggerLogicParams::logic
     * The value (or the pointer to , in case of Get) of the
     * CAEN_DGTZ_TrigerLogic_t structure, defining the trigger logic
     * mode (AND / OR).
     * @var ChannelPairTriggerLogicParams::coincidenceWindow
     * The coincidence gate (in ns). It corresponds to the Primitives
     * Gate Length parameter of the WaveCatcher software (see the
     * software User Manual).\n
     * Note: it must be ≥ 15 ns. (it should be a multiple of 5 ns also;
     * otherwise, the library will put the closer multiple of 5 as gate
     * length). Maximum value is 5*255 = 1275 ns.
     */
    struct ChannelPairTriggerLogicParams {
        CAEN_DGTZ_TrigerLogic_t logic;
        uint16_t coincidenceWindow;
    };

    /**
     * @struct TriggerLogicParams
     * @brief For parameter handling in Set / GetTriggerLogic
     * @var TriggerLogicParams::logic
     * The trigger logic to set/get according to the
     * CAEN_DGTZ_TrigerLogic_t structure.
     * @var TriggerLogicParams::majorityLevel
     * Value of the majority level. Allowed values range between 0 and
     * (max num. channel – 1). “0” means more than 0, i.e. ≥ 1.
     */
    struct TriggerLogicParams {
        CAEN_DGTZ_TrigerLogic_t logic;
        uint32_t majorityLevel;
    };

    /**
     * @struct SAMTriggerCountVetoParams
     * @brief For parameter handling in Set / GetSAMTriggerCountVetoParam
     * @var SAMTriggerCountVetoParams::enable
     * enable the trigger counter veto
     * @var SAMTriggerCountVetoParams::vetoWindow
     * programs the time window for the veto
     */
    struct SAMTriggerCountVetoParams {
        CAEN_DGTZ_EnaDis_t enable;
        uint32_t vetoWindow;
    };


    /**
     * @class EasyBase
     * @brief For user-friendly configuration of various bit masks.
     *
     * Base class to handle all the translation between named variables
     * and bit masks. All actual such mask helpers should just inherit
     * from it and implment the specific variable names and specifications.
     *
     * @param EasyBase::mask
     * Initialize from bit mask
     */
    class EasyBase
    {
    protected:
        /* We save variables in a standard map using boost::any values
         * to allow simple variable lookup based on string name but
         * variable value types. The map also allows easy iteration
         * through all variable names and values. */
        typedef std::map<const std::string, boost::any> var_map;
        var_map variables;
        typedef std::pair<std::string, std::pair<uint8_t, uint8_t>> layout_entry;
        typedef std::vector<layout_entry> var_layout;
        var_layout layout;
        /* We save original mask if constructed from one, so that we can
         * use it to only change the relevant mask bit defined and not
         * touch any reserved bits that may or may not need to remain
         * untouched.
         */
        uint32_t origMask = 0x0;
        /**
         * @brief unpack at most 8 bits from 32 bit mask
         * @param mask:
         * bit mask
         * @param bits:
         * number of bits to unpack
         * @param offset:
         * offset to the bits to unpack, i.e. how many bits to right-shift
         */
        const uint8_t unpackBits(const uint32_t mask, const uint8_t bits, const uint8_t offset) const
        {
            assert(bits <= 8);
            assert(bits + offset <= 32);
            return (const uint8_t)((mask >> offset) & ((1 << bits) - 1));
        }
        /**
         * @brief pack at most 8 bits into 32 bit mask
         * @param value:
         * value to pack
         * @param bits:
         * number of bits to pack
         * @param offset:
         * offset to the packed bits, i.e. how many bits to left-shift
         */
        const uint32_t packBits(const uint8_t value, const uint8_t bits, const uint8_t offset) const
        {
            assert(bits <= 8);
            assert(bits + offset <= 32);
            return (const uint32_t)((value & ((1 << bits) - 1)) << offset);
        }

        /* force small integers to full unsigned int for proper printing */
        template<typename T>
        const std::string ui_to_string(const T v) const
        {
            std::stringstream ss;
            ss << unsigned(v);
            return ss.str();
        }
        /* Shared helpers since one constructor cannot reuse the other */
        virtual void initLayout() {
            /* Default layout helper - do nothing */
        };
        virtual void autoInit(const uint32_t mask) {
            uint8_t val;
            for (const auto &keyVal : layout) {
                val = (unpackBits(mask, keyVal.second.first, keyVal.second.second));
                variables[keyVal.first] = (const uint8_t)val;
            }
            /* Make sure parsing is correct */
            if (mask != toBits()) {
                std::cerr << "WARNING: mismatch between mask " << mask << " and parsed value " << toBits() << " in autoInit of " << getClassName() << std::endl;
                std::cerr << variables.size() << " variables, " << layout.size() << " layouts" << std::endl;
                throw std::runtime_error(std::string("autoInit failed for: ")+getClassName());
            }
        };
        void construct() {
            /* Default construct helper - just call initLayout */
            initLayout();
        };
        virtual void constructFromMask(const uint32_t mask) {
            /* Generic construct helper - using possibly overriden
             * layout helper to unpack mask. */
            /* NOTE: a number of masks carry values in the reserved parts.
             *       It's not clear if we break things if we blindly truncate
             *       those during set. So we save the original mask when
             *       constructed from mask and only pack our defined
             *       valid bits into that saved mask during toBits().
             */
            origMask = mask;
            initLayout();
            autoInit(mask);
        };
    public:
        virtual const std::string getClassName() const { return "EasyBase"; };
        EasyBase()
        {
            /* Default constructor - do nothing */
            construct();
        }
        EasyBase(const uint32_t mask)
        {
            /* Construct from bit mask - override in children if needed */
            constructFromMask(mask);
        }
        ~EasyBase()
        {
            /* Do nothing */
        }
        /* NOTE: variable get/set helpers restricted to only declared names */
        virtual const uint8_t getValue(const std::string name) const
        {
            if (variables.find(name) != variables.end()) {
                return boost::any_cast<uint8_t>(variables.at(name));
            }
            std::cerr << "No such variable: " << name << std::endl;
            throw std::invalid_argument(std::string("No such variable: ")+name);
        }
        virtual void setValue(const std::string name, uint8_t val)
        {
            /* TODO: add value check or enforce based on bits in layout? */
            if (variables.find(name) != variables.end()) {
                variables[name] = (const uint8_t)val;
            }
            std::cerr << "No such variable: " << name << std::endl;
            throw std::invalid_argument(std::string("No such variable: ")+name);
        }
        /* Convert to low-level bit mask in line with docs */
        virtual const uint32_t toBits() const
        {
            /* Use saved mask from init if available - it's 0 otherwise */
            uint32_t mask = origMask;
            /* NOTE: we must use variables.at() rather than variables[] here
             * to avoid 'argument discards qualifiers' error during compile */
            for (const auto &keyVal : layout) {
                mask |= packBits(boost::any_cast<uint8_t>(variables.at(keyVal.first)), keyVal.second.first, keyVal.second.second);
            }
            return mask;
        }
        /* Convert to (constant) configuration doc string */
        virtual const std::string toConfHelpString(const std::string name, bool header=false) const
        {
            std::stringstream ss;
            if (header) {
                ss << "### Format for " << name << " is:" << std::endl;
            }
            ss << '{';
            int i = 0;
            /* NOTE: map does not preserve insert order so we use layout
             * for ordering. */
            for (const auto &keyVal : layout) {
                /* NOTE: some registers have forced values that we need
                 * to keep track of but not show user. */
                if ((keyVal.first).substr(0, 12) == "__reserved__") {
                    continue;
                }
                // No comma before first entry
                if (i > 0) {
                    ss << ",";
                }
                ss << keyVal.first;
                i++;
            }
            ss << '}';
            if (header) {
                ss << std::endl;
            }
            return ss.str();
        }
        /* Convert to (constant) configuration string */
        virtual const std::string toConfValueString() const
        {
            /* NOTE: map does not preserve insert order so we use layout
             * for ordering. */
            std::stringstream ss;
            ss << '{';
            int i = 0;
            for (const auto &keyVal : layout) {
                /* NOTE: some registers have forced values that we need
                 * to keep track of but not show user. */
                if ((keyVal.first).substr(0, 12) == "__reserved__") {
                    continue;
                }
                /* NOTE: we must use variables.at rather than variables[] here
                 * to avoid 'argument discards qualifiers' compile problems */
                uint8_t val = boost::any_cast<uint8_t>(variables.at(keyVal.first));
                // No comma before first entry
                if (i > 0) {
                    ss << ",";
                }
                ss << ui_to_string(val);
                i++;
            }
            ss<< '}';
            return ss.str();
        }
        /* Convert to (constant) configuration string */
        virtual const std::string toConfString() const
        {
            std::stringstream ss;
            ss << toConfValueString() << " # " << toConfHelpString(getClassName());
            return ss.str();
        }
    }; // class EasyBase


    /**
     * @class EasyBoardConfiguration
     * @brief For user-friendly configuration of Board Configuration mask.
     *
     * This register contains general settings for the board
     * configuration.
     *
     * @param EasyBoardConfiguration::triggerOverlapSetting
     * Trigger Overlap Setting (default value is 0).\n
     * When two acquisition windows are overlapped, the second trigger
     * can be either accepted or rejected. Options are:\n
     * 0 = Trigger Overlapping Not Allowed (no trigger is accepted until
     * the current acquisition window is finished);\n
     * 1 = Trigger Overlapping Allowed (the current acquisition window
     * is prematurely closed by the arrival of a new trigger).\n
     * NOTE: it is suggested to keep this bit cleared in case of using a
     * DPP firmware.
     * @param EasyBoardConfiguration::testPatternEnable
     * Test Pattern Enable (default value is 0).\n
     * This bit enables a triangular (0<-->3FFF) test wave to be
     * provided at the ADCs input for debug purposes. Options are:\n
     * 0 = disabled\n
     * 1 = enabled.
     * @param EasyBoardConfiguration::selfTriggerPolarity
     * Self-trigger Polarity (default value is 0).\n
     * Options are:\n
     * 0 = Positive (the self-trigger is generated upon the input pulse
     * overthreshold)\n
     * 1 = Negative (the self-trigger is generated upon the input pulse
     * underthreshold).
     */
    class EasyBoardConfiguration : public EasyBase
    {
    protected:
        /* Shared base since one constructor cannot reuse the other */
        /*
         * EasyBoardConfiguration fields:
         * trigger overlap setting in [1], test pattern enable in [3],
         * (reserved forced 1) in [4], self-trigger polarity in [6].
         */
        virtual void initLayout() override
        {
            layout = {
                {"triggerOverlapSetting", {(const uint8_t)1, (const uint8_t)1}},
                {"testPatternEnable", {(const uint8_t)1, (const uint8_t)3}},
                {"__reserved__0_", {(const uint8_t)1, (const uint8_t)4}},
                {"selfTriggerPolarity", {(const uint8_t)1, (const uint8_t)6}}
            };
        }
        /* NOTE: use inherited generic constructFromMask(mask) */
        void construct(const uint8_t triggerOverlapSetting, const uint8_t testPatternEnable, const uint8_t selfTriggerPolarity) {
            initLayout();
            variables = {
                {"triggerOverlapSetting", (const uint8_t)(triggerOverlapSetting & 0x1)},
                {"testPatternEnable", (const uint8_t)(testPatternEnable & 0x1)},
                {"__reserved__0_", (const uint8_t)(0x1)},
                {"selfTriggerPolarity", (const uint8_t)(selfTriggerPolarity & 0x1)}
            };
        };
    public:
        virtual const std::string getClassName() const override { return "EasyBoardConfiguration"; };
        /* Construct using default values from docs */
        EasyBoardConfiguration(const uint8_t triggerOverlapSetting, const uint8_t testPatternEnable, const uint8_t selfTriggerPolarity)
        {
            construct(triggerOverlapSetting, testPatternEnable, selfTriggerPolarity);
        }
        /* Construct from low-level bit mask in line with docs */
        EasyBoardConfiguration(const uint32_t mask)
        {
            constructFromMask(mask);
        }
    }; // class EasyBoardConfiguration


    /**
     * @class EasyDPPBoardConfiguration
     * @brief For user-friendly configuration of DPP Board Configuration mask.
     *
     * This register contains general settings for the DPP board
     * configuration.
     *
     * @param EasyDPPBoardConfiguration::individualTrigger
     * Individual trigger: must be 1
     * @param EasyDPPBoardConfiguration::analogProbe
     * Analog Probe: Selects which signal is associated to the Analog
     * trace in the readout data. Options are:\n
     * 00: Input\n
     * 01: Smoothed Input\n
     * 10: Baseline\n
     * 11: Reserved.
     * @param EasyDPPBoardConfiguration::waveformRecording
     * Waveform Recording: enables the data recording of the
     * waveform. The user must define the number of samples to be saved
     * in the Record Length 0x1n24 register. Options are:\n
     * 0: disabled\n
     * 1: enabled.
     * @param EasyDPPBoardConfiguration::extrasRecording
     * Extras Recording: when enabled the EXTRAS word is saved into the
     * event data. Refer to the ”Channel Aggregate Data Format” chapter
     * of the DPP User Manual for more details about the EXTRAS
     * word. Options are:\n
     * 0: disabled\n
     * 1: enabled.
     * @param EasyDPPBoardConfiguration::timeStampRecording
     * Time Stamp Recording: must be 1
     * @param EasyDPPBoardConfiguration::chargeRecording
     * Charge Recording: must be 1
     * @param EasyDPPBoardConfiguration::externalTriggerMode
     * External Trigger mode. The external trigger mode on TRG-IN
     * connector can be used according to the following options:\n
     * 00: Trigger\n
     * 01: Veto\n
     * 10: An -Veto\n
     * 11: Reserved.
     */
    class EasyDPPBoardConfiguration : public EasyBase
    {
    protected:
        /* Shared base since one constructor cannot reuse the other */
        /*
         * EasyDPPBoardConfiguration fields:
         * (reserved forced 1) in [4], individual trigger in [8],
         * analog probe in [12:13], waveform recording in [16],
         * extras recording in [17], time stamp recording in [18],
         * charge recording in [19], external trigger mode in [20:21].
         */
        virtual void initLayout() override
        {
            layout = {
                {"__reserved__0_", {(const uint8_t)1, (const uint8_t)4}},
                {"individualTrigger", {(const uint8_t)1, (const uint8_t)8}},
                {"analogProbe", {(const uint8_t)2, (const uint8_t)12}},
                {"waveformRecording", {(const uint8_t)1, (const uint8_t)16}},
                {"extrasRecording", {(const uint8_t)1, (const uint8_t)17}},
                {"timeStampRecording", {(const uint8_t)1, (const uint8_t)18}},
                {"chargeRecording", {(const uint8_t)1, (const uint8_t)19}},
                {"externalTriggerMode", {(const uint8_t)2, (const uint8_t)20}}
            };
        }
        /* NOTE: use inherited generic constructFromMask(mask) */
        void construct(const uint8_t individualTrigger, const uint8_t analogProbe, const uint8_t waveformRecording, const uint8_t extrasRecording, const uint8_t timeStampRecording, const uint8_t chargeRecording, const uint8_t externalTriggerMode) {
            initLayout();
            variables = {
                {"__reserved__0_", (const uint8_t)(0x1)},
                {"individualTrigger", (const uint8_t)(individualTrigger & 0x1)},
                {"analogProbe", (const uint8_t)(analogProbe & 0x3)},
                {"waveformRecording", (const uint8_t)(waveformRecording & 0x1)},
                {"extrasRecording", (const uint8_t)(extrasRecording & 0x1)},
                {"timeStampRecording", (const uint8_t)(timeStampRecording & 0x1)},
                {"chargeRecording", (const uint8_t)(chargeRecording & 0x1)},
                {"externalTriggerMode", (const uint8_t)(externalTriggerMode & 0x3)}
            };
        };
    public:
        virtual const std::string getClassName() const override { return "EasyDPPBoardConfiguration"; };
        /* Construct using default values from docs */
        EasyDPPBoardConfiguration(const uint8_t individualTrigger, const uint8_t analogProbe, const uint8_t waveformRecording, const uint8_t extrasRecording, const uint8_t timeStampRecording, const uint8_t chargeRecording, const uint8_t externalTriggerMode)
        {
            construct(individualTrigger, analogProbe, waveformRecording, extrasRecording, timeStampRecording, chargeRecording, externalTriggerMode);
        }
        /* Construct from low-level bit mask in line with docs */
        EasyDPPBoardConfiguration(const uint32_t mask)
        {
            constructFromMask(mask);
        }
    }; // class EasyDPPBoardConfiguration


    /**
     * @class EasyAcquisitionControl
     * @brief For user-friendly configuration of Acquisition Control mask.
     *
     * This register manages the acquisition settings.
     *
     * @param EasyAcquisitionControl::startStopMode
     * Start/Stop Mode Selection (default value is 00). Options are:\n
     * 00 = SW CONTROLLED. Start/stop of the run takes place on software
     * command by setting/resetting bit[2] of this register\n
     * 01 = S-IN/GPI CONTROLLED (S-IN for VME, GPI for Desktop/NIM). If
     * the acquisition is armed (i.e. bit[2] = 1), then the acquisition
     * starts when S-IN/GPI is asserted and stops when S-IN/GPI returns
     * inactive. If bit[2] = 0, the acquisition is always off\n
     * 10 = FIRST TRIGGER CONTROLLED. If the acquisition is armed
     * (i.e. bit[2] = 1), then the run starts on the first trigger pulse
     * (rising edge on TRG-IN); this pulse is not used as input trigger,
     * while actual triggers start from the second pulse. The stop of
     * Run must be SW controlled (i.e. bit[2] = 0)\n
     * 11 = LVDS CONTROLLED (VME only). It is like option 01 but using
     * LVDS (RUN) instead of S-IN.\n
     * The LVDS can be set using registers 0x811C and 0x81A0.
     * @param EasyAcquisitionControl::acquisitionStartArm
     * Acquisition Start/Arm (default value is 0).\n
     * When bits[1:0] = 00, this bit acts as a Run Start/Stop. When
     * bits[1:0] = 01, 10, 11, this bit arms the acquisition and the
     * actual Start/Stop is controlled by an external signal. Options
     * are:\n
     * 0 = Acquisition STOP (if bits[1:0]=00); Acquisition DISARMED (others)\n
     * 1 = Acquisition RUN (if bits[1:0]=00); Acquisition ARMED (others).
     * @param EasyAcquisitionControl::triggerCountingMode
     * Trigger Counting Mode (default value is 0). Through this bit it
     * is possible to count the reading requests from channels to mother
     * board. The reading requests may come from the following options:\n
     * 0 = accepted triggers from combination of channels\n
     * 1 = triggers from combination of channels, in addition to TRG-IN
     * and SW TRG.
     * @param EasyAcquisitionControl::memoryFullModeSelection
     * Memory Full Mode Selection (default value is 0). Options are:\n
     * 0 = NORMAL. The board is full whenever all buffers are full\n
     * 1 = ONE BUFFER FREE. The board is full whenever Nb-1 buffers are
     * full, where Nb is the overall number of buffers in which the
     * channel memory is divided.
     * @param EasyAcquisitionControl::pLLRefererenceClockSource
     * PLL Reference Clock Source (Desktop/NIM only). Default value is 0.\n
     * Options are:\n
     * 0 = internal oscillator (50 MHz)\n
     * 1 = external clock from front panel CLK-IN connector.\n
     * NOTE: this bit is reserved in case of VME boards.
     * @param EasyAcquisitionControl::lVDSIOBusyEnable
     * LVDS I/O Busy Enable (VME only). Default value is 0.\n
     * The LVDS I/Os can be programmed to accept a Busy signal as input,
     * or to propagate it as output. Options are:\n
     * 0 = disabled\n
     * 1 = enabled.\n
     * NOTE: this bit is supported only by VME boards and meaningful
     * only if the LVDS new features are enabled (bit[8]=1 of register
     * 0x811C). Register 0x81A0 should also be configured for nBusy/nVeto.
     * @param EasyAcquisitionControl::lVDSVetoEnable
     * LVDS I/O Veto Enable (VME only). Default value is 0.\n
     * The LVDS I/Os can be programmed to accept a Veto signal as input,
     * or to transfer it as output. Options are:\n
     * 0 = disabled (default)\n
     * 1 = enabled.\n
     * NOTE: this bit is supported only by VME boards and meaningful
     * only if the LVDS new features are enabled (bit[8]=1 of register
     * 0x811C). Register 0x81A0 should also be configured for nBusy/nVeto.
     * @param EasyAcquisitionControl::lVDSIORunInEnable
     * LVDS I/O RunIn Enable Mode (VME only). Default value is 0.\n
     * The LVDS I/Os can be programmed to accept a RunIn signal as
     * input, or to transfer it as output. Options are:\n
     * 0 = starts on RunIn level (default)\n
     * 1 = starts on RunIn rising edge.\n
     * NOTE: this bit is supported only by VME boards and meaningful
     * only if the LVDS new features are enabled (bit[8]=1 of register
     * 0x811C). Register 0x81A0 must also be configured for nBusy/nVeto.
     */
    class EasyAcquisitionControl : public EasyBase
    {
    protected:
        /* Shared base since one constructor cannot reuse the other */
        /*
         * EasyAcquisitionControl fields:
         * start/stop mode [0:1], acquisition start/arm in [2],
         * trigger counting mode in [3], memory full mode selection in [5],
         * PLL reference clock source in [6], LVDS I/O busy enable in [8],
         * LVDS veto enable in [9], LVDS I/O RunIn enable in [11].
         */
        virtual void initLayout() override
        {
            layout = {{"startStopMode", {(const uint8_t)2, (const uint8_t)0}},
                      {"acquisitionStartArm", {(const uint8_t)1, (const uint8_t)2}},
                      {"triggerCountingMode", {(const uint8_t)1, (const uint8_t)3}},
                      {"memoryFullModeSelection", {(const uint8_t)1, (const uint8_t)5}},
                      {"pLLRefererenceClockSource", {(const uint8_t)1, (const uint8_t)6}},
                      {"lVDSIOBusyEnable", {(const uint8_t)1, (const uint8_t)8}},
                      {"lVDSVetoEnable", {(const uint8_t)1, (const uint8_t)9}},
                      {"lVDSIORunInEnable", {(const uint8_t)1, (const uint8_t)11}}
            };
        }
        /* NOTE: use inherited generic constructFromMask(mask) */
        void construct(const uint8_t startStopMode, const uint8_t acquisitionStartArm, const uint8_t triggerCountingMode, const uint8_t memoryFullModeSelection, const uint8_t pLLRefererenceClockSource, const uint8_t lVDSIOBusyEnable, const uint8_t lVDSVetoEnable, const uint8_t lVDSIORunInEnable) {
            initLayout();
            variables = {
                {"startStopMode", (const uint8_t)(startStopMode & 0x3)},
                {"acquisitionStartArm", (const uint8_t)(acquisitionStartArm & 0x1)},
                {"triggerCountingMode", (const uint8_t)(triggerCountingMode & 0x1)},
                {"memoryFullModeSelection", (const uint8_t)(memoryFullModeSelection & 0x1)},
                {"pLLRefererenceClockSource", (const uint8_t)(pLLRefererenceClockSource & 0x1)},
                {"lVDSIOBusyEnable", (const uint8_t)(lVDSIOBusyEnable & 0x1)},
                {"lVDSVetoEnable", (const uint8_t)(lVDSVetoEnable & 0x1)},
                {"lVDSIORunInEnable", (const uint8_t)(lVDSIORunInEnable & 0x1)}
            };
        };
    public:
        virtual const std::string getClassName() const override { return "EasyAcquisitionControl"; };
        /* Construct using default values from docs */
        EasyAcquisitionControl(const uint8_t startStopMode, const uint8_t acquisitionStartArm, const uint8_t triggerCountingMode, const uint8_t memoryFullModeSelection, const uint8_t pLLRefererenceClockSource, const uint8_t lVDSIOBusyEnable, const uint8_t lVDSVetoEnable, const uint8_t lVDSIORunInEnable)
        {
            construct(startStopMode, acquisitionStartArm, triggerCountingMode, memoryFullModeSelection, pLLRefererenceClockSource, lVDSIOBusyEnable, lVDSVetoEnable, lVDSIORunInEnable);
        }
        /* Construct from low-level bit mask in line with docs */
        EasyAcquisitionControl(const uint32_t mask)
        {
            constructFromMask(mask);
        }
    }; // class EasyAcquisitionControl


    /**
     * @class EasyDPPAcquisitionControl
     * @brief For user-friendly configuration of Acquisition Control mask.
     *
     * This register manages the acquisition settings.
     *
     * @param EasyDPPAcquisitionControl::startStopMode
     * Start/Stop Mode Selection (default value is 00). Options are:\n
     * 00 = SW CONTROLLED. Start/stop of the run takes place on software
     * command by setting/resetting bit[2] of this register\n
     * 01 = S-IN/GPI CONTROLLED (S-IN for VME, GPI for Desktop/NIM). If
     * the acquisition is armed (i.e. bit[2] = 1), then the acquisition
     * starts when S-IN/GPI is asserted and stops when S-IN/GPI returns
     * inactive. If bit[2] = 0, the acquisition is always off\n
     * 10 = FIRST TRIGGER CONTROLLED. If the acquisition is armed
     * (i.e. bit[2] = 1), then the run starts on the first trigger pulse
     * (rising edge on TRG-IN); this pulse is not used as input trigger,
     * while actual triggers start from the second pulse. The stop of
     * Run must be SW controlled (i.e. bit[2] = 0)\n
     * 11 = LVDS CONTROLLED (VME only). It is like option 01 but using
     * LVDS (RUN) instead of S-IN.\n
     * The LVDS can be set using registers 0x811C and 0x81A0.
     * @param EasyDPPAcquisitionControl::acquisitionStartArm
     * Acquisition Start/Arm (default value is 0).\n
     * When bits[1:0] = 00, this bit acts as a Run Start/Stop. When
     * bits[1:0] = 01, 10, 11, this bit arms the acquisition and the
     * actual Start/Stop is controlled by an external signal. Options
     * are:\n
     * 0 = Acquisition STOP (if bits[1:0]=00); Acquisition DISARMED (others)\n
     * 1 = Acquisition RUN (if bits[1:0]=00); Acquisition ARMED (others).
     * @param EasyDPPAcquisitionControl::triggerCountingMode
     * Trigger Counting Mode (default value is 0). Through this bit it
     * is possible to count the reading requests from channels to mother
     * board. The reading requests may come from the following options:\n
     * 0 = accepted triggers from combination of channels\n
     * 1 = triggers from combination of channels, in addition to TRG-IN
     * and SW TRG.
     * @param EasyDPPAcquisitionControl::pLLRefererenceClockSource
     * PLL Reference Clock Source (Desktop/NIM only). Default value is 0.\n
     * Options are:\n
     * 0 = internal oscillator (50 MHz)\n
     * 1 = external clock from front panel CLK-IN connector.\n
     * NOTE: this bit is reserved in case of VME boards.
     * @param EasyDPPAcquisitionControl::lVDSIOBusyEnable
     * LVDS I/O Busy Enable (VME only). Default value is 0.\n
     * The LVDS I/Os can be programmed to accept a Busy signal as input,
     * or to propagate it as output. Options are:\n
     * 0 = disabled\n
     * 1 = enabled.\n
     * NOTE: this bit is supported only by VME boards and meaningful
     * only if the LVDS new features are enabled (bit[8]=1 of register
     * 0x811C). Register 0x81A0 should also be configured for nBusy/nVeto.
     * @param EasyDPPAcquisitionControl::lVDSVetoEnable
     * LVDS I/O Veto Enable (VME only). Default value is 0.\n
     * The LVDS I/Os can be programmed to accept a Veto signal as input,
     * or to transfer it as output. Options are:\n
     * 0 = disabled (default)\n
     * 1 = enabled.\n
     * NOTE: this bit is supported only by VME boards and meaningful
     * only if the LVDS new features are enabled (bit[8]=1 of register
     * 0x811C). Register 0x81A0 should also be configured for nBusy/nVeto.
     * @param EasyDPPAcquisitionControl::lVDSIORunInEnable
     * LVDS I/O RunIn Enable Mode (VME only). Default value is 0.\n
     * The LVDS I/Os can be programmed to accept a RunIn signal as
     * input, or to transfer it as output. Options are:\n
     * 0 = starts on RunIn level (default)\n
     * 1 = starts on RunIn rising edge.\n
     * NOTE: this bit is supported only by VME boards and meaningful
     * only if the LVDS new features are enabled (bit[8]=1 of register
     * 0x811C). Register 0x81A0 must also be configured for nBusy/nVeto.
     */
    class EasyDPPAcquisitionControl : public EasyBase
    {
    protected:
        /* Shared base since one constructor cannot reuse the other */
        /*
         * EasyDPPAcquisitionControl fields:
         * start/stop mode [0:1], acquisition start/arm in [2],
         * trigger counting mode in [3], PLL reference clock
         * source in [6], LVDS I/O busy enable in [8],
         * LVDS veto enable in [9], LVDS I/O RunIn enable in [11].
         */
        virtual void initLayout() override
        {
            layout = {{"startStopMode", {(const uint8_t)2, (const uint8_t)0}},
                      {"acquisitionStartArm", {(const uint8_t)1, (const uint8_t)2}},
                      {"triggerCountingMode", {(const uint8_t)1, (const uint8_t)3}},
                      {"pLLRefererenceClockSource", {(const uint8_t)1, (const uint8_t)6}},
                      {"lVDSIOBusyEnable", {(const uint8_t)1, (const uint8_t)8}},
                      {"lVDSVetoEnable", {(const uint8_t)1, (const uint8_t)9}},
                      {"lVDSIORunInEnable", {(const uint8_t)1, (const uint8_t)11}}
            };
        }
        /* NOTE: use inherited generic constructFromMask(mask) */
        void construct(const uint8_t startStopMode, const uint8_t acquisitionStartArm, const uint8_t triggerCountingMode, const uint8_t pLLRefererenceClockSource, const uint8_t lVDSIOBusyEnable, const uint8_t lVDSVetoEnable, const uint8_t lVDSIORunInEnable) {
            initLayout();
            variables = {
                {"startStopMode", (const uint8_t)(startStopMode & 0x3)},
                {"acquisitionStartArm", (const uint8_t)(acquisitionStartArm & 0x1)},
                {"triggerCountingMode", (const uint8_t)(triggerCountingMode & 0x1)},
                {"pLLRefererenceClockSource", (const uint8_t)(pLLRefererenceClockSource & 0x1)},
                {"lVDSIOBusyEnable", (const uint8_t)(lVDSIOBusyEnable & 0x1)},
                {"lVDSVetoEnable", (const uint8_t)(lVDSVetoEnable & 0x1)},
                {"lVDSIORunInEnable", (const uint8_t)(lVDSIORunInEnable & 0x1)}
            };
        };
    public:
        virtual const std::string getClassName() const override { return "EasyDPPAcquisitionControl"; };
        /* Construct using default values from docs */
        EasyDPPAcquisitionControl(const uint8_t startStopMode, const uint8_t acquisitionStartArm, const uint8_t triggerCountingMode, const uint8_t pLLRefererenceClockSource, const uint8_t lVDSIOBusyEnable, const uint8_t lVDSVetoEnable, const uint8_t lVDSIORunInEnable)
        {
            construct(startStopMode, acquisitionStartArm, triggerCountingMode, pLLRefererenceClockSource, lVDSIOBusyEnable, lVDSVetoEnable, lVDSIORunInEnable);
        }
        /* Construct from low-level bit mask in line with docs */
        EasyDPPAcquisitionControl(const uint32_t mask)
        {
            constructFromMask(mask);
        }
    }; // class EasyDPPAcquisitionControl


    /**
     * @class EasyAcquisitionStatus
     * @brief For user-friendly configuration of Acquisition Status mask.
     *
     * This register monitors a set of conditions related to the
     * acquisition status.
     *
     * @param EasyAcquisitionStatus::acquisitionStatus
     * Acquisition Status. It reflects the status of the acquisition and
     * drivers the front panel ’RUN’ LED. Options are:\n
     * 0 = acquisition is stopped (’RUN’ is off)\n
     * 1 = acquisition is running (’RUN’ lits).
     * @param EasyAcquisitionStatus::eventReady
     * Event Ready. Indicates if any events are available for
     * readout. Options are:\n
     * 0 = no event is available for readout\n
     * 1 = at least one event is available for readout.\n
     * NOTE: the status of this bit must be considered when managing the
     * readout from the digitizer.
     * @param EasyAcquisitionStatus::eventFull
     * Event Full. Indicates if at least one channel has reached the
     * FULL condition. Options are:\n
     * 0 = no channel has reached the FULL condition\n
     * 1 = the maximum number of events to be read is reached.
     * @param EasyAcquisitionStatus::clockSource
     * Clock Source. Indicates the clock source status. Options are:\n
     * 0 = internal (PLL uses the internal 50 MHz oscillator as reference)\n
     * 1 = external (PLL uses the external clock on CLK-IN connector as
     * reference).
     * @param EasyAcquisitionStatus::pLLBypassMode
     * PLL Bypass Mode. This bit drives the front panel 'PLL BYPS' LED.\n
     * Options are:\n
     * 0 = PLL bypass mode is not ac ve ('PLL BYPS' is off)\n
     * 1 = PLL bypass mode is ac ve and the VCXO frequency directly
     * drives the clock distribution chain ('PLL BYPS' lits).\n
     * WARNING: before operating in PLL Bypass Mode, it is recommended
     * to contact CAEN for feasibility.
     * @param EasyAcquisitionStatus::pLLUnlockDetect
     * PLL Unlock Detect. This bit flags a PLL unlock condition. Options
     * are:\n
     * 0 = PLL has had an unlock condition since the last register read
     * access\n
     * 1 = PLL has not had any unlock condition since the last register
     * read access.\n
     * NOTE: flag can be restored to 1 via read access to register 0xEF04.
     * @param EasyAcquisitionStatus::boardReady
     * Board Ready. This flag indicates if the board is ready for
     * acquisition (PLL and ADCs are correctly synchronised). Options
     * are:\n
     * 0 = board is not ready to start the acquisition\n
     * 1 = board is ready to start the acquisition.\n
     * NOTE: this bit should be checked a er software reset to ensure
     * that the board will enter immediately in run mode a er the RUN
     * mode setting; otherwise, a latency between RUN mode setting and
     * Acquisition start might occur.
     * @param EasyAcquisitionStatus::s_IN
     * S-IN (VME boards) or GPI (DT/NIM boards) Status. Reads the
     * current logical level on S-IN (GPI) front panel connector.
     * @param EasyAcquisitionStatus::tRG_IN
     * TRG-IN Status. Reads the current logical level on TRG-IN front
     * panel connector.
     */
    class EasyAcquisitionStatus : public EasyBase
    {
    protected:
        /* Shared base since one constructor cannot reuse the other */
        /*
         * EasyAcquisitionStatus fields:
         * acquisition status [2], event ready [3], event full in [4],
         * clock source in [5], PLL bypass mode in [6],
         * PLL unlock detect in [7], board ready in [8], S-In in [15],
         * TRG-IN in [16].
         */
        virtual void initLayout() override
        {
            layout = {
                {"acquisitionStatus", {(const uint8_t)1, (const uint8_t)2}},
                {"eventReady", {(const uint8_t)1, (const uint8_t)3}},
                {"eventFull", {(const uint8_t)1, (const uint8_t)4}},
                {"clockSource", {(const uint8_t)1, (const uint8_t)5}},
                {"pLLBypassMode", {(const uint8_t)1, (const uint8_t)6}},
                {"pLLUnlockDetect", {(const uint8_t)1, (const uint8_t)7}},
                {"boardReady", {(const uint8_t)1, (const uint8_t)8}},
                {"s_IN", {(const uint8_t)1, (const uint8_t)15}},
                {"tRG_IN", {(const uint8_t)1, (const uint8_t)16}}
            };
        }
        /* NOTE: use inherited generic constructFromMask(mask) */
        void construct(const uint8_t acquisitionStatus, const uint8_t eventReady, const uint8_t eventFull, const uint8_t clockSource, const uint8_t pLLBypassMode, const uint8_t pLLUnlockDetect, const uint8_t boardReady, const uint8_t s_IN, const uint8_t tRG_IN) {
            initLayout();
            variables = {
                {"acquisitionStatus", (const uint8_t)(acquisitionStatus & 0x1)},
                {"eventReady", (const uint8_t)(eventReady & 0x1)},
                {"eventFull", (const uint8_t)(eventFull & 0x1)},
                {"clockSource", (const uint8_t)(clockSource & 0x1)},
                {"pLLBypassMode", (const uint8_t)(pLLBypassMode & 0x1)},
                {"pLLUnlockDetect", (const uint8_t)(pLLUnlockDetect & 0x1)},
                {"boardReady", (const uint8_t)(boardReady & 0x1)},
                {"s_IN", (const uint8_t)(s_IN & 0x1)},
                {"tRG_IN", (const uint8_t)(tRG_IN & 0x1)}
            };
        };
    public:
        virtual const std::string getClassName() const override { return "EasyAcquisitionStatus"; };
        /* Construct using default values from docs */
        EasyAcquisitionStatus(const uint8_t acquisitionStatus, const uint8_t eventReady, const uint8_t eventFull, const uint8_t clockSource, const uint8_t pLLBypassMode, const uint8_t pLLUnlockDetect, const uint8_t boardReady, const uint8_t s_IN, const uint8_t tRG_IN)
        {
            construct(acquisitionStatus, eventReady, eventFull, clockSource, pLLBypassMode, pLLUnlockDetect, boardReady, s_IN, tRG_IN);
        }
        /* Construct from low-level bit mask in line with docs */
        EasyAcquisitionStatus(const uint32_t mask)
        {
            constructFromMask(mask);
        }
    }; // class EasyAcquisitionStatus


    /**
     * @class EasyDPPAcquisitionStatus
     * @brief For user-friendly configuration of Acquisition Status mask.
     *
     * This register monitors a set of conditions related to the
     * acquisition status.
     *
     * @param EasyDPPAcquisitionStatus::acquisitionStatus
     * Acquisition Status. It reflects the status of the acquisition and
     * drivers the front panel ’RUN’ LED. Options are:\n
     * 0 = acquisition is stopped (’RUN’ is off)\n
     * 1 = acquisition is running (’RUN’ lits).
     * @param EasyDPPAcquisitionStatus::eventReady
     * Event Ready. Indicates if any events are available for
     * readout. Options are:\n
     * 0 = no event is available for readout\n
     * 1 = at least one event is available for readout.\n
     * NOTE: the status of this bit must be considered when managing the
     * readout from the digitizer.
     * @param EasyDPPAcquisitionStatus::eventFull
     * Event Full. Indicates if at least one channel has reached the
     * FULL condition. Options are:\n
     * 0 = no channel has reached the FULL condition\n
     * 1 = the maximum number of events to be read is reached.
     * @param EasyDPPAcquisitionStatus::clockSource
     * Clock Source. Indicates the clock source status. Options are:\n
     * 0 = internal (PLL uses the internal 50 MHz oscillator as reference)\n
     * 1 = external (PLL uses the external clock on CLK-IN connector as
     * reference).
     * @param EasyDPPAcquisitionStatus::pLLUnlockDetect
     * PLL Unlock Detect. This bit flags a PLL unlock condition. Options
     * are:\n
     * 0 = PLL has had an unlock condition since the last register read
     * access\n
     * 1 = PLL has not had any unlock condition since the last register
     * read access.\n
     * NOTE: flag can be restored to 1 via read access to register 0xEF04.
     * @param EasyDPPAcquisitionStatus::boardReady
     * Board Ready. This flag indicates if the board is ready for
     * acquisition (PLL and ADCs are correctly synchronised). Options
     * are:\n
     * 0 = board is not ready to start the acquisition\n
     * 1 = board is ready to start the acquisition.\n
     * NOTE: this bit should be checked a er software reset to ensure
     * that the board will enter immediately in run mode a er the RUN
     * mode setting; otherwise, a latency between RUN mode setting and
     * Acquisition start might occur.
     * @param EasyDPPAcquisitionStatus::s_IN
     * S-IN (VME boards) or GPI (DT/NIM boards) Status. Reads the
     * current logical level on S-IN (GPI) front panel connector.
     * @param EasyDPPAcquisitionStatus::tRG_IN
     * TRG-IN Status. Reads the current logical level on TRG-IN front
     * panel connector.
     */
    class EasyDPPAcquisitionStatus : public EasyBase
    {
    protected:
        /* Shared base since one constructor cannot reuse the other */
        /*
         * EasyDPPAcquisitionStatus fields:
         * acquisition status [2], event ready [3], event full in [4],
         * clock source in [5], PLL unlock detect in [7], board ready in [8],
         * S-In in [15], TRG-IN in [16].
         */
        virtual void initLayout() override
        {
            layout = {
                {"acquisitionStatus", {(const uint8_t)1, (const uint8_t)2}},
                {"eventReady", {(const uint8_t)1, (const uint8_t)3}},
                {"eventFull", {(const uint8_t)1, (const uint8_t)4}},
                {"clockSource", {(const uint8_t)1, (const uint8_t)5}},
                {"pLLUnlockDetect", {(const uint8_t)1, (const uint8_t)7}},
                {"boardReady", {(const uint8_t)1, (const uint8_t)8}},
                {"s_IN", {(const uint8_t)1, (const uint8_t)15}},
                {"tRG_IN", {(const uint8_t)1, (const uint8_t)16}}
            };
        }
        /* NOTE: use inherited generic constructFromMask(mask) */
        void construct(const uint8_t acquisitionStatus, const uint8_t eventReady, const uint8_t eventFull, const uint8_t clockSource, const uint8_t pLLUnlockDetect, const uint8_t boardReady, const uint8_t s_IN, const uint8_t tRG_IN) {
            initLayout();
            variables = {
                {"acquisitionStatus", (const uint8_t)(acquisitionStatus & 0x1)},
                {"eventReady", (const uint8_t)(eventReady & 0x1)},
                {"eventFull", (const uint8_t)(eventFull & 0x1)},
                {"clockSource", (const uint8_t)(clockSource & 0x1)},
                {"pLLUnlockDetect", (const uint8_t)(pLLUnlockDetect & 0x1)},
                {"boardReady", (const uint8_t)(boardReady & 0x1)},
                {"s_IN", (const uint8_t)(s_IN & 0x1)},
                {"tRG_IN", (const uint8_t)(tRG_IN & 0x1)}
            };
        };
    public:
        virtual const std::string getClassName() const override { return "EasyDPPAcquisitionStatus"; };
        /* Construct using default values from docs */
        EasyDPPAcquisitionStatus(const uint8_t acquisitionStatus, const uint8_t eventReady, const uint8_t eventFull, const uint8_t clockSource, const uint8_t pLLUnlockDetect, const uint8_t boardReady, const uint8_t s_IN, const uint8_t tRG_IN)
        {
            construct(acquisitionStatus, eventReady, eventFull, clockSource, pLLUnlockDetect, boardReady, s_IN, tRG_IN);
        }
        /* Construct from low-level bit mask in line with docs */
        EasyDPPAcquisitionStatus(const uint32_t mask)
        {
            constructFromMask(mask);
        }
    }; // class EasyDPPAcquisitionStatus


    /**
     * @class EasyGlobalTriggerMask
     * @brief For user-friendly configuration of Global Trigger Mask.
     *
     * This register sets which signal can contribute to the global
     * trigger generation.
     *
     * @param EasyGlobalTriggerMask::groupTriggerMask
     * Bit n corresponds to the trigger request from group n that
     * participates to the global trigger generation (n = 0,...,3 for DT
     * and NIM; n = 0,...,7 for VME boards). Options are:\n
     * 0 = trigger request is not sensed for global trigger generation\n
     * 1 = trigger request participates in the global trigger generation.\n
     * NOTE: in case of DT and NIMboards, only bits[3:0] are meaningful,
     * while bits[7:4] are reserved.
     * @param EasyGlobalTriggerMask::majorityCoincidenceWindow
     * Majority Coincidence Window. Sets the me window (in units of the
     * trigger clock) for the majority coincidence. Majority level must
     * be set different from 0 through bits[26:24].
     * @param EasyGlobalTriggerMask::majorityLevel
     * Majority Level. Sets the majority level for the global trigger
     * generation. For a level m, the trigger fires when at least m+1 of
     * the enabled trigger requests (bits[7:0] or [3:0]) are
     * over-threshold inside the majority coincidence window
     * (bits[23:20]).\n
     * NOTE: the majority level must be smaller than the number of
     * trigger requests enabled via bits[7:0] mask (or [3:0]).
     * @param EasyGlobalTriggerMask::lVDSTrigger
     * LVDS Trigger (VME boards only). When enabled, the trigger from
     * LVDS I/O participates to the global trigger generation (in logic
     * OR). Options are:\n
     * 0 = disabled\n
     * 1 = enabled.
     * @param EasyGlobalTriggerMask::externalTrigger
     * External Trigger (default value is 1). When enabled, the external
     * trigger on TRG-IN participates to the global trigger generation
     * in logic OR with the other enabled signals. Options are:\n
     * 0 = disabled\n
     * 1 = enabled.
     * @param EasyGlobalTriggerMask::softwareTrigger
     * Software Trigger (default value is 1). When enabled, the software
     * trigger participates to the global trigger signal generation in
     * logic OR with the other enabled signals. Options are:\n
     * 0 = disabled\n
     * 1 = enabled.
     */
    class EasyGlobalTriggerMask : public EasyBase
    {
    protected:
        /* Shared base since one constructor cannot reuse the other */
        /*
         * EasyGlobalTriggerMask fields:
         * groupTriggerMask in [0:7], majorityCoincidenceWindow in [20:23],
         * majorityLevel in [24:26], LVDS trigger in [29],
         * external trigger in [30], software trigger in [31].
         */
        virtual void initLayout() override
        {
            layout = {
                {"groupTriggerMask", {(const uint8_t)8, (const uint8_t)0}},
                {"majorityCoincidenceWindow", {(const uint8_t)4, (const uint8_t)20}},
                {"majorityLevel", {(const uint8_t)3, (const uint8_t)24}},
                {"lVDSTrigger", {(const uint8_t)1, (const uint8_t)29}},
                {"externalTrigger", {(const uint8_t)1, (const uint8_t)30}},
                {"softwareTrigger", {(const uint8_t)1, (const uint8_t)31}}
            };
        }
        /* NOTE: use inherited generic constructFromMask(mask) */
        void construct(const uint8_t groupTriggerMask, const uint8_t majorityCoincidenceWindow, const uint8_t majorityLevel, const uint8_t lVDSTrigger, const uint8_t externalTrigger, const uint8_t softwareTrigger) {
            initLayout();
            variables = {
                {"groupTriggerMask", (const uint8_t)(groupTriggerMask & 0xFF)},
                {"majorityCoincidenceWindow", (const uint8_t)(majorityCoincidenceWindow & 0xF)},
                {"majorityLevel", (const uint8_t)(majorityLevel & 0x7)},
                {"lVDSTrigger", (const uint8_t)(lVDSTrigger & 0x1)},
                {"externalTrigger", (const uint8_t)(externalTrigger & 0x1)},
                {"softwareTrigger", (const uint8_t)(softwareTrigger & 0x1)}
            };
        };
    public:
        virtual const std::string getClassName() const override { return "EasyGlobalTriggerMask"; };
        /* Construct using default values from docs */
        EasyGlobalTriggerMask(const uint8_t groupTriggerMask, const uint8_t majorityCoincidenceWindow, const uint8_t majorityLevel, const uint8_t lVDSTrigger, const uint8_t externalTrigger, const uint8_t softwareTrigger)
        {
            construct(groupTriggerMask, majorityCoincidenceWindow, majorityLevel, lVDSTrigger, externalTrigger, softwareTrigger);
        }
        /* Construct from low-level bit mask in line with docs */
        EasyGlobalTriggerMask(const uint32_t mask)
        {
            constructFromMask(mask);
        }
    }; // class EasyGlobalTriggerMask


    /**
     * @class EasyDPPGlobalTriggerMask
     * @brief For user-friendly configuration of Global Trigger Mask.
     *
     * This register sets which signal can contribute to the global
     * trigger generation.
     *
     * @param EasyDPPGlobalTriggerMask::lVDSTrigger
     * LVDS Trigger (VME boards only). When enabled, the trigger from
     * LVDS I/O participates to the global trigger generation (in logic
     * OR). Options are:\n
     * 0 = disabled\n
     * 1 = enabled.
     * @param EasyDPPGlobalTriggerMask::externalTrigger
     * External Trigger (default value is 1). When enabled, the external
     * trigger on TRG-IN participates to the global trigger generation
     * in logic OR with the other enabled signals. Options are:\n
     * 0 = disabled\n
     * 1 = enabled.
     * @param EasyDPPGlobalTriggerMask::softwareTrigger
     * Software Trigger (default value is 1). When enabled, the software
     * trigger participates to the global trigger signal generation in
     * logic OR with the other enabled signals. Options are:\n
     * 0 = disabled\n
     * 1 = enabled.
     */
    class EasyDPPGlobalTriggerMask : public EasyBase
    {
    protected:
        /* Shared base since one constructor cannot reuse the other */
        /*
         * EasyDPPGlobalTriggerMask fields:
         * LVDS trigger in [29], external trigger in [30],
         * software trigger in [31].
         */
        virtual void initLayout() override
        {
            layout = {
                {"lVDSTrigger", {(const uint8_t)1, (const uint8_t)29}},
                {"externalTrigger", {(const uint8_t)1, (const uint8_t)30}},
                {"softwareTrigger", {(const uint8_t)1, (const uint8_t)31}}
            };
        }
        /* NOTE: use inherited generic constructFromMask(mask) */
        void construct(const uint8_t lVDSTrigger, const uint8_t externalTrigger, const uint8_t softwareTrigger) {
            initLayout();
            variables = {
                {"lVDSTrigger", (const uint8_t)(lVDSTrigger & 0x1)},
                {"externalTrigger", (const uint8_t)(externalTrigger & 0x1)},
                {"softwareTrigger", (const uint8_t)(softwareTrigger & 0x1)}
            };
        };
    public:
        virtual const std::string getClassName() const override { return "EasyDPPGlobalTriggerMask"; };
        /* Construct using default values from docs */
        EasyDPPGlobalTriggerMask(const uint8_t lVDSTrigger, const uint8_t externalTrigger, const uint8_t softwareTrigger)
        {
            construct(lVDSTrigger, externalTrigger, softwareTrigger);
        }
        /* Construct from low-level bit mask in line with docs */
        EasyDPPGlobalTriggerMask(const uint32_t mask)
        {
            constructFromMask(mask);
        }
    }; // class EasyDPPGlobalTriggerMask


    /**
     * @class EasyFrontPanelTRGOUTEnableMask
     * @brief For user-friendly configuration of Front Panel TRG-OUT Enable Mask.
     *
     * This register sets which signal can contribute to
     * generate the signal on the front panel TRG-OUT LEMO connector
     * (GPO in case of DT and NIM boards).
     *
     * @param EasyFrontPanelTRGOUTEnableMask::groupTriggerMask
     * This mask sets the trigger requests participating in the TRG-OUT
     * (GPO). Bit n corresponds to the trigger request from group n.\n
     * Options are:\n
     * 0 = Trigger request does not participate to the TRG-OUT (GPO) signal\n
     * 1 = Trigger request participates to the TRG-OUT (GPO) signal.\n
     * NOTE: in case of DT and NIM boards, only bits[3:0] are meaningful
     * while bis[7:4] are reserved.
     * @param EasyFrontPanelTRGOUTEnableMask::tRGOUTGenerationLogic
     * TRG-OUT (GPO) Generation Logic. The enabled trigger requests
     * (bits [7:0] or [3:0]) can be combined to generate the TRG-OUT
     * (GPO) signal. Options are:\n
     * 00 = OR\n
     * 01 = AND\n
     * 10 = Majority\n
     * 11 = reserved.
     * @param EasyFrontPanelTRGOUTEnableMask::majorityLevel
     * Majority Level. Sets the majority level for the TRG-OUT (GPO)
     * signal generation. Allowed level values are between 0 and 7 for
     * VME boards, and between 0 and 3 for DT and NIM boards. For a
     * level m, the trigger fires when at least m+1 of the trigger
     * requests are generated by the enabled channels (bits [7:0] or [3:0]).
     * @param EasyFrontPanelTRGOUTEnableMask::lVDSTriggerEnable
     * LVDS Trigger Enable (VME boards only). If the LVDS I/Os are
     * programmed as outputs, they can participate in the TRG-OUT (GPO)
     * signal generation. They are in logic OR with the other enabled
     * signals. Options are:\n
     * 0 = disabled\n
     * 1 = enabled.
     * @param EasyFrontPanelTRGOUTEnableMask::externalTrigger
     * External Trigger. When enabled, the external trigger on TRG-IN
     * can participate in the TRG-OUT (GPO) signal generation in logic
     * OR with the other enabled signals. Options are:\n
     * 0 = disabled\n
     * 1 = enabled.
     * @param EasyFrontPanelTRGOUTEnableMask::softwareTrigger
     * Software Trigger. When enabled, the software trigger can
     * participate in the TRG-OUT (GPO) signal generation in logic OR
     * with the other enabled signals. Options are:\n
     * 0 = disabled\n
     * 1 = enabled.
     */
    class EasyFrontPanelTRGOUTEnableMask : public EasyBase
    {
    protected:
        /* Shared base since one constructor cannot reuse the other */
        /*
         * EasyFrontPanelTRGOUTEnableMask fields:
         * group Trigger mask in [0:7], TRG-OUT generation logic in [8:9],
         * majority level in [10:12], LVDS trigger enable in [29],
         * external trigger in [30], software trigger in [31].
         */
        virtual void initLayout() override
        {
            layout = {
                {"groupTriggerMask", {(const uint8_t)8, (const uint8_t)0}},
                {"tRGOUTGenerationLogic", {(const uint8_t)2, (const uint8_t)8}},
                {"majorityLevel", {(const uint8_t)3, (const uint8_t)10}},
                {"lVDSTriggerEnable", {(const uint8_t)1, (const uint8_t)29}},
                {"externalTrigger", {(const uint8_t)1, (const uint8_t)30}},
                {"softwareTrigger", {(const uint8_t)1, (const uint8_t)31}}
            };
        }
        /* NOTE: use inherited generic constructFromMask(mask) */
        void construct(const uint8_t groupTriggerMask, const uint8_t tRGOUTGenerationLogic, const uint8_t majorityLevel, const uint8_t lVDSTriggerEnable, const uint8_t externalTrigger, const uint8_t softwareTrigger) {
            initLayout();
            variables = {
                {"groupTriggerMask", (const uint8_t)(groupTriggerMask & 0xFF)},
                {"tRGOUTGenerationLogic", (const uint8_t)(tRGOUTGenerationLogic & 0x3)},
                {"majorityLevel", (const uint8_t)(majorityLevel & 0x7)},
                {"lVDSTriggerEnable", (const uint8_t)(lVDSTriggerEnable & 0x1)},
                {"externalTrigger", (const uint8_t)(externalTrigger & 0x1)},
                {"softwareTrigger", (const uint8_t)(softwareTrigger & 0x1)}
            };
        };
    public:
        virtual const std::string getClassName() const override { return "EasyFrontPanelTRGOUTEnableMask"; };
        /* Construct using default values from docs */
        EasyFrontPanelTRGOUTEnableMask(const uint8_t groupTriggerMask, const uint8_t tRGOUTGenerationLogic, const uint8_t majorityLevel, const uint8_t lVDSTriggerEnable, const uint8_t externalTrigger, const uint8_t softwareTrigger)
        {
            construct(groupTriggerMask, tRGOUTGenerationLogic, majorityLevel, lVDSTriggerEnable, externalTrigger, softwareTrigger);
        }
        /* Construct from low-level bit mask in line with docs */
        EasyFrontPanelTRGOUTEnableMask(const uint32_t mask)
        {
            constructFromMask(mask);
        }
    }; // class EasyFrontPanelTRGOUTEnableMask


    /**
     * @class EasyDPPFrontPanelTRGOUTEnableMask
     * @brief For user-friendly configuration of Front Panel TRG-OUT Enable Mask.
     *
     * This register sets which signal can contribute to
     * generate the signal on the front panel TRG-OUT LEMO connector
     * (GPO in case of DT and NIM boards).
     *
     * @param EasyDPPFrontPanelTRGOUTEnableMask::lVDSTriggerEnable
     * LVDS Trigger Enable (VME boards only). If the LVDS I/Os are
     * programmed as outputs, they can participate in the TRG-OUT (GPO)
     * signal generation. They are in logic OR with the other enabled
     * signals. Options are:\n
     * 0 = disabled\n
     * 1 = enabled.
     * @param EasyDPPFrontPanelTRGOUTEnableMask::externalTrigger
     * External Trigger. When enabled, the external trigger on TRG-IN
     * can participate in the TRG-OUT (GPO) signal generation in logic
     * OR with the other enabled signals. Options are:\n
     * 0 = disabled\n
     * 1 = enabled.
     * @param EasyDPPFrontPanelTRGOUTEnableMask::softwareTrigger
     * Software Trigger. When enabled, the software trigger can
     * participate in the TRG-OUT (GPO) signal generation in logic OR
     * with the other enabled signals. Options are:\n
     * 0 = disabled\n
     * 1 = enabled.
     */
    class EasyDPPFrontPanelTRGOUTEnableMask : public EasyBase
    {
    protected:
        /* Shared base since one constructor cannot reuse the other */
        /*
         * EasyDPPFrontPanelTRGOUTEnableMask fields:
         * LVDS trigger enable in [29], external trigger in [30],
         * software trigger in [31].
         */
        virtual void initLayout() override
        {
            layout = {
                {"lVDSTriggerEnable", {(const uint8_t)1, (const uint8_t)29}},
                {"externalTrigger", {(const uint8_t)1, (const uint8_t)30}},
                {"softwareTrigger", {(const uint8_t)1, (const uint8_t)31}}
            };
        }
        /* NOTE: use inherited generic constructFromMask(mask) */
        void construct(const uint8_t lVDSTriggerEnable, const uint8_t externalTrigger, const uint8_t softwareTrigger) {
            initLayout();
            variables = {
                {"lVDSTriggerEnable", (const uint8_t)(lVDSTriggerEnable & 0x1)},
                {"externalTrigger", (const uint8_t)(externalTrigger & 0x1)},
                {"softwareTrigger", (const uint8_t)(softwareTrigger & 0x1)}
            };
        };
    public:
        virtual const std::string getClassName() const override { return "EasyDPPFrontPanelTRGOUTEnableMask"; };
        /* Construct using default values from docs */
        EasyDPPFrontPanelTRGOUTEnableMask(const uint8_t lVDSTriggerEnable, const uint8_t externalTrigger, const uint8_t softwareTrigger)
        {
            construct(lVDSTriggerEnable, externalTrigger, softwareTrigger);
        }
        /* Construct from low-level bit mask in line with docs */
        EasyDPPFrontPanelTRGOUTEnableMask(const uint32_t mask)
        {
            constructFromMask(mask);
        }
    }; // class EasyDPPFrontPanelTRGOUTEnableMask


    /**
     * @class EasyFrontPanelIOControl
     * @brief For user-friendly configuration of Acquisition Control mask.
     *
     * This register manages the front panel I/O connectors. Default
     * value is 0x000000.
     *
     * @param EasyFrontPanelIOControl::lEMOIOElectricalLevel
     * LEMO I/Os Electrical Level. This bit sets the electrical level of
     * the front panel LEMO connectors: TRG-IN, TRG-OUT (GPO in case of
     * DT and NIM boards), S-IN (GPI in case of DT and NIM
     * boards). Options are:\n
     * 0 = NIM I/O levels\n
     * 1 = TTL I/O levels.
     * @param EasyFrontPanelIOControl::tRGOUTEnable
     * TRG-OUT Enable (VME boards only). Enables the TRG-OUT LEMO front
     * panel connector. Options are:\n
     * 0 = enabled (default)\n
     * 1 = high impedance.\n
     * NOTE: this bit is reserved in case of DT and NIM boards.
     * @param EasyFrontPanelIOControl::lVDSIODirectionFirst
     * LVDS I/O [3:0] Direction (VME boards only). Sets the direction of
     * the signals on the first 4-pin group of the LVDS I/O
     * connector. Options are:\n
     * 0 = input\n
     * 1 = output.\n
     * NOTE: this bit is reserved in case of DT and NIM boards.
     * @param EasyFrontPanelIOControl::lVDSIODirectionSecond
     * LVDS I/O [7:4] Direction (VME boards only). Sets the direction of
     * the second 4-pin group of the LVDS I/O connector. Options are:\n
     * 0 = input\n
     * 1 = output.\n
     * NOTE: this bit is reserved in case of DT and NIM boards.
     * @param EasyFrontPanelIOControl::lVDSIODirectionThird
     * LVDS I/O [11:8] Direction (VME boards only). Sets the direction of
     * the third 4-pin group of the LVDS I/O connector. Options are:\n
     * 0 = input\n
     * 1 = output.\n
     * NOTE: this bit is reserved in case of DT and NIM boards.
     * @param EasyFrontPanelIOControl::lVDSIODirectionFourth
     * LVDS I/O [15:12] Direction (VME boards only). Sets the direction of
     * the fourth 4-pin group of the LVDS I/O connector. Options are:\n
     * 0 = input\n
     * 1 = output.\n
     * NOTE: this bit is reserved in case of DT and NIM boards.
     * @param EasyFrontPanelIOControl::lVDSIOSignalConfiguration
     * LVDS I/O Signal Configuration (VME boards and LVDS I/O old
     * features only). This configuration must be enabled through bit[8]
     * set to 0. Options are:\n
     * 00 = general purpose I/O\n
     * 01 = programmed I/O\n
     * 10 = pattern mode: LVDS signals are input and their value is
     * written into the header PATTERN field\n
     * 11 = reserved.\n
     * NOTE: these bits are reserved in case of DT and NIM boards.
     * @param EasyFrontPanelIOControl::lVDSIONewFeaturesSelection
     * LVDS I/O New Features Selection (VME boards only). Options are:\n
     * 0 = LVDS old features\n
     * 1 = LVDS new features.\n
     * The new features options can be configured through register
     * 0x81A0. Please, refer to the User Manual for all details.\n
     * NOTE: LVDS I/O New Features option is valid from motherboard
     * firmware revision 3.8 on.\n
     * NOTE: this bit is reserved in case of DT and NIM boards.
     * @param EasyFrontPanelIOControl::lVDSIOPatternLatchMode
     * LVDS I/Os Pattern Latch Mode (VME boards only). Options are:\n
     * 0 = Pattern (i.e. 16-pin LVDS status) is latched when the
     * (internal) global trigger is sent to channels, in consequence of
     * an external trigger. It accounts for post-trigger settings and
     * input latching delays\n
     * 1 = Pattern (i.e. 16-pin LVDS status) is latched when an external
     * trigger arrives.\n
     * NOTE: this bit is reserved in case of DT and NIM boards.
     * @param EasyFrontPanelIOControl::tRGINControl
     * TRG-IN control. The board trigger logic can be synchronized
     * either with the edge of the TRG-IN signal, or with its whole
     * duration.\n
     * Note: this bit must be used in conjunction with bit[11] =
     * 0. Options are:\n
     * 0 = trigger is synchronized with the edge of the TRG-IN signal\n
     * 1 = trigger is synchronized with the whole duration of the TRG-IN
     * signal.
     * @param EasyFrontPanelIOControl::tRGINMezzanines
     * TRG-IN to Mezzanines (channels). Options are:\n
     * 0 = TRG-IN signal is processed by the motherboard and sent to
     * mezzanine (default). The trigger logic is then synchronized with
     * TRG-IN\n
     * 1 = TRG-IN is directly sent to the mezzanines with no mother
     * board processing nor delay. This option can be useful when TRG-IN
     * is used to veto the acquisition.\n
     * NOTE: if this bit is set to 1, then bit[10] is ignored.
     * @param EasyFrontPanelIOControl::forceTRGOUT
     * Force TRG-OUT (GPO). This bit can force TRG-OUT (GPO in case of
     * DT and NIM boards) test logical level if bit[15] = 1. Options
     * are:\n
     * 0 = Force TRG-OUT (GPO) to 0\n
     * 1 = Force TRG-OUT (GPO) to 1.
     * @param EasyFrontPanelIOControl::tRGOUTMode
     * TRG-OUT (GPO) Mode. Options are:\n
     * 0 = TRG-OUT (GPO) is an internal signal (according to bits[17:16])\n
     * 1= TRG-OUT (GPO) is a test logic level set via bit[14].
     * @param EasyFrontPanelIOControl::tRGOUTModeSelection
     * TRG-OUT (GPO) Mode Selection. Options are:\n
     * 00 = Trigger: TRG-OUT/GPO propagates the internal trigger sources
     * according to register 0x8110\n
     * 01 = Motherboard Probes: TRG-OUT/GPO is used to propagate signals
     * of the motherboards according to bits[19:18]\n
     * 10 = Channel Probes: TRG-OUT/GPO is used to propagate signals of
     * the mezzanines (Channel Signal Virtual Probe)\n
     * 11 = S-IN (GPI) propagation.
     * @param EasyFrontPanelIOControl::motherboardVirtualProbeSelection
     * Motherboard Virtual Probe Selection (to be propagated on TRG-
     * OUT/GPO). Options are:\n
     * 00 = RUN/delayedRUN: this is the RUN in case of ROC FPGA firmware
     * rel. less than 4.12. This probe can be selected according to
     * bit[20].\n
     * 01 = CLKOUT: this clock is synchronous with the sampling clock of
     * the ADC and this option can be used to align the phase of the
     * clocks in different boards\n
     * 10 = CLK Phase\n
     * 11 = BUSY/UNLOCK: this is the board BUSY in case of ROC FPGA
     * firmware rel. 4.5 or lower. This probe can be selected according
     * to bit[20].
     * @param EasyFrontPanelIOControl::motherboardVirtualProbePropagation
     * According to bits[19:18], this bit selects the probe to be
     * propagated on TRG-OUT . If bits[19:18] = 00, then bit[20] options
     * are:\n
     * 0 = RUN, the signal is active when the acquisition is running and
     * it is synchonized with the start run. This option must be used to
     * synchronize the start/stop of the acquisition through the
     * TRG-OUT->TR-IN or TRG-OUT->S-IN (GPI) daisy chain.\n
     * 1 = delayedRUN. This option can be used to debug the
     * synchronization when the start/stop is propagated through the
     * LVDS I/O (VME boards). If bits[19:18] = 11, then bit[20] options
     * are:\n
     * 0 = Board BUSY\n
     * 1 = PLL Lock Loss.\n
     * NOTE: this bit is reserved in case of ROC FPGA firmware rel. 4.5
     * or lower.\n
     * NOTE: this bit corresponds to BUSY/UNLOCK for ROC FPGA firmware
     * rel. less than 4.12.
     * @param EasyFrontPanelIOControl::patternConfiguration
     * Pattern Configuration. Configures the information given by the
     * 16-bit PATTERN field in the header of the event format (VME
     * only). Option are:\n
     * 00 = PATTERN: 16-bit pattern latched on the 16 LVDS signals as
     * one trigger arrives (default)\n
     * Other options are reserved.
     */
    class EasyFrontPanelIOControl : public EasyBase
    {
    protected:
        /* Shared base since one constructor cannot reuse the other */
        /*
         * EasyFrontPanelIOControl fields:
         * LEMO I/O electrical level [0], TRG-OUT enable [1],
         * LVDS I/O 1st Direction in [2], LVDS I/O 2nd Direction in [3],
         * LVDS I/O 3rd Direction in [4], LVDS I/O 4th Direction in [5],
         * LVDS I/O signal configuration [6:7],
         * LVDS I/O new features selection in [8],
         * LVDS I/Os pattern latch mode in [9],
         * TRG-IN control in [10], TRG-IN to mezzanines in [11],
         * force TRG-OUT in [14], TRG-OUT mode in [15],
         * TRG-OUT mode selection in [16:17],
         * motherboard virtual probe selection in [18:19],
         * motherboard virtual probe propagation in [20],
         * pattern configuration in [21:22]
         */
        virtual void initLayout() override
        {
            layout = {{"lEMOIOElectricalLevel", {(const uint8_t)1, (const uint8_t)0}},
                      {"tRGOUTEnable", {(const uint8_t)1, (const uint8_t)1}},
                      {"lVDSIODirectionFirst", {(const uint8_t)1, (const uint8_t)2}},
                      {"lVDSIODirectionSecond", {(const uint8_t)1, (const uint8_t)3}},
                      {"lVDSIODirectionThird", {(const uint8_t)1, (const uint8_t)4}},
                      {"lVDSIODirectionFourth", {(const uint8_t)1, (const uint8_t)5}},
                      {"lVDSIOSignalConfiguration", {(const uint8_t)2, (const uint8_t)6}},
                      {"lVDSIONewFeaturesSelection", {(const uint8_t)1, (const uint8_t)8}},
                      {"lVDSIOPatternLatchMode", {(const uint8_t)1, (const uint8_t)9}},
                      {"tRGINControl", {(const uint8_t)1, (const uint8_t)10}},
                      {"tRGINMezzanines", {(const uint8_t)1, (const uint8_t)11}},
                      {"forceTRGOUT", {(const uint8_t)1, (const uint8_t)14}},
                      {"tRGOUTMode", {(const uint8_t)1, (const uint8_t)15}},
                      {"tRGOUTModeSelection", {(const uint8_t)2, (const uint8_t)16}},
                      {"motherboardVirtualProbeSelection", {(const uint8_t)2, (const uint8_t)18}},
                      {"motherboardVirtualProbePropagation", {(const uint8_t)1, (const uint8_t)20}},
                      {"patternConfiguration", {(const uint8_t)2, (const uint8_t)21}}
            };
        }
        /* NOTE: use inherited generic constructFromMask(mask) */
        void construct(const uint8_t lEMOIOElectricalLevel, const uint8_t tRGOUTEnable, const uint8_t lVDSIODirectionFirst, const uint8_t lVDSIODirectionSecond, const uint8_t lVDSIODirectionThird, const uint8_t lVDSIODirectionFourth, const uint8_t lVDSIOSignalConfiguration, const uint8_t lVDSIONewFeaturesSelection, const uint8_t lVDSIOPatternLatchMode, const uint8_t tRGINControl, const uint8_t tRGINMezzanines, const uint8_t forceTRGOUT, const uint8_t tRGOUTMode, const uint8_t tRGOUTModeSelection, const uint8_t motherboardVirtualProbeSelection, const uint8_t motherboardVirtualProbePropagation, const uint8_t patternConfiguration) {
            initLayout();
            variables = {
                {"lEMOIOElectricalLevel", (const uint8_t)(lEMOIOElectricalLevel & 0x1)},
                {"tRGOUTEnable", (const uint8_t)(tRGOUTEnable & 0x1)},
                {"lVDSIODirectionFirst", (const uint8_t)(lVDSIODirectionFirst & 0x1)},
                {"lVDSIODirectionSecond", (const uint8_t)(lVDSIODirectionSecond & 0x1)},
                {"lVDSIODirectionThird", (const uint8_t)(lVDSIODirectionThird & 0x1)},
                {"lVDSIODirectionFourth", (const uint8_t)(lVDSIODirectionFourth & 0x1)},
                {"lVDSIOSignalConfiguration", (const uint8_t)(lVDSIOSignalConfiguration & 0x3)},
                {"lVDSIONewFeaturesSelection", (const uint8_t)(lVDSIONewFeaturesSelection & 0x1)},
                {"lVDSIOPatternLatchMode", (const uint8_t)(lVDSIOPatternLatchMode & 0x1)},
                {"tRGINControl", (const uint8_t)(tRGINControl & 0x1)},
                {"tRGINMezzanines", (const uint8_t)(tRGINMezzanines & 0x1)},
                {"forceTRGOUT", (const uint8_t)(forceTRGOUT & 0x1)},
                {"tRGOUTMode", (const uint8_t)(tRGOUTMode & 0x1)},
                {"tRGOUTModeSelection", (const uint8_t)(tRGOUTModeSelection & 0x3)},
                {"motherboardVirtualProbeSelection", (const uint8_t)(motherboardVirtualProbeSelection & 0x3)},
                {"motherboardVirtualProbePropagation", (const uint8_t)(motherboardVirtualProbePropagation & 0x1)},
                {"patternConfiguration", (const uint8_t)(patternConfiguration & 0x3)}
            };
        };
    public:
        virtual const std::string getClassName() const override { return "EasyFrontPanelIOControl"; };
        /* Construct using default values from docs */
        EasyFrontPanelIOControl(const uint8_t lEMOIOElectricalLevel, const uint8_t tRGOUTEnable, const uint8_t lVDSIODirectionFirst, const uint8_t lVDSIODirectionSecond, const uint8_t lVDSIODirectionThird, const uint8_t lVDSIODirectionFourth, const uint8_t lVDSIOSignalConfiguration, const uint8_t lVDSIONewFeaturesSelection, const uint8_t lVDSIOPatternLatchMode, const uint8_t tRGINControl, const uint8_t tRGINMezzanines, const uint8_t forceTRGOUT, const uint8_t tRGOUTMode, const uint8_t tRGOUTModeSelection, const uint8_t motherboardVirtualProbeSelection, const uint8_t motherboardVirtualProbePropagation, const uint8_t patternConfiguration)
        {
            construct(lEMOIOElectricalLevel, tRGOUTEnable, lVDSIODirectionFirst, lVDSIODirectionSecond, lVDSIODirectionThird, lVDSIODirectionFourth, lVDSIOSignalConfiguration, lVDSIONewFeaturesSelection, lVDSIOPatternLatchMode, tRGINControl, tRGINMezzanines, forceTRGOUT, tRGOUTMode, tRGOUTModeSelection, motherboardVirtualProbeSelection, motherboardVirtualProbePropagation, patternConfiguration);
        }
        /* Construct from low-level bit mask in line with docs */
        EasyFrontPanelIOControl(const uint32_t mask)
        {
            constructFromMask(mask);
        }
    }; // class EasyFrontPanelIOControl


    /**
     * @class EasyDPPFrontPanelIOControl
     * @brief For user-friendly configuration of Acquisition Control mask.
     *
     * NOTE: Identical to EasyFrontPanelIOControl
     */
    class EasyDPPFrontPanelIOControl : public EasyFrontPanelIOControl
    {
    /* Just inherit everything else from parent class */
    public:
        virtual const std::string getClassName() const override { return "EasyDPPFrontPanelIOControl"; };
        EasyDPPFrontPanelIOControl(const uint8_t lEMOIOElectricalLevel, const uint8_t tRGOUTEnable, const uint8_t lVDSIODirectionFirst, const uint8_t lVDSIODirectionSecond, const uint8_t lVDSIODirectionThird, const uint8_t lVDSIODirectionFourth, const uint8_t lVDSIOSignalConfiguration, const uint8_t lVDSIONewFeaturesSelection, const uint8_t lVDSIOPatternLatchMode, const uint8_t tRGINControl, const uint8_t tRGINMezzanines, const uint8_t forceTRGOUT, const uint8_t tRGOUTMode, const uint8_t tRGOUTModeSelection, const uint8_t motherboardVirtualProbeSelection, const uint8_t motherboardVirtualProbePropagation, const uint8_t patternConfiguration) : EasyFrontPanelIOControl(lEMOIOElectricalLevel, tRGOUTEnable, lVDSIODirectionFirst, lVDSIODirectionSecond, lVDSIODirectionThird, lVDSIODirectionFourth, lVDSIOSignalConfiguration, lVDSIONewFeaturesSelection, lVDSIOPatternLatchMode, tRGINControl, tRGINMezzanines, forceTRGOUT, tRGOUTMode, tRGOUTModeSelection, motherboardVirtualProbeSelection, motherboardVirtualProbePropagation, patternConfiguration) {};
        EasyDPPFrontPanelIOControl(uint32_t mask) : EasyFrontPanelIOControl(mask) {};
    }; // class EasyDPPFrontPanelIOControl


    /**
     * @class EasyROCFPGAFirmwareRevision
     * @brief For user-friendly configuration of ROC FPGA Firmware Revision.
     *
     * This register contains the motherboard FPGA (ROC) firmware
     * revision information.\n
     * The complete format is:\n
     * Firmware Revision = X.Y (16 lower bits)\n
     * Firmware Revision Date = Y/M/DD (16 higher bits)\n
     * EXAMPLE 1: revision 3.08, November 12th, 2007 is 0x7B120308.\n
     * EXAMPLE 2: revision 4.09, March 7th, 2016 is 0x03070409.\n
     * NOTE: the nibble code for the year makes this information to roll
     * over each 16 years.
     *
     * @param EasyROCFPGAFirmwareRevision::minorRevisionNumber
     * ROC Firmware Minor Revision Number (Y).
     * @param EasyROCFPGAFirmwareRevision::majorRevisionNumber
     * ROC Firmware Major Revision Number (X).
     * @param EasyROCFPGAFirmwareRevision::revisionDate
     * ROC Firmware Revision Date (Y/M/DD).
     */
    class EasyROCFPGAFirmwareRevision : public EasyBase
    {
    protected:
        /* Shared base since one constructor cannot reuse the other */
        /*
         * EasyROCFPGAFirmwareRevision fields:
         * minor revision number in [0:7], major revision number in [8:15],
         * revision date in [16:31].
         */
        /* NOTE: we split revision date into the four digits internally
         since it is just four 4-bit values clamped into 16-bit anyway.
         This makes the generic and DPP version much more similar, too. */
        void initLayout()
        {
            layout = {
                {"minorRevisionNumber", {(const uint8_t)8, (const uint8_t)0}},
                {"majorRevisionNumber", {(const uint8_t)8, (const uint8_t)8}},
                {"revisionDayLower", {(const uint8_t)4, (const uint8_t)16}},
                {"revisionDayUpper", {(const uint8_t)4, (const uint8_t)20}},
                {"revisionMonth", {(const uint8_t)4, (const uint8_t)24}},
                {"revisionYear", {(const uint8_t)4, (const uint8_t)28}}
            };
        }
        void construct(const uint8_t minorRevisionNumber, const uint8_t majorRevisionNumber, const uint8_t revisionDayLower, const uint8_t revisionDayUpper, const uint8_t revisionMonth, const uint8_t revisionYear)
        {
            initLayout();
            variables = {
                {"minorRevisionNumber", (const uint8_t)minorRevisionNumber},
                {"majorRevisionNumber", (const uint8_t)majorRevisionNumber},
                {"revisionDayLower", (const uint8_t)(revisionDayLower & 0x7)},
                {"revisionDayUpper", (const uint8_t)(revisionDayUpper & 0x7)},
                {"revisionMonth", (const uint8_t)(revisionMonth & 0x7)},
                {"revisionYear", (const uint8_t)(revisionYear & 0x7)}
            };
        }
    public:
        virtual const std::string getClassName() const override { return "EasyROCFPGAFirmwareRevision"; };
        /* Construct using default values from docs */
        /* We allow both clamped and individual revision date format here */
        EasyROCFPGAFirmwareRevision(const uint8_t minorRevisionNumber, const uint8_t majorRevisionNumber, const uint16_t revisionDate)
        {
            const uint8_t revisionDayLower = (const uint8_t)(revisionDate & 0x7);
            const uint8_t revisionDayUpper = (const uint8_t)((revisionDate >> 4) & 0x7);
            const uint8_t revisionMonth = (const uint8_t)((revisionDate >> 8) & 0x7);
            const uint8_t revisionYear = (const uint8_t)((revisionDate >> 12) & 0x7);
            construct(minorRevisionNumber, majorRevisionNumber, revisionDayLower, revisionDayUpper, revisionMonth, revisionYear);
        }
        EasyROCFPGAFirmwareRevision(const uint8_t minorRevisionNumber, const uint8_t majorRevisionNumber, const uint8_t revisionDayLower, const uint8_t revisionDayUpper, const uint8_t revisionMonth, const uint8_t revisionYear)
        {
            construct(minorRevisionNumber, majorRevisionNumber, revisionDayLower, revisionDayUpper, revisionMonth, revisionYear);
        }
        /* Construct from low-level bit mask in line with docs */
        EasyROCFPGAFirmwareRevision(const uint32_t mask)
        {
            constructFromMask(mask);
        }
    }; // class EasyROCFPGAFirmwareRevision


    /**
     * @class EasyDPPROCFPGAFirmwareRevision
     * @brief For user-friendly configuration of ROC FPGA Firmware Revision.
     *
     * NOTE: identical to EasyROCFPGAFirmwareRevision
     */
    class EasyDPPROCFPGAFirmwareRevision : public EasyBase
    {
    protected:
        /* Shared base since one constructor cannot reuse the other */
        /*
         * EasyDPPROCFPGAFirmwareRevision fields:
         * firmware revision number in [0:7], firmware DPP code in [8:15],
         * build day lower in [16:19], build day upper in [20:23],
         * build month in [24:27], build year in [28:31]
         */
        void initLayout()
        {
            layout = {
                {"firmwareRevisionNumber", {(const uint8_t)8, (const uint8_t)0}},
                {"firmwareDPPCode", {(const uint8_t)8, (const uint8_t)8}},
                {"buildDayLower", {(const uint8_t)4, (const uint8_t)16}},
                {"buildDayUpper", {(const uint8_t)4, (const uint8_t)20}},
                {"buildMonth", {(const uint8_t)4, (const uint8_t)24}},
                {"buildYear", {(const uint8_t)4, (const uint8_t)28}}
            };
        }
        /* NOTE: use inherited generic constructFromMask(mask) */
        void construct(const uint8_t firmwareRevisionNumber, const uint8_t firmwareDPPCode, const uint8_t buildDayLower, const uint8_t buildDayUpper, const uint8_t buildMonth, const uint8_t buildYear)
        {
            initLayout();
            variables = {
                {"firmwareRevisionNumber", (const uint8_t)firmwareRevisionNumber},
                {"firmwareDPPCode", (const uint8_t)firmwareDPPCode},
                {"buildDayLower", (const uint8_t)(buildDayLower & 0xF)},
                {"buildDayUpper", (const uint8_t)(buildDayUpper & 0xF)},
                {"buildMonth", (const uint8_t)(buildMonth & 0xF)},
                {"buildYear", (const uint8_t)(buildYear & 0xF)}
            };
        }
    public:
        virtual const std::string getClassName() const override { return "EasyDPPROCFPGAFirmwareRevision"; };
        /* Construct using default values from docs */
        /* We allow both clamped and individual revision date format here */
        EasyDPPROCFPGAFirmwareRevision(const uint8_t minorRevisionNumber, const uint8_t majorRevisionNumber, const uint16_t revisionDate)
        {
            const uint8_t revisionDayLower = (const uint8_t)(revisionDate & 0x7);
            const uint8_t revisionDayUpper = (const uint8_t)((revisionDate >> 4) & 0x7);
            const uint8_t revisionMonth = (const uint8_t)((revisionDate >> 8) & 0x7);
            const uint8_t revisionYear = (const uint8_t)((revisionDate >> 12) & 0x7);
            construct(minorRevisionNumber, majorRevisionNumber, revisionDayLower, revisionDayUpper, revisionMonth, revisionYear);
        }
        EasyDPPROCFPGAFirmwareRevision(const uint8_t firmwareRevisionNumber, const uint8_t firmwareDPPCode, const uint8_t buildDayLower, const uint8_t buildDayUpper, const uint8_t buildMonth, const uint8_t buildYear)
        {
            construct(firmwareRevisionNumber, firmwareDPPCode, buildDayLower, buildDayUpper, buildMonth, buildYear);
        }
        /* Construct from low-level bit mask in line with docs */
        EasyDPPROCFPGAFirmwareRevision(const uint32_t mask)
        {
            constructFromMask(mask);
        }
    }; // class EasyDPPROCFPGAFirmwareRevision


    /**
     * @class EasyFanSpeedControl
     * @brief For user-friendly configuration of Fan Speed Control mask.
     *
     * This register manages the on-board fan speed in order to
     * guarantee an appropriate cooling according to the internal
     * temperature variations.\n
     * NOTE: from revision 4 of the motherboard PCB (see register 0xF04C
     * of the Configuration ROM), the automatic fan speed control has
     * been implemented, and it is supported by ROC FPGA firmware
     * revision greater than 4.4 (see register 0x8124).\n
     * Independently of the revision, the user can set the fan speed
     * high by setting bit[3] = 1. Setting bit[3] = 0 will restore the
     * automatic control for revision 4 or higher, or the low fan speed
     * in case of revisions lower than 4.\n
     * NOTE: this register is supported by Desktop (DT) boards only.
     *
     * @param EasyFanSpeedControl::fanSpeedMode
     * Fan Speed Mode. Options are:\n
     * 0 = slow speed or automatic speed tuning\n
     * 1 = high speed.
     */
    class EasyFanSpeedControl : public EasyBase
    {
    protected:
        /* Shared helpers since one constructor cannot reuse the other */
        /*
         * EasyFanSpeedControl fields:
         * fan Speed mode in [3], (reserved forced 1) in [4:5].
         */
        void initLayout() override
        {
            layout = {
                {"fanSpeedMode", {(const uint8_t)1, (const uint8_t)3}},
                {"__reserved__0_", {(const uint8_t)2, (const uint8_t)4}}
            };
        }
        /* NOTE: use inherited generic constructFromMask(mask) */
        void construct(const uint8_t fanSpeedMode) {
            initLayout();
            variables = {
                {"fanSpeedMode", (const uint8_t)(fanSpeedMode & 0x1)},
                {"__reserved__0_", (const uint8_t)(0x1)}
            };
        };
    public:
        virtual const std::string getClassName() const override { return "EasyFanSpeedControl"; };
        /* Construct using default values from docs */
        EasyFanSpeedControl(const uint8_t fanSpeedMode=0)
        {
            construct(fanSpeedMode);
        }
        /* Construct from low-level bit mask in line with docs */
        EasyFanSpeedControl(const uint32_t mask)
        {
            constructFromMask(mask);
        }
    }; // class EasyFanSpeedControl


    /**
     * @class EasyDPPFanSpeedControl
     * @brief For user-friendly configuration of Fan Speed Control mask.
     *
     * NOTE: identical to EasyFanSpeedControl
     */
    class EasyDPPFanSpeedControl : public EasyFanSpeedControl
    {
    /* Just inherit everything else from parent class */
    public:
        virtual const std::string getClassName() const override { return "EasyDPPFanSpeedControl"; };
        EasyDPPFanSpeedControl(uint8_t fanSpeedMode) : EasyFanSpeedControl(fanSpeedMode) {};
        EasyDPPFanSpeedControl(uint32_t mask) : EasyFanSpeedControl(mask) {};
    }; // class EasyDPPFanSpeedControl


    /**
     * @class EasyReadoutControl
     * @brief For user-friendly configuration of Readout Control mask.
     *
     * This register is mainly intended for VME boards, anyway some bits
     * are applicable also for DT and NIM boards.
     *
     * @param EasyReadoutControl::vMEInterruptLevel
     * VME Interrupt Level (VME boards only). Options are:\n
     * 0 = VME interrupts are disabled\n
     * 1,..,7 = sets the VME interrupt level.\n
     * NOTE: these bits are reserved in case of DT and NIM boards.
     * @param EasyReadoutControl::opticalLinkInterruptEnable
     * Optical Link Interrupt Enable. Op ons are:\n
     * 0 = Optical Link interrupts are disabled\n
     * 1 = Optical Link interrupts are enabled.
     * @param EasyReadoutControl::vMEBusErrorEventAlignedEnable
     * VME Bus Error / Event Aligned Readout Enable (VME boards
     * only). Options are:\n
     * 0 = VME Bus Error / Event Aligned Readout disabled (the module
     * sends a DTACK signal until the CPU inquires the module)\n
     * 1 = VME Bus Error / Event Aligned Readout enabled (the module is
     * enabled either to generate a Bus Error to finish a block transfer
     * or during the empty buffer readout in D32).\n
     * NOTE: this bit is reserved (must be 1) in case of DT and NIM boards.
     * @param EasyReadoutControl::vMEAlign64Mode
     * VME Align64 Mode (VME boards only). Options are:\n
     * 0 = 64-bit aligned readout mode disabled\n
     * 1 = 64-bit aligned readout mode enabled.\n
     * NOTE: this bit is reserved (must be 0) in case of DT and NIM boards.
     * @param EasyReadoutControl::vMEBaseAddressRelocation
     * VME Base Address Relocation (VME boards only). Options are:\n
     * 0 = Address Relocation disabled (VME Base Address is set by the on-board
     * rotary switches)\n
     * 1 = Address Relocation enabled (VME Base Address is set by
     * register 0xEF0C).\n
     * NOTE: this bit is reserved (must be 0) in case of DT and NIM boards.
     * @param EasyReadoutControl::interruptReleaseMode
     * Interrupt Release mode (VME boards only). Options are:\n
     * 0 = Release On Register Access (RORA): this is the default mode,
     * where interrupts are removed by disabling them either by setting
     * VME Interrupt Level to 0 (VME Interrupts) or by setting Optical
     * Link Interrupt Enable to 0\n
     * 1 = Release On Acknowledge (ROAK). Interrupts are automatically
     * disabled at the end of a VME interrupt acknowledge cycle (INTACK
     * cycle).\n
     * NOTE: ROAK mode is supported only for VME interrupts. ROAK mode
     * is not supported on interrupts generated over Optical Link.
     * NOTE: this bit is reserved (must be 0) in case of DT and NIM boards.
     * @param EasyReadoutControl::extendedBlockTransferEnable
     * Extended Block Transfer Enable (VME boarsd only). Selects the
     * memory interval allocated for block transfers. Options are:\n
     * 0 = Extended Block Transfer Space is disabled, and the block
     * transfer region is a 4kB in the 0x0000 - 0x0FFC interval\n
     * 1 = Extended Block Transfer Space is enabled, and the block
     * transfer is a 16 MB in the 0x00000000 - 0xFFFFFFFC interval.\n
     * NOTE: in Extended mode, the board VME Base Address is only set
     * via the on-board [31:28] rotary switches or bits[31:28] of
     * register 0xEF10.\n
     * NOTE: this register is reserved in case of DT and NIM boards.
     */
    class EasyReadoutControl : public EasyBase
    {
    protected:
        /* Shared helpers since one constructor cannot reuse the other */
        /*
         * EasyReadoutControl fields:
         * VME interrupt level in [0:2], optical link interrupt enable in [3],
         * VME bus error / event aligned readout enable in [4],
         * VME align64 mode in [5], VME base address relocation in [6],
         * Interrupt release mode in [7], 
         * extended block transfer enable in [8].
         */
        void initLayout() override
        {
            layout = {
                {"vMEInterruptLevel", {(const uint8_t)3, (const uint8_t)0}},
                {"opticalLinkInterruptEnable", {(const uint8_t)1, (const uint8_t)3}},
                {"vMEBusErrorEventAlignedEnable", {(const uint8_t)1, (const uint8_t)4}},
                {"vMEAlign64Mode", {(const uint8_t)1, (const uint8_t)5}},
                {"vMEBaseAddressRelocation", {(const uint8_t)1, (const uint8_t)6}},
                {"interruptReleaseMode", {(const uint8_t)1, (const uint8_t)7}},
                {"extendedBlockTransferEnable", {(const uint8_t)1, (const uint8_t)8}}
            };
        }
        /* NOTE: use inherited generic constructFromMask(mask) */
        void construct(const uint8_t vMEInterruptLevel, const uint8_t opticalLinkInterruptEnable, const uint8_t vMEBusErrorEventAlignedEnable, const uint8_t vMEAlign64Mode, const uint8_t vMEBaseAddressRelocation, const uint8_t interruptReleaseMode, const uint8_t extendedBlockTransferEnable) {
            initLayout();
            variables = {
                {"vMEInterruptLevel", (const uint8_t)(vMEInterruptLevel & 0x7)},
                {"opticalLinkInterruptEnable", (const uint8_t)(opticalLinkInterruptEnable & 0x1)},
                {"vMEBusErrorEventAlignedEnable", (const uint8_t)(vMEBusErrorEventAlignedEnable & 0x1)},
                {"vMEAlign64Mode", (const uint8_t)(vMEAlign64Mode & 0x1)},
                {"vMEBaseAddressRelocation", (const uint8_t)(vMEBaseAddressRelocation & 0x1)},
                {"interruptReleaseMode", (const uint8_t)(interruptReleaseMode & 0x1)},
                {"extendedBlockTransferEnable", (const uint8_t)(extendedBlockTransferEnable & 0x1)}
            };
        };
    public:
        virtual const std::string getClassName() const override { return "EasyReadoutControl"; };
        /* Construct using default values from docs */
        EasyReadoutControl(const uint8_t vMEInterruptLevel, const uint8_t opticalLinkInterruptEnable, const uint8_t vMEBusErrorEventAlignedEnable, const uint8_t vMEAlign64Mode, const uint8_t vMEBaseAddressRelocation, const uint8_t interruptReleaseMode, const uint8_t extendedBlockTransferEnable)
        {
            construct(vMEInterruptLevel, opticalLinkInterruptEnable, vMEBusErrorEventAlignedEnable, vMEAlign64Mode, vMEBaseAddressRelocation, interruptReleaseMode, extendedBlockTransferEnable);
        }
        /* Construct from low-level bit mask in line with docs */
        EasyReadoutControl(const uint32_t mask)
        {
            constructFromMask(mask);
        }
    }; // class EasyReadoutControl


    /**
     * @class EasyDPPReadoutControl
     * @brief For user-friendly configuration of Readout Control mask.
     *
     * NOTE: identical to EasyReadoutControl
     */
    class EasyDPPReadoutControl : public EasyReadoutControl
    {
    /* Just inherit everything else from parent class */
    public:
        virtual const std::string getClassName() const override { return "EasyDPPReadoutControl"; };
        EasyDPPReadoutControl(const uint8_t vMEInterruptLevel, const uint8_t opticalLinkInterruptEnable, const uint8_t vMEBusErrorEventAlignedEnable, const uint8_t vMEAlign64Mode, const uint8_t vMEBaseAddressRelocation, const uint8_t interruptReleaseMode, const uint8_t extendedBlockTransferEnable) : EasyReadoutControl(vMEInterruptLevel, opticalLinkInterruptEnable, vMEBusErrorEventAlignedEnable, vMEAlign64Mode, vMEBaseAddressRelocation, interruptReleaseMode, extendedBlockTransferEnable){};
        EasyDPPReadoutControl(const uint32_t mask) : EasyReadoutControl(mask) {};
    }; // class EasyDPPReadoutControl


    /**
     * @class EasyReadoutStatus
     * @brief For user-friendly configuration of Readout Status mask.
     *
     * This register contains informa on related to the readout.
     *
     * @param EasyReadoutStatus::eventReady
     * Event Ready. Indicates if there are events stored ready for
     * readout. Options are:\n
     * 0 = no data ready\n
     * 1 = event ready.
     * @param EasyReadoutStatus::outputBufferStatus
     * Output Buffer Status. Indicates if the Output Buffer is in Full
     * condition. Options are:\n
     * 0 = the Output Buffer is not FULL\n
     * 1 = the Output Buffer is FULL.
     * @param EasyReadoutStatus::busErrorSlaveTerminated
     * Bus Error (VME boards) / Slave-Terminated (DT/NIM boards) Flag.
     * Options are:\n
     * 0 = no Bus Error occurred (VME boards) or no terminated transfer
     * (DT/NIM boards)\n
     * 1 = a Bus Error occurred (VME boards) or one transfer has been
     * terminated by the digitizer in consequence of an unsupported
     * register access or block transfer prematurely terminated in event
     * aligned readout (DT/NIM).\n
     * NOTE: this bit is reset a er register readout at 0xEF04.
     */
    class EasyReadoutStatus : public EasyBase
    {
    protected:
        /* Shared helpers since one constructor cannot reuse the other */
        /*
         * EasyReadoutStatus fields:
         * event ready [0], output buffer status in [1],
         * bus error / slave terminated in [2].
         */
        void initLayout() override
        {
            layout = {
                {"eventReady", {(const uint8_t)1, (const uint8_t)0}},
                {"outputBufferStatus", {(const uint8_t)1, (const uint8_t)1}},
                {"busErrorSlaveTerminated", {(const uint8_t)1, (const uint8_t)2}},
            };
        }
        /* NOTE: use inherited generic constructFromMask(mask) */
        void construct(const uint8_t eventReady, const uint8_t outputBufferStatus, const uint8_t busErrorSlaveTerminated) {
            initLayout();
            variables = {
                {"eventReady", (const uint8_t)(eventReady & 0x1)},
                {"outputBufferStatus", (const uint8_t)(outputBufferStatus & 0x1)},
                {"busErrorSlaveTerminated", (const uint8_t)(busErrorSlaveTerminated & 0x1)},
            };
        };
    public:
        virtual const std::string getClassName() const override { return "EasyReadoutStatus"; };
        /* Construct using default values from docs */
        EasyReadoutStatus(const uint8_t eventReady, const uint8_t outputBufferStatus, const uint8_t busErrorSlaveTerminated)
        {
            construct(eventReady, outputBufferStatus, busErrorSlaveTerminated);
        }
        /* Construct from low-level bit mask in line with docs */
        EasyReadoutStatus(const uint32_t mask)
        {
            constructFromMask(mask);
        }
    }; // class EasyReadoutStatus


    /**
     * @class EasyDPPReadoutStatus
     * @brief For user-friendly configuration of Readout Status mask.
     *
     * NOTE: identical to EasyReadoutStatus
     */
    class EasyDPPReadoutStatus : public EasyReadoutStatus
    {
    /* Just inherit everything else from parent class */
    public:
        virtual const std::string getClassName() const override { return "EasyDPPReadoutStatus"; };
        /* Construct using default values from docs */
        EasyDPPReadoutStatus(const uint8_t eventReady, const uint8_t outputBufferStatus, const uint8_t busErrorSlaveTerminated) : EasyReadoutStatus(eventReady, outputBufferStatus, busErrorSlaveTerminated) {};
        EasyDPPReadoutStatus(const uint32_t mask) : EasyReadoutStatus(mask) {};
    }; // class EasyDPPReadoutStatus


    /**
     * @class EasyScratch
     * @brief For user-friendly configuration of Scratch mask.
     *
     * This register is used for dummy read and write testing, fields
     * are arbitrarily chosen.
     *
     * @param EasyScratch::dummy1
     * A 1-bit test.
     * @param EasyScratch::dummy2
     * A 1-bit test.
     * @param EasyScratch::dummy3
     * A 1-bit test.
     * @param EasyScratch::dummy4
     * A 3-bit test.
     * @param EasyScratch::dummy5
     * A 5-bit test.
     * @param EasyScratch::dummy6
     * A 6-bit test.
     * @param EasyScratch::dummy7
     * A 7-bit test.
     * @param EasyScratch::dummy8
     * A 8-bit test.
     */
    class EasyScratch : public EasyBase
    {
    protected:
        /* Shared helpers since one constructor cannot reuse the other */
        /*
         * EasyScratch fields:
         * dummy1 in [0], dummy2 in [1], dummy3 in [2], dummy4 in [3:5],
         * dummy5 in [6:10], dummy6 in [11:16], dummy8 in [17:23],
         * dummy8 in [24:31].
         */
        void initLayout() override
        {
            layout = {
                {"dummy1", {(const uint8_t)1, (const uint8_t)0}},
                {"dummy2", {(const uint8_t)1, (const uint8_t)1}},
                {"dummy3", {(const uint8_t)1, (const uint8_t)2}},
                {"dummy4", {(const uint8_t)3, (const uint8_t)3}},
                {"dummy5", {(const uint8_t)5, (const uint8_t)6}},
                {"dummy6", {(const uint8_t)6, (const uint8_t)11}},
                {"dummy7", {(const uint8_t)7, (const uint8_t)17}},
                {"dummy8", {(const uint8_t)8, (const uint8_t)24}}
            };
        }
        /* NOTE: use inherited generic constructFromMask(mask) */
        void construct(const uint8_t dummy1, const uint8_t dummy2, const uint8_t dummy3, const uint8_t dummy4, const uint8_t dummy5, const uint8_t dummy6, const uint8_t dummy7, const uint8_t dummy8) {
            initLayout();
            variables = {
                {"dummy1", (const uint8_t)(dummy1 & 0x1)},
                {"dummy2", (const uint8_t)(dummy2 & 0x1)},
                {"dummy3", (const uint8_t)(dummy3 & 0x1)},
                {"dummy4", (const uint8_t)(dummy4 & 0x7)},
                {"dummy5", (const uint8_t)(dummy5 & 0xF)},
                {"dummy6", (const uint8_t)(dummy6 & 0x1F)},
                {"dummy7", (const uint8_t)(dummy7 & 0x3F)},
                {"dummy8", (const uint8_t)(dummy8 & 0xFF)},
            };
        };
    public:
        virtual const std::string getClassName() const override { return "EasyScratch"; };
        /* Construct using default values from docs */
        EasyScratch(const uint8_t dummy1, const uint8_t dummy2, const uint8_t dummy3, const uint8_t dummy4, const uint8_t dummy5, const uint8_t dummy6, const uint8_t dummy7, const uint8_t dummy8)
        {
            construct(dummy1, dummy2, dummy3, dummy4, dummy5, dummy6, dummy7, dummy8);
        }
        /* Construct from low-level bit mask in line with docs */
        EasyScratch(const uint32_t mask)
        {
            constructFromMask(mask);
        }
    }; // class EasyScratch


    /**
     * @class EasyDPPScratch
     * @brief For user-friendly configuration of Readout Status mask.
     *
     * NOTE: identical to EasyScratch
     */
    class EasyDPPScratch : public EasyScratch
    {
    /* Just inherit everything else from parent class */
    public:
        virtual const std::string getClassName() const override { return "EasyDPPScratch"; };
        /* Construct using default values from docs */
        EasyDPPScratch(const uint8_t dummy1, const uint8_t dummy2, const uint8_t dummy3, const uint8_t dummy4, const uint8_t dummy5, const uint8_t dummy6, const uint8_t dummy7, const uint8_t dummy8) : EasyScratch(dummy1, dummy2, dummy3, dummy4, dummy5, dummy6, dummy7, dummy8) {};
        EasyDPPScratch(const uint32_t mask) : EasyScratch(mask) {};
    }; // class EasyDPPScratch


    /**
     * @class EasyAMCFirmwareRevision
     * @brief For user-friendly configuration of AMC Firmware Revision.
     *
     * This register contains the channel FPGA (AMC) firmware revision
     * information. The complete format is:\n
     * Firmware Revision = X.Y (16 lower bits)\n
     * Firmware Revision Date = Y/M/DD (16 higher bits)\n
     * EXAMPLE 1: revision 1.03, November 12th, 2007 is 0x7B120103.\n
     * EXAMPLE 2: revision 2.09, March 7th, 2016 is 0x03070209.\n
     * NOTE: the nibble code for the year makes this information to roll
     * over each 16 years.
     *
     * @param EasyAMCFirmwareRevision::minorRevisionNumber
     * AMC Firmware Minor Revision Number (Y).
     * @param EasyAMCFirmwareRevision::majorRevisionNumber
     * AMC Firmware Major Revision Number (X).
     * @param EasyAMCFirmwareRevision::revisionDate
     * AMC Firmware Revision Date (Y/M/DD).
     */
    class EasyAMCFirmwareRevision : public EasyBase
    {
    protected:
        /* Shared base since one constructor cannot reuse the other */
        /*
         * EasyAMCFirmwareRevision fields:
         * minor revision number in [0:7], major revision number in [8:15],
         * revision date in [16:31].
         */
        /* NOTE: we split revision date into the four digits internally
         since it is just four 4-bit values clamped into 16-bit anyway.
         This makes the generic and DPP version much more similar, too. */
        void initLayout()
        {
            layout = {
                {"minorRevisionNumber", {(const uint8_t)8, (const uint8_t)0}},
                {"majorRevisionNumber", {(const uint8_t)8, (const uint8_t)8}},
                {"revisionDayLower", {(const uint8_t)4, (const uint8_t)16}},
                {"revisionDayUpper", {(const uint8_t)4, (const uint8_t)20}},
                {"revisionMonth", {(const uint8_t)4, (const uint8_t)24}},
                {"revisionYear", {(const uint8_t)4, (const uint8_t)28}}
            };
        }
        void construct(const uint8_t minorRevisionNumber, const uint8_t majorRevisionNumber, const uint8_t revisionDayLower, const uint8_t revisionDayUpper, const uint8_t revisionMonth, const uint8_t revisionYear)
        {
            initLayout();
            variables = {
                {"minorRevisionNumber", (const uint8_t)minorRevisionNumber},
                {"majorRevisionNumber", (const uint8_t)majorRevisionNumber},
                {"revisionDayLower", (const uint8_t)(revisionDayLower & 0x7)},
                {"revisionDayUpper", (const uint8_t)(revisionDayUpper & 0x7)},
                {"revisionMonth", (const uint8_t)(revisionMonth & 0x7)},
                {"revisionYear", (const uint8_t)(revisionYear & 0x7)}
            };
        }
    public:
        virtual const std::string getClassName() const override { return "EasyAMCFirmwareRevision"; };
        /* Construct using default values from docs */
        /* We allow both clamped and individual revision date format here */
        EasyAMCFirmwareRevision(const uint8_t minorRevisionNumber, const uint8_t majorRevisionNumber, const uint16_t revisionDate)
        {
            const uint8_t revisionDayLower = (const uint8_t)(revisionDate & 0x7);
            const uint8_t revisionDayUpper = (const uint8_t)((revisionDate >> 4) & 0x7);
            const uint8_t revisionMonth = (const uint8_t)((revisionDate >> 8) & 0x7);
            const uint8_t revisionYear = (const uint8_t)((revisionDate >> 12) & 0x7);
            construct(minorRevisionNumber, majorRevisionNumber, revisionDayLower, revisionDayUpper, revisionMonth, revisionYear);
        }
        EasyAMCFirmwareRevision(const uint8_t minorRevisionNumber, const uint8_t majorRevisionNumber, const uint8_t revisionDayLower, const uint8_t revisionDayUpper, const uint8_t revisionMonth, const uint8_t revisionYear)
        {
            construct(minorRevisionNumber, majorRevisionNumber, revisionDayLower, revisionDayUpper, revisionMonth, revisionYear);
        }
        /* Construct from low-level bit mask in line with docs */
        EasyAMCFirmwareRevision(const uint32_t mask)
        {
            constructFromMask(mask);
        }
    }; // class EasyAMCFirmwareRevision


    /**
     * @class EasyDPPAMCFirmwareRevision
     * @brief For user-friendly configuration of DPP AMC FPGA Firmware Revision.
     *
     * Returns the DPP firmware revision (mezzanine level).
     * To control the mother board firmware revision see register 0x8124.\n
     * For example: if the register value is 0xC3218303:\n
     * - Firmware Code and Firmware Revision are 131.3\n
     * - Build Day is 21\n
     * - Build Month is March\n
     * - Build Year is 2012.\n
     * NOTE: since 2016 the build year started again from 0.
     *
     * @param EasyDPPAMCFirmwareRevision::firmwareRevisionNumber
     * Firmware revision number
     * @param EasyDPPAMCFirmwareRevision::firmwareDPPCode
     * Firmware DPP code. Each DPP firmware has a unique code.
     * @param EasyDPPAMCFirmwareRevision::buildDayLower
     * Build Day (lower digit)
     * @param EasyDPPAMCFirmwareRevision::buildDayUpper
     * Build Day (upper digit)
     * @param EasyDPPAMCFirmwareRevision::buildMonth
     * Build Month. For example: 3 means March, 12 is December.
     * @param EasyDPPAMCFirmwareRevision::buildYear
     * Build Year. For example: 0 means 2000, 12 means 2012. NOTE: since
     * 2016 the build year started again from 0.
     */
    class EasyDPPAMCFirmwareRevision : public EasyBase
    {
    protected:
        /* Shared base since one constructor cannot reuse the other */
        /*
         * EasyDPPAMCFirmwareRevision fields:
         * firmware revision number in [0:7], firmware DPP code in [8:15],
         * build day lower in [16:19], build day upper in [20:23],
         * build month in [24:27], build year in [28:31]
         */
        void initLayout()
        {
            layout = {
                {"firmwareRevisionNumber", {(const uint8_t)8, (const uint8_t)0}},
                {"firmwareDPPCode", {(const uint8_t)8, (const uint8_t)8}},
                {"buildDayLower", {(const uint8_t)4, (const uint8_t)16}},
                {"buildDayUpper", {(const uint8_t)4, (const uint8_t)20}},
                {"buildMonth", {(const uint8_t)4, (const uint8_t)24}},
                {"buildYear", {(const uint8_t)4, (const uint8_t)28}}
            };
        }
        /* NOTE: use inherited generic constructFromMask(mask) */
        void construct(const uint8_t firmwareRevisionNumber, const uint8_t firmwareDPPCode, const uint8_t buildDayLower, const uint8_t buildDayUpper, const uint8_t buildMonth, const uint8_t buildYear)
        {
            initLayout();
            variables = {
                {"firmwareRevisionNumber", (const uint8_t)firmwareRevisionNumber},
                {"firmwareDPPCode", (const uint8_t)firmwareDPPCode},
                {"buildDayLower", (const uint8_t)(buildDayLower & 0xF)},
                {"buildDayUpper", (const uint8_t)(buildDayUpper & 0xF)},
                {"buildMonth", (const uint8_t)(buildMonth & 0xF)},
                {"buildYear", (const uint8_t)(buildYear & 0xF)}
            };
        }
    public:
        virtual const std::string getClassName() const override { return "EasyDPPAMCFirmwareRevision"; };
        EasyDPPAMCFirmwareRevision(const uint8_t firmwareRevisionNumber, const uint8_t firmwareDPPCode, const uint8_t buildDayLower, const uint8_t buildDayUpper, const uint8_t buildMonth, const uint8_t buildYear)
        {
            construct(firmwareRevisionNumber, firmwareDPPCode, buildDayLower, buildDayUpper, buildMonth, buildYear);
        }
        /* Construct from low-level bit mask in line with docs */
        EasyDPPAMCFirmwareRevision(const uint32_t mask)
        {
            constructFromMask(mask);
        }
    }; // class EasyDPPAMCFirmwareRevision


    /**
     * @class EasyDPPAlgorithmControl
     * @brief For user-friendly configuration of DPP Algorithm Control mask.
     *
     * Management of the DPP algorithm features.
     *
     * @param EasyDPPAlgorithmControl::chargeSensitivity
     * Charge Sensitivity: defines how many pC of charge correspond to
     * one channel of the energy spectrum. Options are:\n
     * 000: 0.16 pC\n
     * 001: 0.32 pC\n
     * 010: 0.64 pC\n
     * 011: 1.28 pC\n
     * 100: 2.56 pC\n
     * 101: 5.12 pC\n
     * 110: 10.24 pC\n
     * 111: 20.48 pC.
     * @param EasyDPPAlgorithmControl::internalTestPulse
     * Internal Test Pulse. It is possible to enable an internal test
     * pulse for debugging purposes. The ADC counts are replaced with
     * the built-in pulse emulator. Options are:\n
     * 0: disabled\n
     * 1: enabled.
     * @param EasyDPPAlgorithmControl::testPulseRate
     * Test Pulse Rate. Set the rate of the built-in test pulse
     * emulator. Options are:\n
     * 00: 1 kHz\n
     * 01: 10 kHz\n
     * 10: 100 kHz\n
     * 11: 1 MHz.
     * @param EasyDPPAlgorithmControl::chargePedestal
     * Charge Pedestal: when enabled a fixed value of 1024 is added to
     * the charge. This feature is useful in case of energies close to
     * zero.
     * @param EasyDPPAlgorithmControl::inputSmoothingFactor
     * Input smoothing factor n. In case of noisy signal it is possible
     * to apply a smoothing filter, where each sample is replaced with
     * the mean value of n previous samples. When enabled, the trigger
     * is evaluated on the smoothed samples, while the charge
     * integration will be performed on the samples corresponding to the
     * ”Analog Probe” selection (bits[13:12] of register 0x8000). In any
     * case the output data contains the smoothed samples. Options are:\n
     * 000: disabled\n
     * 001: 2 samples\n
     * 010: 4 samples\n
     * 011: 8 samples\n
     * 100: 16 samples\n
     * 101: 32 samples\n
     * 110: 64 samples\n
     * 111: Reserved.
     * @param EasyDPPAlgorithmControl::pulsePolarity
     * Pulse Polarity. Options are:\n
     * 0: positive pulse\n
     * 1: negative pulse.
     * @param EasyDPPAlgorithmControl::triggerMode
     * Trigger Mode. Options are:\n
     * 00: Normal mode. Each channel can self-trigger independently from
     * the other channels.\n
     * 01: Paired mode. Each channel of a couple ’n’ acquire the event
     * in logic OR between its self-trigger and the self-trigger of the
     * other channel of the couple. Couple n corresponds to channel n
     * and channel n+2\n
     * 10: Reserved\n
     * 11: Reserved.
     * @param EasyDPPAlgorithmControl::baselineMean
     * Baseline Mean. Sets the number of events for the baseline mean
     * calculation. Options are:\n
     * 000: Fixed: the baseline value is fixed to the value set in
     * register 0x1n38\n
     * 001: 4 samples\n
     * 010: 16 samples\n
     * 011: 64 samples.
     * @param EasyDPPAlgorithmControl::disableSelfTrigger
     * Disable Self Trigger. If disabled, the self-trigger can be still
     * propagated to the TRG-OUT front panel connector, though it is not
     * used by the channel to acquire the event. Options are:\n
     * 0: self-trigger enabled\n
     * 1: self-trigger disabled.
     * @param EasyDPPAlgorithmControl::triggerHysteresis
     * Trigger Hysteresis. The trigger can be inhibited during the
     * trailing edge of a pulse, to avoid re-triggering on the pulse
     * itself. Options are:\n
     * 0 (default value): enabled\n
     * 1: disabled.
     */
    class EasyDPPAlgorithmControl : public EasyBase
    {
    protected:
        /* Shared base since one constructor cannot reuse the other */
        /*
         * EasyDPPAlgorithmControl fields:
         * charge sensitivity in [0:2], internal test pulse in [4],
         * test pulse rate in [5:6], charge pedestal in [8],
         * input smoothing factor in [12:14], pulse polarity in [16],
         * trigger mode in [18:19], baseline mean in [20:22],
         * disable self trigger in [24], trigger hysteresis in [30].
         */
        void initLayout()
        {
            layout = {
                {"chargeSensitivity", {(const uint8_t)3, (const uint8_t)0}},
                {"internalTestPulse", {(const uint8_t)1, (const uint8_t)4}},
                {"testPulseRate", {(const uint8_t)2, (const uint8_t)5}},
                {"chargePedestal", {(const uint8_t)1, (const uint8_t)8}},
                {"inputSmoothingFactor", {(const uint8_t)3, (const uint8_t)12}},
                {"pulsePolarity", {(const uint8_t)1, (const uint8_t)16}},
                {"triggerMode", {(const uint8_t)2, (const uint8_t)18}},
                {"baselineMean", {(const uint8_t)3, (const uint8_t)20}},
                {"disableSelfTrigger", {(const uint8_t)1, (const uint8_t)24}},
                {"triggerHysteresis", {(const uint8_t)1, (const uint8_t)30}}
            };
        }
        /* NOTE: use inherited generic constructFromMask(mask) */
        void construct(const uint8_t chargeSensitivity, const uint8_t internalTestPulse, const uint8_t testPulseRate, const uint8_t chargePedestal, const uint8_t inputSmoothingFactor, const uint8_t pulsePolarity, const uint8_t triggerMode, const uint8_t baselineMean, const uint8_t disableSelfTrigger, const uint8_t triggerHysteresis)
        {
            initLayout();
            variables = {
                {"chargeSensitivity", (const uint8_t)(chargeSensitivity & 0x7)},
                {"internalTestPulse", (const uint8_t)(internalTestPulse & 0x1)},
                {"testPulseRate", (const uint8_t)(testPulseRate & 0x3)},
                {"chargePedestal", (const uint8_t)(chargePedestal & 0x1)},
                {"inputSmoothingFactor", (const uint8_t)(inputSmoothingFactor & 0x7)},
                {"pulsePolarity", (const uint8_t)(pulsePolarity & 0x1)},
                {"triggerMode", (const uint8_t)(triggerMode & 0x3)},
                {"baselineMean", (const uint8_t)(baselineMean & 0x7)},
                {"disableSelfTrigger", (const uint8_t)(disableSelfTrigger & 0x1)},
                {"triggerHysteresis", (const uint8_t)(triggerHysteresis & 0x1)}
            };
        }
    public:
        virtual const std::string getClassName() const override { return "EasyDPPAlgorithmControl"; };
        EasyDPPAlgorithmControl(const uint8_t chargeSensitivity, const uint8_t internalTestPulse, const uint8_t testPulseRate, const uint8_t chargePedestal, const uint8_t inputSmoothingFactor, const uint8_t pulsePolarity, const uint8_t triggerMode, const uint8_t baselineMean, const uint8_t disableSelfTrigger, const uint8_t triggerHysteresis)
        {
            construct(chargeSensitivity, internalTestPulse, testPulseRate, chargePedestal, inputSmoothingFactor, pulsePolarity, triggerMode, baselineMean, disableSelfTrigger, triggerHysteresis);
        }
        /* Construct from low-level bit mask in line with docs */
        EasyDPPAlgorithmControl(const uint32_t mask)
        {
            constructFromMask(mask);
        }
    }; // class EasyDPPAlgorithmControl


    /* Some low-level digitizer helpers */
    /**
     * @brief open raw digitizer

     * @param linkType: which kind of link or bus to connect with
     * @param linkNum: device index on the link or bus
     * @param conetNode: node index if using conet
     * @param VMEBaseAddress: device address if using VME connection
     * @returns
     * low-level digitizer handle.
     */
    static int openRawDigitizer(CAEN_DGTZ_ConnectionType linkType, int linkNum, int conetNode, uint32_t VMEBaseAddress) 
    {
        int handle;
        errorHandler(CAEN_DGTZ_OpenDigitizer(linkType, linkNum, conetNode, VMEBaseAddress, &handle));
        return handle;
    }
    /**
     * @brief extract board info from low-level digitizer handle
     * @param handle: low-level digitizer handle
     * @returns
     * Structure with board info.
     */
    static CAEN_DGTZ_BoardInfo_t getRawDigitizerBoardInfo(int handle) 
    {
        CAEN_DGTZ_BoardInfo_t boardInfo;
        errorHandler(CAEN_DGTZ_GetInfo(handle, &boardInfo));
        return boardInfo;
    }
    /**
     * @brief extract DPP firmware info from low-level digitizer handle
     * @param handle: low-level digitizer handle
     * @returns
     * Structure with DPP firmware info.
     */
    static CAEN_DGTZ_DPPFirmware_t getRawDigitizerDPPFirmware(int handle) 
    {
        CAEN_DGTZ_DPPFirmware_t firmware;
        errorHandler(_CAEN_DGTZ_GetDPPFirmwareType(handle, &firmware));
        return firmware;
    }
    /**
     * @brief close low-level digitizer handle
     * @param handle: low-level digitizer handle
     */
    static void closeRawDigitizer(int handle) 
    {
        errorHandler(CAEN_DGTZ_CloseDigitizer(handle));
    }
    
    
    /**
     * @brief Generic digitizer abstraction
     */
    class Digitizer
    {
    private:
        Digitizer() {}
        Digitizer(int handle) : handle_(handle) { boardInfo_ = getRawDigitizerBoardInfo(handle_); }

    protected:
        int handle_;
        CAEN_DGTZ_BoardInfo_t boardInfo_;
        Digitizer(int handle, CAEN_DGTZ_BoardInfo_t boardInfo) : handle_(handle) { boardInfo_ = boardInfo; }
        virtual uint32_t filterBoardConfigurationSetMask(uint32_t mask)
        { return mask; }
        virtual uint32_t filterBoardConfigurationUnsetMask(uint32_t mask)
        { return mask; }

    public:
        public:

        /* Digitizer creation */
        static Digitizer* open(CAEN_DGTZ_ConnectionType linkType, int linkNum, int conetNode, uint32_t VMEBaseAddress);
        /**
         * @brief Instantiate Digitizer from USB device.
         * @param linkNum: device index on the bus
         * @returns
         * Abstracted digitizer instance for the device.
         */
        static Digitizer* USB(int linkNum) { return open(CAEN_DGTZ_USB,linkNum,0,0); }
        /**
         * @brief Instantiate Digitizer from USB device.
         * @param linkNum: device index on the bus
         * @param VMEBaseAddress: device address if using VME connection
         * @returns
         * Abstracted digitizer instance for the device.
         */
        static Digitizer* USB(int linkNum, uint32_t VMEBaseAddress) { return open(CAEN_DGTZ_USB,linkNum,0,VMEBaseAddress);}

        /* Destruction */
        /**
         * @brief close Digitizer instance.
         * @param handle: low-level device handle
         */
        static void close(int handle) { closeRawDigitizer(handle); }
        /**
         * @brief Destroy Digitizer instance.
         */
        ~Digitizer() { close(handle_); }

        /* Information functions */
        const std::string modelName() const {return std::string(boardInfo_.ModelName);}
        uint32_t modelNo() const {return boardInfo_.Model; }
        virtual uint32_t channels() const { return boardInfo_.Channels; }
        // by default groups do not exists. I.e. one channel pr. group
        virtual uint32_t groups() const { return boardInfo_.Channels; }
        virtual uint32_t channelsPerGroup() const { return 1; }
        uint32_t formFactor() const { return  boardInfo_.FormFactor; }
        uint32_t familyCode() const { return boardInfo_.FamilyCode; }
        const std::string ROCfirmwareRel() const { return std::string(boardInfo_.ROC_FirmwareRel); }
        const std::string AMCfirmwareRel() const { return std::string(boardInfo_.AMC_FirmwareRel); }
        uint32_t serialNumber() const { return boardInfo_.SerialNumber; }
        uint32_t PCBrevision() const { return boardInfo_.PCB_Revision; }
        uint32_t ADCbits() const { return boardInfo_.ADC_NBits; }
        int commHandle() const { return boardInfo_.CommHandle; }
        int VMEhandle() const { return boardInfo_.VMEHandle; }
        const std::string license() const { return std::string(boardInfo_.License); }

        int handle() { return handle_; }

        CAEN_DGTZ_DPPFirmware_t getDPPFirmwareType()
        { CAEN_DGTZ_DPPFirmware_t firmware = CAEN_DGTZ_NotDPPFirmware; errorHandler(_CAEN_DGTZ_GetDPPFirmwareType(handle_, &firmware)); return firmware; }

        /* Raw register read/write functions */
        void writeRegister(uint32_t address, uint32_t value)
        { errorHandler(CAEN_DGTZ_WriteRegister(handle_, address, value)); }
        uint32_t readRegister(uint32_t address)
        { uint32_t value; errorHandler(CAEN_DGTZ_ReadRegister(handle_, address, &value)); return value; }

        /* Utility functions */
        void reset() { errorHandler(CAEN_DGTZ_Reset(handle_)); }

        void calibrate()
        { errorHandler(CAEN_DGTZ_Calibrate(handle_)); }

        uint32_t readTemperature(int32_t ch)
        {uint32_t temp; errorHandler(CAEN_DGTZ_ReadTemperature(handle_, ch, &temp)); return temp; }

        /* Note: to be used only with x742 series. */
        void loadDRS4CorrectionData(CAEN_DGTZ_DRS4Frequency_t frequency)
        { errorHandler(CAEN_DGTZ_LoadDRS4CorrectionData(handle_, frequency)); }

        /* Enables/disables the data correction in the x742 series.
         *
         * Note: to be used only with x742 series.
         *
         * Note: if enabled, the data correction through the DecodeEvent function
         * only applies if a LoadDRS4CorrectionData has been previously
         *  called, otherwise the DecodeEvent runs the same, but data
         *  will be provided out not compensated.
         */
        void enableDRS4Correction()
        { errorHandler(CAEN_DGTZ_EnableDRS4Correction(handle_)); }
        void disableDRS4Correction()
        { errorHandler(CAEN_DGTZ_DisableDRS4Correction(handle_)); }

        /* Note: to be used only with 742 digitizer series. */
        CAEN_DGTZ_DRS4Correction_t getCorrectionTables(int frequency)
        { CAEN_DGTZ_DRS4Correction_t ctable; errorHandler(CAEN_DGTZ_GetCorrectionTables(handle_, frequency, &ctable)); return ctable; }

        void clearData()
        { errorHandler(CAEN_DGTZ_ClearData(handle_)); }

        void disableEventAlignedReadout()
        { errorHandler(CAEN_DGTZ_DisableEventAlignedReadout(handle_)); }

        void sendSWtrigger()
        { errorHandler(CAEN_DGTZ_SendSWtrigger(handle_)); }

        void startAcquisition()
        { errorHandler(CAEN_DGTZ_SWStartAcquisition(handle_)); }

        void stopAcquisition()
        { errorHandler(CAEN_DGTZ_SWStopAcquisition(handle_)); }

        ReadoutBuffer& readData(ReadoutBuffer& buffer,CAEN_DGTZ_ReadMode_t mode)
        { memset(buffer.data, 0, buffer.size); errorHandler(CAEN_DGTZ_ReadData(handle_, mode, buffer.data, &buffer.dataSize)); return buffer; }

        /* Interrupt control */

        /* NOTE: interrupts cannot be used in case of communication via
         * USB (either directly or through V1718 and VME)
         */

        InterruptConfig getInterruptConfig ()
        { InterruptConfig conf; errorHandler(CAEN_DGTZ_GetInterruptConfig(handle_, &conf.state, &conf.level, &conf.status_id, &conf.event_number, &conf.mode)); return conf; }
        void setInterruptConfig (InterruptConfig conf)
        { errorHandler(CAEN_DGTZ_SetInterruptConfig(handle_, conf.state, conf.level, conf.status_id, conf.event_number, conf.mode)); }

        void doIRQWait(uint32_t timeout)
        { errorHandler(CAEN_DGTZ_IRQWait(handle_, timeout)); }

        /* NOTE: VME* calls are for VME bus interrupts and work on a
         * separate VME handle. Not sure if they are needed. 
         */
        int doVMEIRQWait(CAEN_DGTZ_ConnectionType LinkType, int LinkNum, int ConetNode, uint8_t IRQMask,uint32_t timeout)
        { int vmehandle; errorHandler(CAEN_DGTZ_VMEIRQWait(LinkType, LinkNum, ConetNode, IRQMask, timeout, &vmehandle)); return vmehandle; }

        uint8_t doVMEIRQCheck(int vmehandle)
        { uint8_t mask; errorHandler(CAEN_DGTZ_VMEIRQCheck(vmehandle, &mask)); return mask; }

        int32_t doVMEIACKCycle(int vmehandle, uint8_t level)
        { int32_t board_id; errorHandler(CAEN_DGTZ_VMEIACKCycle(vmehandle, level, &board_id)); return board_id; }

        void rearmInterrupt()
        { errorHandler(CAEN_DGTZ_RearmInterrupt(handle_)); }

        /* Memory management */
        ReadoutBuffer mallocReadoutBuffer()
        { ReadoutBuffer b; b.data = NULL; b.size = 0; b.dataSize = 0; errorHandler(_CAEN_DGTZ_MallocReadoutBuffer(handle_, &b.data, &b.size)); return b; }
        void freeReadoutBuffer(ReadoutBuffer b)
        { errorHandler(CAEN_DGTZ_FreeReadoutBuffer(&b.data)); }

        // TODO Think of an intelligent way to handle events not using void pointers
        void* mallocEvent()
        { void* event; errorHandler(CAEN_DGTZ_AllocateEvent(handle_, &event)); return event; }
        void freeEvent(void* event)
        { errorHandler(CAEN_DGTZ_FreeEvent(handle_, (void**)&event)); }

        DPPEvents mallocDPPEvents()
        {
            DPPEvents events;
            /* MallocDPPEvents docs specify that event matrix always
             *  must have MAX_CHANNELS entries. We use nEvents for
             *  internal accounting so it must be same length. */
            events.ptr = new void*[MAX_CHANNELS];
            events.nEvents = new uint32_t[MAX_CHANNELS];
            for (int i = 0; i < MAX_CHANNELS; i++) {
                events.ptr[i] = NULL;
                events.nEvents[i] = 0;
            }
            switch(getDPPFirmwareType())
            {
                case CAEN_DGTZ_DPPFirmware_PHA:
                    events.elemSize = sizeof(CAEN_DGTZ_DPP_PHA_Event_t);
                    break;
                case CAEN_DGTZ_DPPFirmware_PSD:
                    events.elemSize = sizeof(CAEN_DGTZ_DPP_PSD_Event_t);
                    break;
                case CAEN_DGTZ_DPPFirmware_CI:
                    events.elemSize = sizeof(CAEN_DGTZ_DPP_CI_Event_t);
                    break;
                case CAEN_DGTZ_DPPFirmware_QDC:
                    events.elemSize = sizeof(_CAEN_DGTZ_DPP_QDC_Event_t);
                    break;
                default:
                    errorHandler(CAEN_DGTZ_FunctionNotAllowed);
            }
            errorHandler(_CAEN_DGTZ_MallocDPPEvents(handle_, events.ptr, &events.allocatedSize));
            return events;
        }
        void freeDPPEvents(DPPEvents events)
        { errorHandler(_CAEN_DGTZ_FreeDPPEvents(handle_, events.ptr)); delete[](events.ptr); delete[](events.nEvents); events.ptr = NULL; events.nEvents = NULL; }

        DPPWaveforms mallocDPPWaveforms()
        { DPPWaveforms waveforms; waveforms.ptr = NULL; waveforms.allocatedSize = 0; errorHandler(_CAEN_DGTZ_MallocDPPWaveforms(handle_, &waveforms.ptr, &waveforms.allocatedSize)); return waveforms; }
        void freeDPPWaveforms(DPPWaveforms waveforms)
        { errorHandler(_CAEN_DGTZ_FreeDPPWaveforms(handle_, waveforms.ptr)); waveforms.ptr = NULL; waveforms.allocatedSize = 0; }

        /* Detector data information and manipulation*/
        /* TODO: getNumEvents is only accurate for non-DDP firmware - disable for DPP? */
        uint32_t getNumEvents(ReadoutBuffer buffer)
        {uint32_t n; errorHandler(CAEN_DGTZ_GetNumEvents(handle_, buffer.data, buffer.dataSize, &n)); return n; }

        /* TODO: pass existing EventInfo as &info here like for getDPPEvents?
         * it appears we may end up using an uninitialized EventInfo data
         * pointer otherwise.
         * NOTE: In docs it sounds like it just returns a pointer into
         * the *existing* ReadoutBuffer for non-DPP events, so this might
         * actually be just fine for this case.
         */
        EventInfo getEventInfo(ReadoutBuffer buffer, int32_t n)
        { EventInfo info; errorHandler(CAEN_DGTZ_GetEventInfo(handle_, buffer.data, buffer.dataSize, n, &info, &info.data)); return info; }

        void* decodeEvent(EventInfo info, void* event)
        { errorHandler(CAEN_DGTZ_DecodeEvent(handle_, info.data, &event)); return event; }

        BasicEvent extractBasicEvent(EventInfo& info, void *event, uint32_t channel, uint32_t eventNo) {
            BasicEvent basic;
            /* TODO: we cannot generally assume event is CAEN_DGTZ_UINT16_EVE */
            CAEN_DGTZ_UINT16_EVENT_t *realEvent = (CAEN_DGTZ_UINT16_EVENT_t *)event;
            basic.boardId = info.BoardId;
            basic.eventIndex = eventNo;
            basic.channel = channel;
            basic.timestamp = info.TriggerTimeTag;
            basic.count = realEvent->ChSize[channel];
            memcpy(basic.samples, realEvent->DataChannel[channel], basic.count*sizeof(basic.samples));
            return basic;
        }

        DPPEvents& getDPPEvents(ReadoutBuffer buffer, DPPEvents& events)
        { errorHandler(_CAEN_DGTZ_GetDPPEvents(handle_, buffer.data, buffer.dataSize, events.ptr, events.nEvents)); return events; }

        void *extractDPPEvent(DPPEvents& events, uint32_t channel, uint32_t eventNo) { 
            void *event = NULL;
            switch(getDPPFirmwareType())
            {
            case CAEN_DGTZ_DPPFirmware_PHA:
                event = &(((CAEN_DGTZ_DPP_PHA_Event_t **)events.ptr)[channel][eventNo]);
                break;
            case CAEN_DGTZ_DPPFirmware_PSD:
                event = &(((CAEN_DGTZ_DPP_PSD_Event_t **)events.ptr)[channel][eventNo]);
                break;
            case CAEN_DGTZ_DPPFirmware_CI:
                event = &(((CAEN_DGTZ_DPP_CI_Event_t **)events.ptr)[channel][eventNo]);
                break;
            case CAEN_DGTZ_DPPFirmware_QDC:
                event = &(((_CAEN_DGTZ_DPP_QDC_Event_t **)events.ptr)[channel][eventNo]);
                break;
            default:
                errorHandler(CAEN_DGTZ_FunctionNotAllowed);
            }
            return event;
        }

        BasicDPPEvent extractBasicDPPEvent(DPPEvents& events, uint32_t channel, uint32_t eventNo) { 
            BasicDPPEvent basic;
            CAEN_DGTZ_DPP_PHA_Event_t PHAEvent;
            CAEN_DGTZ_DPP_PSD_Event_t PSDEvent;
            CAEN_DGTZ_DPP_CI_Event_t CIEvent;
            _CAEN_DGTZ_DPP_QDC_Event_t QDCEvent;
            switch(getDPPFirmwareType())
            {
            case CAEN_DGTZ_DPPFirmware_PHA:
                PHAEvent = ((CAEN_DGTZ_DPP_PHA_Event_t **)events.ptr)[channel][eventNo];
                basic.timestamp = PHAEvent.TimeTag;
                basic.format = PHAEvent.Format;
                /*NOTE: PHA does not contain Charge, pass Energy instead */
                basic.charge = PHAEvent.Energy;
                break;
            case CAEN_DGTZ_DPPFirmware_PSD:
                PSDEvent = ((CAEN_DGTZ_DPP_PSD_Event_t **)events.ptr)[channel][eventNo];
                basic.timestamp = PSDEvent.TimeTag;
                basic.format = PSDEvent.Format;
                /*NOTE: PSD contains two half-size Charge values - pack them */
                basic.charge = PSDEvent.ChargeLong << 16 | PSDEvent.ChargeShort;
                break;
            case CAEN_DGTZ_DPPFirmware_CI:
                CIEvent = ((CAEN_DGTZ_DPP_CI_Event_t **)events.ptr)[channel][eventNo];
                basic.timestamp = CIEvent.TimeTag;
                basic.format = CIEvent.Format;
                basic.charge = CIEvent.Charge;
                break;
            case CAEN_DGTZ_DPPFirmware_QDC:
                QDCEvent = ((_CAEN_DGTZ_DPP_QDC_Event_t **)events.ptr)[channel][eventNo];
                basic.timestamp = QDCEvent.TimeTag;
                basic.format = QDCEvent.Format;
                basic.charge = QDCEvent.Charge;
                break;
            default:
                errorHandler(CAEN_DGTZ_FunctionNotAllowed);
            }
            return basic;
        }
        
        /* NOTE: the backend function takes a single event from the
         * acquired event matrix and decodes it to waveforms. We expose
         * the direct call as well as the helper to extract the
         * waveforms for a given channel and event number element in the
         * events matrix. */
        DPPWaveforms& decodeDPPWaveforms(void *event, DPPWaveforms& waveforms)
        {
            errorHandler(_CAEN_DGTZ_DecodeDPPWaveforms(handle_, event, waveforms.ptr)); 
            return waveforms; }
        DPPWaveforms& decodeDPPWaveforms(DPPEvents& events, uint32_t channel, uint32_t eventNo, DPPWaveforms& waveforms)
        {
            void *event = extractDPPEvent(events, channel, eventNo);
            return decodeDPPWaveforms(event, waveforms); 
        }

        BasicDPPWaveforms extractBasicDPPWaveforms(DPPWaveforms& waveforms) { 
            BasicDPPWaveforms basic;
            CAEN_DGTZ_DPP_PHA_Waveforms_t *PHAWaveforms;
            CAEN_DGTZ_DPP_PSD_Waveforms_t *PSDWaveforms;
            CAEN_DGTZ_DPP_CI_Waveforms_t *CIWaveforms;
            _CAEN_DGTZ_DPP_QDC_Waveforms_t *QDCWaveforms;
            switch(getDPPFirmwareType())
            {
            case CAEN_DGTZ_DPPFirmware_PHA:
                PHAWaveforms = (CAEN_DGTZ_DPP_PHA_Waveforms_t *)waveforms.ptr;
                basic.Ns = PHAWaveforms->Ns;
                /* NOTE: for whatever reason PHA uses int instead of
                   uint for Trace1 and Trace2. Fake uint for now. */
                std::cout << "WARNING: using uint16_t for Trace1 and Trace2 arrays in BasicDPPWaveforms - you may need to manually cast!" << std::endl;    
                basic.Sample1 = (uint16_t*)PHAWaveforms->Trace1;
                basic.Sample2 = (uint16_t*)PHAWaveforms->Trace2;
                basic.DSample1 = PHAWaveforms->DTrace1;
                basic.DSample2 = PHAWaveforms->DTrace2;
                basic.DSample3 = NULL;
                basic.DSample4 = NULL;
                break;
            case CAEN_DGTZ_DPPFirmware_PSD:
                PSDWaveforms = (CAEN_DGTZ_DPP_PSD_Waveforms_t *)waveforms.ptr;
                basic.Ns = PSDWaveforms->Ns;
                basic.Sample1 = PSDWaveforms->Trace1;
                basic.Sample2 = PSDWaveforms->Trace2;
                basic.DSample1 = PSDWaveforms->DTrace1;
                basic.DSample2 = PSDWaveforms->DTrace2;
                basic.DSample3 = PSDWaveforms->DTrace3;
                basic.DSample4 = PSDWaveforms->DTrace4;
                break;
            case CAEN_DGTZ_DPPFirmware_CI:
                CIWaveforms = (CAEN_DGTZ_DPP_CI_Waveforms_t *)waveforms.ptr;
                basic.Ns = CIWaveforms->Ns;
                basic.Sample1 = CIWaveforms->Trace1;
                basic.Sample2 = CIWaveforms->Trace2;
                basic.DSample1 = CIWaveforms->DTrace1;
                basic.DSample2 = CIWaveforms->DTrace2;
                basic.DSample3 = CIWaveforms->DTrace3;
                basic.DSample4 = CIWaveforms->DTrace4;
                break;
            case CAEN_DGTZ_DPPFirmware_QDC:
                QDCWaveforms = (_CAEN_DGTZ_DPP_QDC_Waveforms_t *)waveforms.ptr;
                basic.Ns = QDCWaveforms->Ns;
                basic.Sample1 = QDCWaveforms->Trace1;
                basic.Sample2 = QDCWaveforms->Trace2;
                basic.DSample1 = QDCWaveforms->DTrace1;
                basic.DSample2 = QDCWaveforms->DTrace2;
                basic.DSample3 = QDCWaveforms->DTrace3;
                basic.DSample4 = QDCWaveforms->DTrace4;
                break;
            default:
                errorHandler(CAEN_DGTZ_FunctionNotAllowed);
            }
            return basic;
        }
        
        std::string dumpDPPWaveforms(DPPWaveforms& waveforms)
        { 
            std::stringstream ss;
            uint32_t allocatedSize = 0;
            uint32_t wavesNs = 0;
            allocatedSize = unsigned(waveforms.allocatedSize);
            //std::cout << "dumpDPPWaveforms: allocated size is " << allocatedSize << std::endl;
            ss << "allocatedSize="  << allocatedSize << " ";
            switch(getDPPFirmwareType())
            {
                case CAEN_DGTZ_DPPFirmware_PHA:
                    {
                        CAEN_DGTZ_DPP_PHA_Waveforms_t *waves = (CAEN_DGTZ_DPP_PHA_Waveforms_t *)(waveforms.ptr);
                        wavesNs = unsigned(waves->Ns);
                        ss << "PHA:Ns=" << wavesNs;
                        break;
                    }
                case CAEN_DGTZ_DPPFirmware_PSD:
                    {
                        CAEN_DGTZ_DPP_PSD_Waveforms_t *waves = (CAEN_DGTZ_DPP_PSD_Waveforms_t *)waveforms.ptr;
                        wavesNs = unsigned(waves->Ns);
                        ss << "PSD:Ns=" << wavesNs;
                        break;
                    }                    
                case CAEN_DGTZ_DPPFirmware_CI:
                    {
                        CAEN_DGTZ_DPP_CI_Waveforms_t *waves = (CAEN_DGTZ_DPP_CI_Waveforms_t *)waveforms.ptr;
                        wavesNs = unsigned(waves->Ns);
                        ss << "CI:Ns=" << wavesNs;
                        break;
                        }
                case CAEN_DGTZ_DPPFirmware_QDC:
                    {
                        _CAEN_DGTZ_DPP_QDC_Waveforms_t *waves = (_CAEN_DGTZ_DPP_QDC_Waveforms_t *)(waveforms.ptr);
                        wavesNs = unsigned(waves->Ns);
                        ss << "QDC:Ns=" << wavesNs;
                        break;
                    }
            default:
                ss << "UNKNOWN";
            }
            return ss.str();
        }

        /* Device configuration - i.e. getter and setters */
        uint32_t getRecordLength(int channel=-1) // Default channel -1 == all
        { uint32_t size; errorHandler(_CAEN_DGTZ_GetRecordLength(handle_,&size,channel)); return size; }
        void setRecordLength(uint32_t size)  // Default channel -1 == all
        { errorHandler(_CAEN_DGTZ_SetRecordLength(handle_,size,-1)); }
        void setRecordLength(int channel, uint32_t size)
        { errorHandler(_CAEN_DGTZ_SetRecordLength(handle_,size,channel)); }

        uint32_t getMaxNumEventsBLT()
        { uint32_t n; errorHandler(CAEN_DGTZ_GetMaxNumEventsBLT(handle_, &n)); return n; }
        void setMaxNumEventsBLT(uint32_t n)
        { errorHandler(CAEN_DGTZ_SetMaxNumEventsBLT(handle_, n)); }

        uint32_t getChannelEnableMask()
        { uint32_t mask; errorHandler(CAEN_DGTZ_GetChannelEnableMask(handle_, &mask)); return mask;}
        void setChannelEnableMask(uint32_t mask)
        { errorHandler(CAEN_DGTZ_SetChannelEnableMask(handle_, mask)); }

        uint32_t getGroupEnableMask()
        { uint32_t mask; errorHandler(CAEN_DGTZ_GetGroupEnableMask(handle_, &mask)); return mask;}
        void setGroupEnableMask(uint32_t mask)
        { errorHandler(CAEN_DGTZ_SetGroupEnableMask(handle_, mask)); }

        /* TODO: mark get/setDecimationFactor as not allowed on DPP?
         *       Not clear from docs but one should supposedly use the
         *       corresponding SetDPPParameters function there instead. */
        /**
         * @bug
         * CAEN_DGTZ_GetDecimationFactor fails with GenericError
         * on DT5740_171 and V1740D_137, apparently a mismatch between
         * DigitizerTable value end value read from register in the
         * V1740 specific case. 
         */
        uint16_t getDecimationFactor()
        { uint16_t factor; errorHandler(CAEN_DGTZ_GetDecimationFactor(handle_, &factor)); return factor; }
        void setDecimationFactor(uint16_t factor)
        { errorHandler(CAEN_DGTZ_SetDecimationFactor(handle_, factor)); }

        uint32_t getPostTriggerSize()
        { uint32_t percent; errorHandler(CAEN_DGTZ_GetPostTriggerSize(handle_, &percent)); return percent;}
        /* TODO: setPostTriggerSize fails with CommError on V1740D. */
        void setPostTriggerSize(uint32_t percent)
        { errorHandler(CAEN_DGTZ_SetPostTriggerSize(handle_, percent)); }

        CAEN_DGTZ_IOLevel_t getIOlevel()
        { CAEN_DGTZ_IOLevel_t level; errorHandler(CAEN_DGTZ_GetIOLevel(handle_, &level)); return level;}
        void setIOlevel(CAEN_DGTZ_IOLevel_t level)
        { errorHandler(CAEN_DGTZ_SetIOLevel(handle_, level));}

        CAEN_DGTZ_AcqMode_t getAcquisitionMode()
        { CAEN_DGTZ_AcqMode_t mode; errorHandler(CAEN_DGTZ_GetAcquisitionMode(handle_, &mode)); return mode;}
        void setAcquisitionMode(CAEN_DGTZ_AcqMode_t mode)
        { errorHandler(CAEN_DGTZ_SetAcquisitionMode(handle_, mode));}

        CAEN_DGTZ_TriggerMode_t getExternalTriggerMode()
        { CAEN_DGTZ_TriggerMode_t mode; errorHandler(CAEN_DGTZ_GetExtTriggerInputMode(handle_, &mode)); return mode; }
        void setExternalTriggerMode(CAEN_DGTZ_TriggerMode_t mode)
        { errorHandler(CAEN_DGTZ_SetExtTriggerInputMode(handle_, mode)); }

        virtual uint32_t getChannelDCOffset(uint32_t channel)
        { uint32_t offset; errorHandler(CAEN_DGTZ_GetChannelDCOffset(handle_, channel, &offset)); return offset; }
        virtual void setChannelDCOffset(uint32_t channel, uint32_t offset)
        { errorHandler(CAEN_DGTZ_SetChannelDCOffset(handle_, channel, offset)); }

        /**
         * @bug
         * Get/Set GroupDCOffset often fails with GenericError on V1740D_137
         * if used with specific group - something fails in the V1740
         * specific case. But for whatever reason it works fine in QDC
         * sample app unless a get is inserted before the set!?!
         */
        uint32_t getGroupDCOffset() 
        { 
            //std::cout << "in getGroupDCOffset broadcast" << std::endl;
            uint32_t offset; errorHandler(CAEN_DGTZ_ReadRegister(handle_, 0x8098, &offset)); return offset; 
        }
        virtual uint32_t getGroupDCOffset(uint32_t group)
        {
            //std::cout << "in getGroupDCOffset " << group << std::endl;
            uint32_t offset;
            if (group >= groups())  // Needed because of bug in CAEN_DGTZ_GetGroupDCOffset - patch sent
                errorHandler(CAEN_DGTZ_InvalidChannelNumber);
            uint32_t mask = readRegister(0x1088|group<<8); 
            if (mask & 0x4) {
                std::cerr << "precondition for getting Group DC Offset is NOT satisfied: mask is " << mask << std::endl;
                errorHandler(CAEN_DGTZ_CommError);
            }
            errorHandler(CAEN_DGTZ_GetGroupDCOffset(handle_, group, &offset)); return offset;
        }
        void setGroupDCOffset(uint32_t offset)
        {
            //std::cout << "in setGroupDCOffset broadcast" << std::endl;
            errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x8098, offset));
        }
        virtual void setGroupDCOffset(uint32_t group, uint32_t offset)
        {
            //std::cout << "in setGroupDCOffset " << group << ", " << offset << std::endl;
            if (group >= groups())  // Needed because of bug in CAEN_DGTZ_SetGroupDCOffset - patch sent
                errorHandler(CAEN_DGTZ_InvalidChannelNumber);
            /* NOTE: the 740 register docs emphasize that one MUST check
             * that the mask in reg 1n88  has mask[2]==0 before writing
             * to the DC Offset register, 1n98 - but the backend function
             * does not seem to do so!
             * We manually check here for now.
             */
            uint32_t mask = readRegister(0x1088|group<<8); 
            if (mask & 0x4) {
                std::cerr << "precondition for setting Group DC Offset is NOT satisfied: mask is " << mask << std::endl;
                errorHandler(CAEN_DGTZ_CommError);
            }            
            errorHandler(CAEN_DGTZ_SetGroupDCOffset(handle_, group, offset));
        }

        CAEN_DGTZ_TriggerMode_t getSWTriggerMode()
        { CAEN_DGTZ_TriggerMode_t mode; errorHandler(CAEN_DGTZ_GetSWTriggerMode(handle_, &mode)); return mode; }
        void setSWTriggerMode(CAEN_DGTZ_TriggerMode_t mode)
        { errorHandler(CAEN_DGTZ_SetSWTriggerMode(handle_, mode)); }

        CAEN_DGTZ_TriggerMode_t getChannelSelfTrigger(uint32_t channel)
        { CAEN_DGTZ_TriggerMode_t mode; errorHandler(CAEN_DGTZ_GetChannelSelfTrigger(handle_, channel, &mode)); return mode; }
        void setChannelSelfTrigger(uint32_t channel, CAEN_DGTZ_TriggerMode_t mode)
        { errorHandler(CAEN_DGTZ_SetChannelSelfTrigger(handle_, mode, 1<<channel)); }

        CAEN_DGTZ_TriggerMode_t getGroupSelfTrigger(uint32_t group)
        {
            if (group >= groups())  // Needed because of bug in CAEN_DGTZ_GetGroupSelfTrigger - patch sent
                errorHandler(CAEN_DGTZ_InvalidChannelNumber);
            CAEN_DGTZ_TriggerMode_t mode;
            /* TODO: report typo in digitizer function docs to upstream:
             * CAEN_DGTZ_SetGroupSelfTrigger twice instead of Get and Set */
            errorHandler(CAEN_DGTZ_GetGroupSelfTrigger(handle_, group, &mode));
            return mode; }
        void setGroupSelfTrigger(uint32_t group, CAEN_DGTZ_TriggerMode_t mode)
        {
            /* TODO: patch and send upstream */
            if (group >= groups())  // Needed because of bug in CAEN_DGTZ_SetGroupTriggerThreshold - patch pending
                errorHandler(CAEN_DGTZ_InvalidChannelNumber);

            errorHandler(CAEN_DGTZ_SetGroupSelfTrigger(handle_, mode, 1<<group)); }

        uint32_t getChannelTriggerThreshold(uint32_t channel)
        { uint32_t treshold; errorHandler(_CAEN_DGTZ_GetChannelTriggerThreshold(handle_, channel, &treshold)); return treshold; }
        void setChannelTriggerThreshold(uint32_t channel, uint32_t treshold)
        { errorHandler(_CAEN_DGTZ_SetChannelTriggerThreshold(handle_, channel, treshold)); }

        /**
         * @bug
         * CAEN_DGTZ_GetGroupTriggerThreshold fails with ReadDeviceRegisterFail
         * on V1740D_137. 
         */
        uint32_t getGroupTriggerThreshold(uint32_t group)
        {
            uint32_t treshold;
            if (group >= groups())  // Needed because of bug in CAEN_DGTZ_GetGroupTriggerThreshold - patch sent
                errorHandler(CAEN_DGTZ_InvalidChannelNumber);
            errorHandler(CAEN_DGTZ_GetGroupTriggerThreshold(handle_, group, &treshold)); return treshold;
        }
        void setGroupTriggerThreshold(uint32_t group, uint32_t treshold)
        {
            if (group >= groups())  // Needed because of bug in CAEN_DGTZ_SetGroupTriggerThreshold - patch sent
                errorHandler(CAEN_DGTZ_InvalidChannelNumber);
            errorHandler(CAEN_DGTZ_SetGroupTriggerThreshold(handle_, group, treshold));
        }

        uint32_t getChannelGroupMask(uint32_t group)
        {
            uint32_t mask;
            /* TODO: patch and send upstream */
            if (group >= groups())  // Needed because of bug in CAEN_DGTZ_GetGroupTriggerThreshold - patch pending
                errorHandler(CAEN_DGTZ_InvalidChannelNumber);
            errorHandler(_CAEN_DGTZ_GetChannelGroupMask(handle_, group, &mask)); return mask;
        }
        void setChannelGroupMask(uint32_t group, uint32_t mask)
        {
            /* TODO: patch and send upstream */
            if (group >= groups())  // Needed because of bug in CAEN_DGTZ_SetGroupTriggerThreshold - patch pending
                errorHandler(CAEN_DGTZ_InvalidChannelNumber);
            errorHandler(_CAEN_DGTZ_SetChannelGroupMask(handle_, group, mask));
        }

        CAEN_DGTZ_TriggerPolarity_t getTriggerPolarity(uint32_t channel)
        { CAEN_DGTZ_TriggerPolarity_t polarity; errorHandler(CAEN_DGTZ_GetTriggerPolarity(handle_, channel, &polarity)); return polarity; }
        void setTriggerPolarity(uint32_t channel, CAEN_DGTZ_TriggerPolarity_t polarity)
        { errorHandler(CAEN_DGTZ_SetTriggerPolarity(handle_, channel, polarity)); }

        uint32_t getGroupFastTriggerThreshold(uint32_t group)
        {
            uint32_t treshold;
            if (group >= groups())  // Needed because of bug in CAEN_DGTZ_GetGroupFastTriggerThreshold - patch sent
                errorHandler(CAEN_DGTZ_InvalidChannelNumber);
            errorHandler(CAEN_DGTZ_GetGroupFastTriggerThreshold(handle_, group, &treshold)); return treshold;
        }
        void setGroupFastTriggerThreshold(uint32_t group, uint32_t treshold)
        {
            if (group >= groups())  // Needed because of bug in CAEN_DGTZ_SetGroupFastTriggerThreshold - patch sent
                errorHandler(CAEN_DGTZ_InvalidChannelNumber);
            errorHandler(CAEN_DGTZ_SetGroupFastTriggerThreshold(handle_, group, treshold));
        }

        uint32_t getGroupFastTriggerDCOffset(uint32_t group)
        {
            uint32_t offset;
            if (group >= groups())  // Needed because of bug in CAEN_DGTZ_GetGroupFastTriggerDCOffset - patch sent
                errorHandler(CAEN_DGTZ_InvalidChannelNumber);
            errorHandler(CAEN_DGTZ_GetGroupFastTriggerDCOffset(handle_, group, &offset)); return offset;
        }
        void setGroupFastTriggerDCOffset(uint32_t group, uint32_t offset)
        {
            if (group >= groups())  // Needed because of bug in CAEN_DGTZ_SetGroupFastTriggerDCOffset - patch sent
                errorHandler(CAEN_DGTZ_InvalidChannelNumber);
            errorHandler(CAEN_DGTZ_SetGroupFastTriggerDCOffset(handle_, group, offset));
        }

        CAEN_DGTZ_EnaDis_t getFastTriggerDigitizing()
        { CAEN_DGTZ_EnaDis_t mode; errorHandler(CAEN_DGTZ_GetFastTriggerDigitizing(handle_, &mode)); return mode;}
        void setFastTriggerDigitizing(CAEN_DGTZ_EnaDis_t mode)
        { errorHandler(CAEN_DGTZ_SetFastTriggerDigitizing(handle_, mode));}

        CAEN_DGTZ_TriggerMode_t getFastTriggerMode()
        { 
            /* TODO: patch and send upstream */
            /* NOTE: it looks like model check is missing in upstream
             * get function. We mimic the check from the corresponding
             * set function here to refuse all but X742 model. */
            switch (familyCode()) {
            case CAEN_DGTZ_XX742_FAMILY_CODE:
                /* Allowed to proceed */
                break;
            default:
                errorHandler(CAEN_DGTZ_FunctionNotAllowed);
                break;    
            }
            CAEN_DGTZ_TriggerMode_t mode; 
            errorHandler(CAEN_DGTZ_GetFastTriggerMode(handle_, &mode)); 
            return mode;
        }
        void setFastTriggerMode(CAEN_DGTZ_TriggerMode_t mode)
        { errorHandler(CAEN_DGTZ_SetFastTriggerMode(handle_, mode)); }

        CAEN_DGTZ_DRS4Frequency_t getDRS4SamplingFrequency()
        { CAEN_DGTZ_DRS4Frequency_t frequency; errorHandler(CAEN_DGTZ_GetDRS4SamplingFrequency(handle_, &frequency)); return frequency; }
        void setDRS4SamplingFrequency(CAEN_DGTZ_DRS4Frequency_t frequency)
        { errorHandler(CAEN_DGTZ_SetDRS4SamplingFrequency(handle_, frequency)); }

        CAEN_DGTZ_RunSyncMode_t getRunSynchronizationMode()
        { CAEN_DGTZ_RunSyncMode_t mode; errorHandler(CAEN_DGTZ_GetRunSynchronizationMode(handle_, &mode)); return mode;}
        void setRunSynchronizationMode(CAEN_DGTZ_RunSyncMode_t mode)
        { errorHandler(CAEN_DGTZ_SetRunSynchronizationMode(handle_, mode));}

        CAEN_DGTZ_OutputSignalMode_t getOutputSignalMode()
        { CAEN_DGTZ_OutputSignalMode_t mode; errorHandler(CAEN_DGTZ_GetOutputSignalMode(handle_, &mode)); return mode;}
        void setOutputSignalMode(CAEN_DGTZ_OutputSignalMode_t mode)
        { errorHandler(CAEN_DGTZ_SetOutputSignalMode(handle_, mode));}

        CAEN_DGTZ_EnaDis_t getDESMode()
        { CAEN_DGTZ_EnaDis_t mode; errorHandler(CAEN_DGTZ_GetDESMode(handle_, &mode)); return mode;}
        void setDESMode(CAEN_DGTZ_EnaDis_t mode)
        { errorHandler(CAEN_DGTZ_SetDESMode(handle_, mode));}

        CAEN_DGTZ_ZS_Mode_t getZeroSuppressionMode()
        { CAEN_DGTZ_ZS_Mode_t mode; errorHandler(CAEN_DGTZ_GetZeroSuppressionMode(handle_, &mode)); return mode; }
        void setZeroSuppressionMode(CAEN_DGTZ_ZS_Mode_t mode)
        { errorHandler(CAEN_DGTZ_SetZeroSuppressionMode(handle_, mode)); }

        ZSParams getChannelZSParams(uint32_t channel=-1) // Default channel -1 == all
        { ZSParams params; errorHandler(CAEN_DGTZ_GetChannelZSParams(handle_, channel, &params.weight, &params.threshold, &params.nsamp)); return params; }
        void setChannelZSParams(ZSParams params)  // Default channel -1 == all
        { errorHandler(CAEN_DGTZ_SetChannelZSParams(handle_, -1, params.weight, params.threshold, params.nsamp));}
        void setChannelZSParams(uint32_t channel, ZSParams params)
        { errorHandler(CAEN_DGTZ_SetChannelZSParams(handle_, channel, params.weight, params.threshold, params.nsamp));}

        /* TODO: reject AnalogMonOutput access on DPP?
         * "Note: this function is not supported by V1742, V1743, and any
         * digitizer when running a DPP firmware."
         * According to digitizer library doc. Seems to be harmless on
         * 740DPP, however.
         */
        CAEN_DGTZ_AnalogMonitorOutputMode_t getAnalogMonOutput()
        { CAEN_DGTZ_AnalogMonitorOutputMode_t mode; errorHandler(CAEN_DGTZ_GetAnalogMonOutput(handle_, &mode)); return mode; }
        void setAnalogMonOutput(CAEN_DGTZ_AnalogMonitorOutputMode_t mode)
        { errorHandler(CAEN_DGTZ_SetAnalogMonOutput(handle_, mode)); }

        /* NOTE: CAENDigitizer API does not match current docs here.
         *       According to docs the Get function should take a plain
         *       uint32_t channelmask, and not a uint32_t *pointer* as
         *       it really does.
         *       The underlying implementation saves and loads the
         *       channelmask into a register, so the docs are wrong
         *       (confirmed by upstream).
         */
        /* NOTE: from the CAEN digitizer library it sounds like these
         * only apply for the V1724 model (in practice X780 and X781 too).
         * To make matters worse it looks like only the set function but
         * not the get function includes rejection of other models. Thus
         * we end up actually getting a value in our confs but can't set
         * it again without causing errors.
         */
        AIMParams getAnalogInspectionMonParams()
        { 
            /* TODO: patch and send upstream */
            /* NOTE: it looks like model check is missing in upstream
             * get function. We mimic the check from the corresponding
             * set function here to refuse all but X724, X780 and X781
             * models. */
            switch (familyCode()) {
            case CAEN_DGTZ_XX724_FAMILY_CODE:
            case CAEN_DGTZ_XX780_FAMILY_CODE:
            case CAEN_DGTZ_XX781_FAMILY_CODE:
                /* Allowed to proceed */
                break;
            default:
                errorHandler(CAEN_DGTZ_FunctionNotAllowed);
                break;    
            }
            /* NOTE: we explicitly initialize params here since some of
             * them may remain untouched garbage otherwise */
            AIMParams params; 
            params.channelmask = 0;
            params.offset = 0;
            params.mf = (CAEN_DGTZ_AnalogMonitorMagnify_t)0;
            params.ami = (CAEN_DGTZ_AnalogMonitorInspectorInverter_t)0;
            errorHandler(CAEN_DGTZ_GetAnalogInspectionMonParams(handle_, &params.channelmask, &params.offset, &params.mf, &params.ami)); return params; 
        }
        void setAnalogInspectionMonParams(AIMParams params)
        { errorHandler(CAEN_DGTZ_SetAnalogInspectionMonParams(handle_, params.channelmask, params.offset, params.mf, params.ami)); }

        CAEN_DGTZ_EnaDis_t getEventPackaging()
        { CAEN_DGTZ_EnaDis_t mode; errorHandler(CAEN_DGTZ_GetEventPackaging(handle_, &mode)); return mode;}
        void setEventPackaging(CAEN_DGTZ_EnaDis_t mode)
        { errorHandler(CAEN_DGTZ_SetEventPackaging(handle_, mode));}

        virtual uint32_t getDPPPreTriggerSize(int channel=-1) // Default channel -1 == all
        { uint32_t samples; errorHandler(CAEN_DGTZ_GetDPPPreTriggerSize(handle_, channel, &samples)); return samples; }
        virtual void setDPPPreTriggerSize(int channel, uint32_t samples)
        { errorHandler(CAEN_DGTZ_SetDPPPreTriggerSize(handle_, channel, samples)); }
        virtual void setDPPPreTriggerSize(uint32_t samples) // Default channel -1 == all
        { errorHandler(CAEN_DGTZ_SetDPPPreTriggerSize(handle_, -1, samples)); }

        /* TODO: mark get/setChannelPulsePolarity as not allowed on DPP?
         *       Not clear from docs but one should supposedly use the
         *       corresponding DPPAlgorithmControl bit there instead. */
        /**
         * @bug
         * CAEN_DGTZ_GetChannelPulsePolarity fails with InvalidParam om
         * DT5740_171 and V1740D_137. Seems to fail deep in readout when
         * the digitizer library calls ReadRegister 0x1n80.
         */
        CAEN_DGTZ_PulsePolarity_t getChannelPulsePolarity(uint32_t channel)
        { CAEN_DGTZ_PulsePolarity_t polarity; errorHandler(CAEN_DGTZ_GetChannelPulsePolarity(handle_, channel, &polarity)); return polarity; }
        void setChannelPulsePolarity(uint32_t channel, CAEN_DGTZ_PulsePolarity_t polarity)
        { errorHandler(CAEN_DGTZ_SetChannelPulsePolarity(handle_, channel, polarity)); }

        virtual DPPAcquisitionMode getDPPAcquisitionMode()
        { DPPAcquisitionMode mode; errorHandler(CAEN_DGTZ_GetDPPAcquisitionMode(handle_, &mode.mode, &mode.param)); return mode; }
        virtual void setDPPAcquisitionMode(DPPAcquisitionMode mode)
        { errorHandler(CAEN_DGTZ_SetDPPAcquisitionMode(handle_, mode.mode, mode.param)); }

        CAEN_DGTZ_DPP_TriggerMode_t getDPPTriggerMode()
        { CAEN_DGTZ_DPP_TriggerMode_t mode; errorHandler(CAEN_DGTZ_GetDPPTriggerMode(handle_, &mode)); return mode; }
        void setDPPTriggerMode(CAEN_DGTZ_DPP_TriggerMode_t mode)
        { errorHandler(CAEN_DGTZ_SetDPPTriggerMode(handle_, mode)); }

        int getDPP_VirtualProbe(int trace)
        { int probe; errorHandler(CAEN_DGTZ_GetDPP_VirtualProbe(handle_, trace, &probe)); return probe; }
        void setDPP_VirtualProbe(int trace, int probe)
        { errorHandler(CAEN_DGTZ_SetDPP_VirtualProbe(handle_, trace, probe)); }

        DPP_SupportedVirtualProbes getDPP_SupportedVirtualProbes(int trace)
        { DPP_SupportedVirtualProbes supported; errorHandler(CAEN_DGTZ_GetDPP_SupportedVirtualProbes(handle_, trace, (int *)&(supported.probes), &supported.numProbes)); return supported; }

        /* Functions specific to x743 */
        
        /* NOTE: with N channels SamIndex is always between 0 and N/2 – 1 */
        
        CAEN_DGTZ_SAM_CORRECTION_LEVEL_t getSAMCorrectionLevel()
        { CAEN_DGTZ_SAM_CORRECTION_LEVEL_t level; errorHandler(CAEN_DGTZ_GetSAMCorrectionLevel(handle_, &level)); return level; }
        void setSAMCorrectionLevel(CAEN_DGTZ_SAM_CORRECTION_LEVEL_t level)
        { errorHandler(CAEN_DGTZ_SetSAMCorrectionLevel(handle_, level)); }

        /* NOTE: docs claim that GetSAMPostTriggerSize takes an int32_t
         * pointer but the actual implementation uses a uint32_t pointer.
         * This appears to be a simple error in the docs. Reported upstream.
         */
        /* NOTE: GetSAMPostTriggerSize API takes an uint32_t pointer but
         * docs explicitly point out that value is always uint8_t:
         * "Value (range between 1 and 255) of the post-trigger delay
         * (pointer to, in case of Get) . Unit is the sampling period
         * multiplied by 16."
         * this also fits the type passed to SetSAMPostTriggerSize.
         * Upstream confirmed that there's no point in using a uint32 - but
         * changing the API might break dependent software. We just
         * leave it alone.
         */
        uint32_t getSAMPostTriggerSize(int samindex)
        { uint32_t value; errorHandler(CAEN_DGTZ_GetSAMPostTriggerSize(handle_, samindex, &value)); return value; }
        void setSAMPostTriggerSize(int samindex, uint8_t value)
        { errorHandler(CAEN_DGTZ_SetSAMPostTriggerSize(handle_, samindex, value)); }

        CAEN_DGTZ_SAMFrequency_t getSAMSamplingFrequency()
        { CAEN_DGTZ_SAMFrequency_t frequency; errorHandler(CAEN_DGTZ_GetSAMSamplingFrequency(handle_, &frequency)); return frequency; }
        void setSAMSamplingFrequency(CAEN_DGTZ_SAMFrequency_t frequency)
        { errorHandler(CAEN_DGTZ_SetSAMSamplingFrequency(handle_, frequency)); }

        /* NOTE: this is a public function according to docs but only
         * exposed as _CAEN_DGTZ_Read_EEPROM in actual API. We leave it
         * as is without trying to wrap buf nicely or anything.
         */
        unsigned char *read_EEPROM(int EEPROMIndex, unsigned short add, int nbOfBytes, unsigned char *buf)
        { errorHandler(_CAEN_DGTZ_Read_EEPROM(handle_, EEPROMIndex, add, nbOfBytes, buf)); return buf; }

        void loadSAMCorrectionData()
        { errorHandler(CAEN_DGTZ_LoadSAMCorrectionData(handle_)); }

        void enableSAMPulseGen(int channel, unsigned short pulsePattern, CAEN_DGTZ_SAMPulseSourceType_t pulseSource)
        { errorHandler(CAEN_DGTZ_EnableSAMPulseGen(handle_, channel, pulsePattern, pulseSource)); }
        void disableSAMPulseGen(int channel)
        { errorHandler(CAEN_DGTZ_DisableSAMPulseGen(handle_, channel)); }

        void sendSAMPulse()
        { errorHandler(CAEN_DGTZ_SendSAMPulse(handle_)); }

        CAEN_DGTZ_AcquisitionMode_t getSAMAcquisitionMode()
        { CAEN_DGTZ_AcquisitionMode_t mode; errorHandler(CAEN_DGTZ_GetSAMAcquisitionMode(handle_, &mode)); return mode; }
        void setSAMAcquisitionMode(CAEN_DGTZ_AcquisitionMode_t mode)
        { errorHandler(CAEN_DGTZ_SetSAMAcquisitionMode(handle_, mode)); }

        ChannelPairTriggerLogicParams getChannelPairTriggerLogic(uint32_t channelA, uint32_t channelB)
        { ChannelPairTriggerLogicParams params; errorHandler(CAEN_DGTZ_GetChannelPairTriggerLogic(handle_, channelA, channelB, &params.logic, &params.coincidenceWindow)); }
        void setChannelPairTriggerLogic(uint32_t channelA, uint32_t channelB, ChannelPairTriggerLogicParams params)
        { errorHandler(CAEN_DGTZ_SetChannelPairTriggerLogic(handle_, channelA, channelB, params.logic, params.coincidenceWindow)); }

        TriggerLogicParams getTriggerLogic()
        { TriggerLogicParams params; errorHandler(CAEN_DGTZ_GetTriggerLogic(handle_, &params.logic, &params.majorityLevel)); }
        void setTriggerLogic(TriggerLogicParams params)
        { errorHandler(CAEN_DGTZ_SetTriggerLogic(handle_, params.logic, params.majorityLevel)); }

        SAMTriggerCountVetoParams getSAMTriggerCountVetoParam(int channel)
        { SAMTriggerCountVetoParams params; errorHandler(CAEN_DGTZ_GetSAMTriggerCountVetoParam(handle_, channel, &params.enable, &params.vetoWindow)); }
        void setSAMTriggerCountVetoParam(int channel, SAMTriggerCountVetoParams params)
        { errorHandler(CAEN_DGTZ_SetSAMTriggerCountVetoParam(handle_, channel, params.enable, params.vetoWindow)); }

        void setDPPEventAggregation(int threshold, int maxsize)
        { errorHandler(CAEN_DGTZ_SetDPPEventAggregation(handle_, threshold, maxsize)); }

        /* NOTE: The channel arg is optional for some models:
         * "INT value corresponding to the channel index (required for
         * DPP-PSD and DPP-CI, ignored by DPP-PHA)."
         * We handle it properly in the backend implementation.
         */ 
        /* TODO: patch and send upstream? */
        /* NOTE: backend get and set functions are not symmetric - namely
         * the get includes explicit X751 handling which does NOT
         * truncate the read value to 1023, whereas set does not
         * handle X751 explicitly and thus will return InvalidParam for
         * any value above 1023. 
         * It looks like a bug that X751 is missing from the list of
         * explicit cases in the set function.
         */
        uint32_t getNumEventsPerAggregate() { return getNumEventsPerAggregate(-1); }
        uint32_t getNumEventsPerAggregate(int32_t channel)
        { uint32_t numEvents; errorHandler(_CAEN_DGTZ_GetNumEventsPerAggregate(handle_, &numEvents, channel)); return numEvents; }
        void setNumEventsPerAggregate(uint32_t numEvents) { setNumEventsPerAggregate(-1, numEvents); }
        void setNumEventsPerAggregate(uint32_t channel, uint32_t numEvents)
        { 
            uint32_t n = numEvents;
            /* NOTE: we explicitly cap numEvents to 1023 here for the 
             * X751 case as mentioned above. */
            switch (familyCode()) {
            case CAEN_DGTZ_XX751_FAMILY_CODE:
                /* Allowed to proceed */
                n &= 0x3FF;
                break;
            default:
                break;    
            }
            errorHandler(_CAEN_DGTZ_SetNumEventsPerAggregate(handle_, n, channel));
        }

        uint32_t getMaxNumAggregatesBLT()
        { uint32_t numAggr; errorHandler(CAEN_DGTZ_GetMaxNumAggregatesBLT(handle_, &numAggr)); return numAggr; }
        void setMaxNumAggregatesBLT(uint32_t numAggr)
        { errorHandler(CAEN_DGTZ_SetMaxNumAggregatesBLT(handle_, numAggr)); }

        void setDPPParameters(uint32_t channelmask, void *params)
        { errorHandler(CAEN_DGTZ_SetDPPParameters(handle_, channelmask, params)); }

        virtual uint32_t getAMCFirmwareRevision(uint32_t group) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyAMCFirmwareRevision getEasyAMCFirmwareRevision(uint32_t group) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyDPPAMCFirmwareRevision getEasyDPPAMCFirmwareRevision(uint32_t group) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getRunDelay() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setRunDelay(uint32_t delay) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getDPPGateWidth(uint32_t group) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setDPPGateWidth(uint32_t group, uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setDPPGateWidth(uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getDPPGateOffset(uint32_t group) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setDPPGateOffset(uint32_t group, uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setDPPGateOffset(uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getDPPFixedBaseline(uint32_t group) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setDPPFixedBaseline(uint32_t group, uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setDPPFixedBaseline(uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getDPPTriggerHoldOffWidth(uint32_t group) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual uint32_t getDPPTriggerHoldOffWidth() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setDPPTriggerHoldOffWidth(uint32_t group, uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setDPPTriggerHoldOffWidth(uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getDPPAlgorithmControl(uint32_t group) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setDPPAlgorithmControl(uint32_t group, uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setDPPAlgorithmControl(uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyDPPAlgorithmControl getEasyDPPAlgorithmControl(uint32_t group) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setEasyDPPAlgorithmControl(uint32_t group, EasyDPPAlgorithmControl settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setEasyDPPAlgorithmControl(EasyDPPAlgorithmControl settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getDPPShapedTriggerWidth(uint32_t group) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual uint32_t getDPPShapedTriggerWidth() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setDPPShapedTriggerWidth(uint32_t group, uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setDPPShapedTriggerWidth(uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getBoardConfiguration() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setBoardConfiguration(uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void unsetBoardConfiguration(uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyBoardConfiguration getEasyBoardConfiguration() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setEasyBoardConfiguration(EasyBoardConfiguration settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyDPPBoardConfiguration getEasyDPPBoardConfiguration() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setEasyDPPBoardConfiguration(EasyDPPBoardConfiguration settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getDPPAggregateOrganization() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setDPPAggregateOrganization(uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getAcquisitionControl() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setAcquisitionControl(uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyAcquisitionControl getEasyAcquisitionControl() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setEasyAcquisitionControl(EasyAcquisitionControl settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyDPPAcquisitionControl getEasyDPPAcquisitionControl() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setEasyDPPAcquisitionControl(EasyDPPAcquisitionControl settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getAcquisitionStatus() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual uint32_t getDPPAcquisitionStatus() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyAcquisitionStatus getEasyAcquisitionStatus() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyDPPAcquisitionStatus getEasyDPPAcquisitionStatus() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getGlobalTriggerMask() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setGlobalTriggerMask(uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyGlobalTriggerMask getEasyGlobalTriggerMask() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setEasyGlobalTriggerMask(EasyGlobalTriggerMask settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyDPPGlobalTriggerMask getEasyDPPGlobalTriggerMask() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setEasyDPPGlobalTriggerMask(EasyDPPGlobalTriggerMask settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getFrontPanelTRGOUTEnableMask() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setFrontPanelTRGOUTEnableMask(uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyFrontPanelTRGOUTEnableMask getEasyFrontPanelTRGOUTEnableMask() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setEasyFrontPanelTRGOUTEnableMask(EasyFrontPanelTRGOUTEnableMask settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyDPPFrontPanelTRGOUTEnableMask getEasyDPPFrontPanelTRGOUTEnableMask() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setEasyDPPFrontPanelTRGOUTEnableMask(EasyDPPFrontPanelTRGOUTEnableMask settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getFrontPanelIOControl() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setFrontPanelIOControl(uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyFrontPanelIOControl getEasyFrontPanelIOControl() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setEasyFrontPanelIOControl(EasyFrontPanelIOControl settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyDPPFrontPanelIOControl getEasyDPPFrontPanelIOControl() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setEasyDPPFrontPanelIOControl(EasyDPPFrontPanelIOControl settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getROCFPGAFirmwareRevision() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyROCFPGAFirmwareRevision getEasyROCFPGAFirmwareRevision() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyDPPROCFPGAFirmwareRevision getEasyDPPROCFPGAFirmwareRevision() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getEventSize() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getFanSpeedControl() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setFanSpeedControl(uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyFanSpeedControl getEasyFanSpeedControl() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setEasyFanSpeedControl(EasyFanSpeedControl settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyDPPFanSpeedControl getEasyDPPFanSpeedControl() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setEasyDPPFanSpeedControl(EasyDPPFanSpeedControl settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getDPPDisableExternalTrigger() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setDPPDisableExternalTrigger(uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getRunStartStopDelay() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setRunStartStopDelay(uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getReadoutControl() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setReadoutControl(uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyReadoutControl getEasyReadoutControl() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setEasyReadoutControl(EasyReadoutControl settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyDPPReadoutControl getEasyDPPReadoutControl() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setEasyDPPReadoutControl(EasyDPPReadoutControl settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getReadoutStatus() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyReadoutStatus getEasyReadoutStatus() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyDPPReadoutStatus getEasyDPPReadoutStatus() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getScratch() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setScratch(uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyScratch getEasyScratch() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setEasyScratch(EasyScratch settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyDPPScratch getEasyDPPScratch() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setEasyDPPScratch(EasyDPPScratch settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getDPPAggregateNumberPerBLT() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setDPPAggregateNumberPerBLT(uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        /* TODO: implement remaining register wrappers as virtual:NotImpemented? */

    }; // class Digitizer

    class Digitizer740 : public Digitizer
    {
    private:
        Digitizer740();
        friend Digitizer* Digitizer::open(CAEN_DGTZ_ConnectionType linkType, int linkNum, int conetNode, uint32_t VMEBaseAddress);

    protected:
        Digitizer740(int handle, CAEN_DGTZ_BoardInfo_t boardInfo) : Digitizer(handle,boardInfo) {}

    public:
        virtual uint32_t channels() const override { return groups()*channelsPerGroup(); }
        virtual uint32_t groups() const override { return boardInfo_.Channels; } // for x740: boardInfo.Channels stores number of groups
        virtual uint32_t channelsPerGroup() const override { return 8; } // 8 channels per group for x740

        /* NOTE: disable inherited 740 ChannelDCOffset since we can only
         *       allow group version here.
         */
        virtual uint32_t getChannelDCOffset(uint32_t channel) override { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setChannelDCOffset(uint32_t channel, uint32_t offset) override { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        /* NOTE: disable inherited 740 GroupDCOffset for specific group
         *       since it randomly fails.
         */
        virtual uint32_t getGroupDCOffset(uint32_t group) override { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setGroupDCOffset(uint32_t group, uint32_t offset) override { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        /* NOTE: BoardConfiguration differs in forced ones and zeros
         * between generic and DPP version. Use a class-specific mask.
         */
        /* According to register docs the bits [0,2,5,7:8,10,23]
         * must be 0 and the bits [4] must be 1 so we always
         * force compliance by a bitwise-or with 0x00000010 followed
         * by a bitwise-and with 0x008005A5 for the set operation.
         * Similarly we prevent mangling of the force ones by a
         * bitwise-and with the xor inverted version of 0x00000010 for
         * the unset operation.
         */
        virtual uint32_t filterBoardConfigurationSetMask(uint32_t mask) override
        { return ((mask | 0x00000010) & 0x008005A5); }
        virtual uint32_t filterBoardConfigurationUnsetMask(uint32_t mask) override
        { return (mask & (0xFFFFFFFF ^ 0x00000010)); }

        /**
         * @brief Get AMCFirmwareRevision mask
         *
         * This register contains the channel FPGA (AMC) firmware
         * revision information.\n
         * The complete format is:\n
         * Firmware Revision = X.Y (16 lower bits)\n
         * Firmware Revision Date = Y/M/DD (16 higher bits)\n
         * EXAMPLE 1: revision 1.03, November 12th, 2007 is 0x7B120103.\n
         * EXAMPLE 2: revision 2.09, March 7th, 2016 is 0x03070209.\n
         * NOTE: the nibble code for the year makes this information to
         * roll over each 16 years.
         *
         * Get the low-level AMCFirmwareRevision mask in line with
         * register docs. It is recommended to use the EasyX wrapper
         * version instead.
         *
         * @param group:
         * group index
         * @returns
         * 32-bit mask with layout described in register docs
         */
        uint32_t getAMCFirmwareRevision(uint32_t group) override
        {
            uint32_t mask;
            if (group >= groups())
                errorHandler(CAEN_DGTZ_InvalidChannelNumber);
            errorHandler(CAEN_DGTZ_ReadRegister(handle_, 0x108C | group<<8, &mask));
            return mask;
        }
        /**
         * @brief Easy Get AMCFirmwareRevision
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating from the bit mask returned by the
         * the underlying low-level get funtion.
         *
         * @param group:
         * group index
         * @returns
         * EasyAMCFirmwareRevision object
         */
        EasyAMCFirmwareRevision getEasyAMCFirmwareRevision(uint32_t group) override
        {
            uint32_t mask;
            mask = getAMCFirmwareRevision(group);
            return EasyAMCFirmwareRevision(mask);
        }

        /* NOTE: Get / Set DC Offset is handled in GroupDCOffset */

        /* NOTE: Get / Set Channel Enable Mask of Group is handled in
         * ChannelGroupMask */

        /* TODO: wrap Group n Low Channels DC Offset Individual Correction from register docs? */

        /* TODO: wrap Group n High Channels DC Offset Individual Correction from register docs? */

        /**
         * @brief Get BoardConfiguration mask
         *
         * This register contains general settings for the board
         * configuration.
         *
         * Get the low-level BoardConfiguration mask in line with
         * register docs. It is recommended to use the EasyX wrapper
         * version instead.
         *
         * NOTE: Read mask from 0x8000, BitSet mask with 0x8004 and
         *       BitClear mask with 0x8008.
         *
         * @returns
         * 32-bit mask with layout described in register docs
         */
        uint32_t getBoardConfiguration() override
        {
            uint32_t mask;
            errorHandler(CAEN_DGTZ_ReadRegister(handle_, 0x8000, &mask));
            return mask;
        }
        /**
         * @brief Set BoardConfiguration mask
         *
         * This register contains general settings for the board
         * configuration.
         *
         * Set the low-level BoardConfiguration mask in line with
         * register docs. It is recommended to use the EasyX wrapper
         * version instead.
         *
         * @param mask:
         * 32-bit mask with layout described in register docs
         */
        void setBoardConfiguration(uint32_t mask) override
        { 
            //errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x8004, filterBoardConfigurationSetMask(mask)));
            errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x8004, mask));
 }
        /**
         * @brief Unset BoardConfiguration mask
         *
         *
         * This register contains general settings for the board
         * configuration.
         *
         * Unset the low-level BoardConfiguration mask in line with
         * register docs. It is recommended to use the EasyX wrapper
         * version instead.
         *
         * @param mask:
         * 32-bit mask with layout described in register docs
         */
        void unsetBoardConfiguration(uint32_t mask) override
        { 
            //errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x8008, filterBoardConfigurationUnsetMask(mask)));
            errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x8008, mask));
 }

        /**
         * @brief Easy Get BoardConfiguration
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating from the bit mask returned by the
         * the underlying low-level get funtion.
         *
         * @returns
         * EasyBoardConfiguration object
         */
        EasyBoardConfiguration getEasyBoardConfiguration() override
        {
            uint32_t mask;
            mask = getBoardConfiguration();
            return EasyBoardConfiguration(mask);
        }
        /**
         * @brief Easy Set BoardConfiguration
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating to the bit mask needed by the
         * the underlying low-level set funtion.
         * 
         * IMPORTANT: this version takes care of both setting and unsetting
         * bits, unlike the low-level set and unset versions.
         *
         * @param settings:
         * EasyBoardConfiguration object
         */
        void setEasyBoardConfiguration(EasyBoardConfiguration settings) override
        {
            uint32_t mask = settings.toBits();
            /* NOTE: we explicitly unset all bits first since set
             * only enables bits */
            unsetBoardConfiguration(0xFFFFFFFF);
            setBoardConfiguration(mask);
        }

        /**
         * @brief Get AcquisitionControl mask
         *
         * This register manages the acquisition settings.
         *
         * Get the low-level AcquisitionControl mask in line with
         * register docs. It is recommended to use the EasyX wrapper
         * version instead.
         *
         * @returns
         * 32-bit mask with layout described in register docs
         */
        uint32_t getAcquisitionControl() override
        {
            uint32_t mask;
            errorHandler(CAEN_DGTZ_ReadRegister(handle_, 0x8100, &mask));
            return mask;
        }
        /**
         * @brief Set AcquisitionControl mask
         *
         * This register manages the acquisition settings.
         *
         * Set the low-level AcquisitionControl mask in line with
         * register docs. It is recommended to use the EasyX wrapper
         * version instead.
         *
         * @param mask:
         * 32-bit mask with layout described in register docs
         */
        void setAcquisitionControl(uint32_t mask) override
        { errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x8100, mask & 0x0FFF)); }
        /**
         * @brief Easy Get AcquisitionControl
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating from the bit mask returned by the
         * the underlying low-level get funtion.
         *
         * @returns
         * EasyAcquisitionControl object
         */
        EasyAcquisitionControl getEasyAcquisitionControl() override
        {
            uint32_t mask;
            mask = getAcquisitionControl();
            return EasyAcquisitionControl(mask);
        }
        /**
         * @brief Easy Set AcquisitionControl
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating to the bit mask needed by the
         * the underlying low-level set funtion.
         *
         * @param settings:
         * EasyAcquisitionControl object
         */
        void setEasyAcquisitionControl(EasyAcquisitionControl settings) override
        {
            uint32_t mask = settings.toBits();
            setAcquisitionControl(mask);
        }

        /**
         * @brief Get AcquisitionStatus mask
         *
         * This register monitors a set of conditions related to the
         * acquisition status.
         *
         * Get the low-level AcquisitionStatus mask in line with
         * register docs. It is recommended to use the EasyX wrapper
         * version instead.
         *
         * @returns
         * 32-bit mask with layout described in register docs
         */
        uint32_t getAcquisitionStatus() override
        {
            uint32_t mask;
            errorHandler(CAEN_DGTZ_ReadRegister(handle_, 0x8104, &mask));
            return mask;
        }
        /**
         * @brief Easy Get AcquisitionStatus
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating from the bit mask returned by the
         * the underlying low-level get funtion.
         *
         * @returns
         * EasyAcquisitionStatus object
         */
        EasyAcquisitionStatus getEasyAcquisitionStatus() override
        {
            uint32_t mask;
            mask = getAcquisitionStatus();
            return EasyAcquisitionStatus(mask);
        }

        /* TODO: is Software Trigger from register docs already covered
         * by sendSWtrigger? */

        /**
         * @brief Get GlobalTriggerMask
         *
         * This register sets which signal can contribute to the global
         * trigger generation.
         *
         * Get the low-level GlobalTriggerMask in line with
         * register docs. It is recommended to use the EasyX wrapper
         * version instead.
         *
         * @returns
         * 32-bit mask with layout described in register docs
         */
        uint32_t getGlobalTriggerMask() override
        {
            uint32_t mask;
            errorHandler(CAEN_DGTZ_ReadRegister(handle_, 0x810C, &mask));
            return mask;
        }
        /**
         * @brief Set GlobalTriggerMask
         *
         * This register sets which signal can contribute to the global
         * trigger generation.
         *
         * Set the low-level GlobalTriggerMask in line with
         * register docs. It is recommended to use the EasyX wrapper
         * version instead.
         *
         * @param mask:
         * 32-bit mask with layout described in register docs
         */
        void setGlobalTriggerMask(uint32_t mask) override
        { errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x810C, mask)); }
        /**
         * @brief Easy Get GlobalTriggerMask
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating from the bit mask returned by the
         * the underlying low-level get funtion.
         *
         * @returns
         * EasyGlobalTriggerMask object
         */
        EasyGlobalTriggerMask getEasyGlobalTriggerMask() override
        {
            uint32_t mask;
            mask = getGlobalTriggerMask();
            return EasyGlobalTriggerMask(mask);
        }
        /**
         * @brief Easy Set GlobalTriggerMask
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating to the bit mask needed by the
         * the underlying low-level set funtion.
         *
         * @param settings:
         * EasyGlobalTriggerMask object
         */
        void setEasyGlobalTriggerMask(EasyGlobalTriggerMask settings) override
        {
            uint32_t mask = settings.toBits();
            setGlobalTriggerMask(mask);
        }

        /**
         * @brief Get FrontPanelTRGOUTEnableMask
         *
         * This register sets which signal can contribute to generate
         * the signal on the front panel TRG-OUT LEMO connector (GPO in
         * case of DT and NIM boards).
         *
         * Get the low-level FrontPanelTRGOUTEnableMask in line with
         * register docs. It is recommended to use the EasyX wrapper
         * version instead.
         *
         * @returns
         * 32-bit mask with layout described in register docs
         */
        uint32_t getFrontPanelTRGOUTEnableMask() override
        {
            uint32_t mask;
            errorHandler(CAEN_DGTZ_ReadRegister(handle_, 0x8110, &mask));
            return mask;
        }
        /**
         * @brief Set FrontPanelTRGOUTEnableMask
         *
         * This register sets which signal can contribute to generate
         * the signal on the front panel TRG-OUT LEMO connector (GPO in
         * case of DT and NIM boards).
         *
         * Set the low-level FrontPanelTRGOUTEnableMask in line with
         * register docs. It is recommended to use the EasyX wrapper
         * version instead.
         *
         * @param mask:
         * 32-bit mask with layout described in register docs
         */
        void setFrontPanelTRGOUTEnableMask(uint32_t mask) override
        { errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x8110, mask)); }
        /**
         * @brief Easy Get FrontPanelTRGOUTEnableMask
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating from the bit mask returned by the
         * the underlying low-level get funtion.
         *
         * @returns
         * EasyFrontPanelTRGOUTEnableMask object
         */
        EasyFrontPanelTRGOUTEnableMask getEasyFrontPanelTRGOUTEnableMask() override
        {
            uint32_t mask;
            mask = getFrontPanelTRGOUTEnableMask();
            return EasyFrontPanelTRGOUTEnableMask(mask);
        }
        /**
         * @brief Easy Set FrontPanelTRGOUTEnableMask
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating to the bit mask needed by the
         * the underlying low-level set funtion.
         *
         * @param settings:
         * EasyFrontPanelTRGOUTEnableMask object
         */
        void setEasyFrontPanelTRGOUTEnableMask(EasyFrontPanelTRGOUTEnableMask settings) override
        {
            uint32_t mask = settings.toBits();
            setFrontPanelTRGOUTEnableMask(mask);
        }

        /* TODO: wrap Post Trigger from register docs? */

        /* TODO: wrap LVDS I/O Data from register docs? */

        /**
         * @brief Get FrontPanelIOControl mask
         *
         * This register manages the front panel I/O connectors.
         *
         * Get the low-level FrontPanelIOControl mask in line with
         * register docs. It is recommended to use the EasyX wrapper
         * version instead.
         *
         * @returns
         * 32-bit mask with layout described in register docs
         */
        uint32_t getFrontPanelIOControl() override
        {
            uint32_t mask;
            errorHandler(CAEN_DGTZ_ReadRegister(handle_, 0x811C, &mask));
            return mask;
        }
        /**
         * @brief Set FrontPanelIOControl mask
         *
         * This register manages the front panel I/O connectors.
         *
         * Set the low-level FrontPanelIOControl mask in line with
         * register docs. It is recommended to use the EasyX wrapper
         * version instead.
         *
         * @param mask:
         * 32-bit mask with layout described in register docs
         */
        void setFrontPanelIOControl(uint32_t mask) override
        { errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x811C, mask)); }
        /**
         * @brief Easy Get FrontPanelIOControl
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating from the bit mask returned by the
         * the underlying low-level get funtion.
         *
         * @returns
         * EasyFrontPanelIOControl object
         */
        EasyFrontPanelIOControl getEasyFrontPanelIOControl() override
        {
            uint32_t mask;
            mask = getFrontPanelIOControl();
            return EasyFrontPanelIOControl(mask);
        }
        /**
         * @brief Easy Set FrontPanelIOControl
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating to the bit mask needed by the
         * the underlying low-level set funtion.
         *
         * @param settings:
         * EasyFrontPanelIOControl object
         */
        void setEasyFrontPanelIOControl(EasyFrontPanelIOControl settings) override
        {
            uint32_t mask = settings.toBits();
            setFrontPanelIOControl(mask);
        }

        /* NOTE: Group Enable Mask from register is handled by GroupEnableMask */

        /**
         * @brief Get ROCFPGAFirmwareRevision mask
         *
         * This register contains the motherboard FPGA (ROC) firmware
         * revision information.\n
         * The complete format is:\n
         * Firmware Revision = X.Y (16 lower bits)\n
         * Firmware Revision Date = Y/M/DD (16 higher bits)\n
         * EXAMPLE 1: revision 3.08, November 12th, 2007 is 0x7B120308.\n
         * EXAMPLE 2: revision 4.09, March 7th, 2016 is 0x03070409.\n
         * NOTE: the nibble code for the year makes this information to
         * roll over each 16 years.
         *
         * Get the low-level ROCFPGAFirmwareRevision mask in line with
         * register docs. It is recommended to use the EasyX wrapper
         * version instead.
         *
         * @returns
         * 32-bit mask with layout described in register docs
         */
        uint32_t getROCFPGAFirmwareRevision() override
        {
            uint32_t mask;
            errorHandler(CAEN_DGTZ_ReadRegister(handle_, 0x8124, &mask));
            return mask;
        }
        /**
         * @brief Easy Get ROCFPGAFirmwareRevision
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating from the bit mask returned by the
         * the underlying low-level get funtion.
         *
         * @returns
         * EasyROCFPGAFirmwareRevision structure
         */
        EasyROCFPGAFirmwareRevision getEasyROCFPGAFirmwareRevision() override
        {
            uint32_t mask;
            mask = getROCFPGAFirmwareRevision();
            return EasyROCFPGAFirmwareRevision(mask);
        }

        /* TODO: wrap Software Clock Sync from register docs? */

        /* TODO: handle Board Info from register docs? looks slightly
         * different from GetInfo. */

        /* TODO: wrap Monitor DAC Mode from register docs? */

        /**
         * @brief Get EventSize
         *
         * This register contains the current available event size in
         * 32-bit words. The value is updated a er a complete readout of
         * each event.
         *
         * Get the low-level EventSize in line with register docs.
         *
         * @returns
         * Event Size (32-bit words).
         */
        uint32_t getEventSize() override
        {
            uint32_t value;
            errorHandler(CAEN_DGTZ_ReadRegister(handle_, 0x814C, &value));
            return value;
        }

        /**
         * @brief Get FanSpeedControl mask
         *
         * This register manages the on-board fan speed in order to
         * guarantee an appropriate cooling according to the internal
         * temperature variations.\n
         * NOTE: from revision 4 of the motherboard PCB (see register
         * 0xF04C of the Configuration ROM), the automatic fan speed
         * control has been implemented, and it is supported by ROC FPGA
         * firmware revision greater than 4.4 (see register 0x8124).\n
         * Independently of the revision, the user can set the fan speed
         * high by setting bit[3] = 1. Setting bit[3] = 0 will restore
         * the automatic control for revision 4 or higher, or the low
         * fan speed in case of revisions lower than 4.\n
         * NOTE: this register is supported by Desktop (DT) boards only.
         *
         * Get the low-level FanSpeedControl mask in line with
         * register docs. It is recommended to use the EasyX wrapper
         * version instead.
         *
         * @returns
         * 32-bit mask with layout described in register docs
         */
        uint32_t getFanSpeedControl() override
        {
            uint32_t mask;
            errorHandler(CAEN_DGTZ_ReadRegister(handle_, 0x8168, &mask));
            return mask;
        }
        /**
         * @brief Set FanSpeedControl mask
         *
         * This register manages the on-board fan speed in order to
         * guarantee an appropriate cooling according to the internal
         * temperature variations.\n
         * NOTE: from revision 4 of the motherboard PCB (see register
         * 0xF04C of the Configuration ROM), the automatic fan speed
         * control has been implemented, and it is supported by ROC FPGA
         * firmware revision greater than 4.4 (see register 0x8124).\n
         * Independently of the revision, the user can set the fan speed
         * high by setting bit[3] = 1. Setting bit[3] = 0 will restore
         * the automatic control for revision 4 or higher, or the low
         * fan speed in case of revisions lower than 4.\n
         * NOTE: this register is supported by Desktop (DT) boards only.
         *
         * Set the low-level FanSpeedControl mask in line with
         * register docs. It is recommended to use the EasyX wrapper
         * version instead.
         *
         * @param mask:
         * 32-bit mask with layout described in register docs
         */
        void setFanSpeedControl(uint32_t mask) override
        { errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x8168, mask)); }
        /**
         * @brief Easy Get FanSpeedControl
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating from the bit mask returned by the
         * the underlying low-level get funtion.
         *
         * @returns
         * EasyFanSpeedControl object
         */
        EasyFanSpeedControl getEasyFanSpeedControl() override
        {
            uint32_t mask;
            mask = getFanSpeedControl();
            return EasyFanSpeedControl(mask);
        }
        /**
         * @brief Easy Set FanSpeedControl
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating to the bit mask needed by the
         * the underlying low-level set funtion.
         *
         * @param settings:
         * EasyFanSpeedControl object
         */
        void setEasyFanSpeedControl(EasyFanSpeedControl settings) override
        {
            uint32_t mask = settings.toBits();
            setFanSpeedControl(mask);
        }


        /**
         * @brief Get Run/Start/Stop Delay
         *
         * When the start of Run is given synchronously to several
         * boards connected in Daisy chain, it is necessary to
         * compensate for the delay in the propagation of the Start (or
         * Stop) signal through the chain. This register sets the delay,
         * expressed in trigger clock cycles between the arrival of the
         * Start signal at the input of the board (either on S-IN/GPI or
         * TRG-IN) and the actual start of Run. The delay is usually
         * zero for the last board in the chain and rises going
         * backwards along the chain.
         *
         * Get the low-level Run/Start/Stop Delay in line with
         * register docs.
         *
         * @returns
         * Delay (in units of 8 ns).
         */
        uint32_t getRunStartStopDelay() override
        { uint32_t delay; errorHandler(CAEN_DGTZ_ReadRegister(handle_, 0x8170, &delay)); return delay; }
        /**
         * @brief Set Run/Start/Stop Delay
         *
         * When the start of Run is given synchronously to several
         * boards connected in Daisy chain, it is necessary to
         * compensate for the delay in the propagation of the Start (or
         * Stop) signal through the chain. This register sets the delay,
         * expressed in trigger clock cycles between the arrival of the
         * Start signal at the input of the board (either on S-IN/GPI or
         * TRG-IN) and the actual start of Run. The delay is usually
         * zero for the last board in the chain and rises going
         * backwards along the chain.
         *
         * Set the low-level Run/Start/Stop Delay in line with
         * register docs.
         *
         * @param delay:
         * Delay (in units of 8 ns).
         */
        virtual void setRunStartStopDelay(uint32_t delay) override
        { errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x8170, delay)); }

        /* NOTE: map these legacy methods to get / setRunStartStopDelay */
        virtual uint32_t getRunDelay()
        { return getRunStartStopDelay(); };
        virtual void setRunDelay(uint32_t delay)
        { return setRunStartStopDelay(delay); };

        /* TODO: wrap Board Failure Status from register docs? */

        /* TODO: wrap Front Panel LVDS I/O New Features from register docs? */

        /* TODO: wrap Buffer Occupancy Gain from register docs? */

        /**
         * @brief Get ReadoutControl mask
         *
         * This register is mainly intended for VME boards, anyway some
         * bits are applicable also for DT and NIM boards.
         *
         * Get the low-level ReadoutControl mask in line with
         * register docs. It is recommended to use the EasyX wrapper
         * version instead.
         *
         * @returns
         * 32-bit mask with layout described in register docs
         */
        uint32_t getReadoutControl() override
        {
            uint32_t mask;
            errorHandler(CAEN_DGTZ_ReadRegister(handle_, 0xEF00, &mask));
            return mask;
        }
        /**
         * @brief Set ReadoutControl mask
         *
         * This register is mainly intended for VME boards, anyway some
         * bits are applicable also for DT and NIM boards.
         *
         * Set the low-level ReadoutControl mask in line with
         * register docs. It is recommended to use the EasyX wrapper
         * version instead.
         *
         * @param mask:
         * 32-bit mask with layout described in register docs
         */
        void setReadoutControl(uint32_t mask) override
        { errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0xEF00, mask)); }
        /**
         * @brief Easy Get ReadoutControl
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating from the bit mask returned by the
         * the underlying low-level get funtion.
         *
         * @returns
         * EasyReadoutControl object
         */
        EasyReadoutControl getEasyReadoutControl() override
        {
            uint32_t mask;
            mask = getReadoutControl();
            return EasyReadoutControl(mask);
        }
        /**
         * @brief Easy Set ReadoutControl
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating to the bit mask needed by the
         * the underlying low-level set funtion.
         *
         * @param settings:
         * EasyReadoutControl object
         */
        void setEasyReadoutControl(EasyReadoutControl settings) override
        {
            uint32_t mask = settings.toBits();
            setReadoutControl(mask);
        }

        /**
         * @brief Get ReadoutStatus mask
         *
         * This register is mainly intended for VME boards, anyway some
         * bits are applicable also for DT and NIM boards.
         *
         * Get the low-level ReadoutStatus mask in line with
         * register docs. It is recommended to use the EasyX wrapper
         * version instead.
         *
         * @returns
         * 32-bit mask with layout described in register docs
         */
        uint32_t getReadoutStatus() override
        {
            uint32_t mask;
            errorHandler(CAEN_DGTZ_ReadRegister(handle_, 0xEF04, &mask));
            return mask;
        }
        /**
         * @brief Easy Get ReadoutStatus
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating from the bit mask returned by the
         * the underlying low-level get funtion.
         *
         * @returns
         * EasyReadoutStatus object
         */
        virtual EasyReadoutStatus getEasyReadoutStatus() override
        {
            uint32_t mask;
            mask = getReadoutStatus();
            return EasyReadoutStatus(mask);
        }

        /* TODO: wrap Board ID from register docs? */
        /* TODO: wrap MCST Base Address and Control from register docs? */
        /* TODO: wrap Relocation Address from register docs? */
        /* TODO: wrap Interrupt Status/ID from register docs? */
        /* TODO: wrap Interrupt Event Number from register docs? */

        /**
         * @brief Get Scratch mask
         *
         * This register is mainly intended for VME boards, anyway some
         * bits are applicable also for DT and NIM boards.
         *
         * Get the low-level Scratch mask in line with
         * register docs. It is recommended to use the EasyX wrapper
         * version instead.
         *
         * @returns
         * 32-bit mask with layout described in register docs
         */
        uint32_t getScratch() override
        {
            uint32_t mask;
            errorHandler(CAEN_DGTZ_ReadRegister(handle_, 0xEF20, &mask));
            return mask;
        }
        /**
         * @brief Set Scratch mask
         *
         * This register is mainly intended for VME boards, anyway some
         * bits are applicable also for DT and NIM boards.
         *
         * Set the low-level Scratch mask in line with
         * register docs. It is recommended to use the EasyX wrapper
         * version instead.
         *
         * @param mask:
         * 32-bit mask with layout described in register docs
         */
        void setScratch(uint32_t mask) override
        { errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0xEF20, mask)); }
        /**
         * @brief Easy Get Scratch
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating from the bit mask returned by the
         * the underlying low-level get funtion.
         *
         * @returns
         * EasyScratch object
         */
        EasyScratch getEasyScratch() override
        {
            uint32_t mask;
            mask = getScratch();
            return EasyScratch(mask);
        }
        /**
         * @brief Easy Set Scratch
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating to the bit mask needed by the
         * the underlying low-level set funtion.
         *
         * @param settings:
         * EasyScratch object
         */
        void setEasyScratch(EasyScratch settings) override
        {
            uint32_t mask = settings.toBits();
            setScratch(mask);
        }

        /* TODO: wrap Software Reset from register docs? */
        /* TODO: wrap Software Clear from register docs? */
        /* TODO: wrap Configuration Reload from register docs? */

        /* TODO: wrap Configuration ROM Checksum from register docs? */
        /* TODO: wrap Configuration ROM Checksum Length BYTE 0 1 2  from register docs? */
        /* TODO: wrap Configuration ROM Constant BYTE 0 1 2 from register docs? */
        /* TODO: wrap Configuration ROM C R Code from register docs? */
        /* TODO: wrap Configuration ROM IEEE OUI BYTE 0 1 2 from register docs? */
        /* TODO: wrap Configuration ROM Board Version from register docs? */
        /* TODO: wrap Configuration ROM Board Form Factor from register docs? */
        /* TODO: wrap Configuration ROM Board ID BYTE 0 1 from register docs */
        /* TODO: wrap Configuration ROM PCB Revision BYTE 0 1 2 3 from register docs? */
        /* TODO: wrap Configuration ROM FLASH Type from register docs? */
        /* TODO: wrap Configuration ROM Board Serial Number BYTE 0 1 from register docs? */
        /* TODO: wrap Configuration ROM VCXO Type from register docs? */

    };

    class Digitizer740DPP : public Digitizer740 {
    private:
        Digitizer740DPP();
        friend Digitizer *
        Digitizer::open(CAEN_DGTZ_ConnectionType linkType, int linkNum, int conetNode, uint32_t VMEBaseAddress);

    protected:
        Digitizer740DPP(int handle, CAEN_DGTZ_BoardInfo_t boardInfo) : Digitizer740(handle, boardInfo) {}

    public:

        /* According to register docs the bits [0:3,5:7,9:11,14:15,22:31]
         * must be 0 and the bits [4,8,18,19] must be 1 so we always
         * force compliance by a bitwise-or with 0x000C0110 followed
         * by a bitwise-and with 0x003F3110 for the set operation.
         * Similarly we prevent mangling of the force ones by a
         * bitwise-and with the xor inverted version of 0x000C0110 for
         * the unset operation.
         */
        virtual uint32_t filterBoardConfigurationSetMask(uint32_t mask) override
        { return ((mask | 0x000C0110) & 0x003F3110); }
        virtual uint32_t filterBoardConfigurationUnsetMask(uint32_t mask) override
        { return (mask & (0xFFFFFFFF ^ 0x000C0110)); }

        /**
         * @brief Get DPP GateWidth
         *
         * Sets the Gate width for the charge integration used in the
         * energy spectra calculation.
         *
         * @param group:
         * channel group index
         * @returns
         * Number of samples for the Gate width. Each sample
         * corresponds to 16 ns - 12 bits
         */
        uint32_t getDPPGateWidth(uint32_t group) override
        {
            if (group >= groups())
                errorHandler(CAEN_DGTZ_InvalidChannelNumber);
            uint32_t value;
            errorHandler(CAEN_DGTZ_ReadRegister(handle_, 0x1030 | group<<8 , &value));
            return value;
        }
        /**
         * @brief Set DPP GateWidth
         *
         * Sets the Gate width for the charge integration used in the
         * energy spectra calculation.
         *
         * @param group:
         * optional channel group index
         * @param value:
         * Number of samples for the Gate width. Each sample
         * corresponds to 16 ns - 12 bits
         */
        void setDPPGateWidth(uint32_t group, uint32_t value) override
        {
            if (group >= groups())
                errorHandler(CAEN_DGTZ_InvalidChannelNumber);
            errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x1030 | group<<8, value & 0xFFF));
        }
        /**
         * @brief Broadcast version: please refer to details in
         * single-group version.
         */
        void setDPPGateWidth(uint32_t value) override
        { errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x8030, value & 0xFFF)); }

        /**
         * @brief Get DPP GateOffset
         *
         * Corresponds to the shift in time of the integration gate
         * position with respect to the trigger.
         *
         * @param group:
         * channel group index
         * @returns
         * Number of samples for the Gate Offset width. Each
         * sample corresponds to 16 ns. - 12 bits
         */
        uint32_t getDPPGateOffset(uint32_t group) override
        {
            if (group >= groups())
                errorHandler(CAEN_DGTZ_InvalidChannelNumber);
            uint32_t value;
            errorHandler(CAEN_DGTZ_ReadRegister(handle_, 0x1034 | group<<8 , &value));
            return value;
        }
        /**
         * @brief Set DPP GateOffset
         *
         * Corresponds to the shift in time of the integration gate
         * position with respect to the trigger.
         *
         * @param group:
         * optional channel group index
         * @param value:
         * Number of samples for the Gate Offset width. Each sample
         * corresponds to 16 ns. - 12 bits
         */
        void setDPPGateOffset(uint32_t group, uint32_t value) override
        {
            if (group >= groups())
                errorHandler(CAEN_DGTZ_InvalidChannelNumber);
            errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x1034 | group<<8, value & 0xFFF));
        }
        /**
         * @brief Broadcast version: please refer to details in
         * single-group version.
         */
        void setDPPGateOffset(uint32_t value) override
        { errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x8034, value & 0xFFF)); }

        /**
         * @brief Get DPP FixedBaseline
         *
         * The baseline calculation can be performed either dynamically
         * or statically. In the first case the user can set the samples
         * of the moving average window through register 0x1n40. In the
         * latter case the user must disable the automatic baseline
         * calculation through bits[22:20] of register 0x1n40 and set
         * the desired value of fixed baseline through this
         * register. The baseline value then remains constant for the
         * whole acquisition.\n
         * Note: This register is ignored in case of dynamic calculation.
         *
         * @param group:
         * channel group index
         * @returns
         * Value of Fixed Baseline in LSB counts - 12 bits
         */
        uint32_t getDPPFixedBaseline(uint32_t group) override
        {
            if (group >= groups())
                errorHandler(CAEN_DGTZ_InvalidChannelNumber);
            uint32_t value;
            errorHandler(CAEN_DGTZ_ReadRegister(handle_, 0x1038 | group<<8 , &value));
            return value;
        }
        /**
         * @brief Set DPP FixedBaseline
         *
         * The baseline calculation can be performed either dynamically
         * or statically. In the first case the user can set the samples
         * of the moving average window through register 0x1n40. In the
         * latter case the user must disable the automatic baseline
         * calculation through bits[22:20] of register 0x1n40 and set
         * the desired value of fixed baseline through this
         * register. The baseline value then remains constant for the
         * whole acquisition.\n
         * Note: This register is ignored in case of dynamic calculation.
         *
         * @param group:
         * optional channel group index
         * @param value:
         * Value of Fixed Baseline in LSB counts - 12 bits
         */
        void setDPPFixedBaseline(uint32_t group, uint32_t value) override
        {
            if (group >= groups())
                errorHandler(CAEN_DGTZ_InvalidChannelNumber);
            errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x1038 | group<<8, value & 0xFFF));
        }
        /**
         * @brief Broadcast version: please refer to details in
         * single-group version.
         */
        void setDPPFixedBaseline(uint32_t value) override
        { errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x8038, value & 0xFFF)); }

        /* TODO: switch DPPPreTriggerSize to use native CAENDigitizer functions? */
        /* NOTE: we can't rename DPPPreTriggerSize to fit DPP register
         * doc Pre Trigger since it is a general Digitizer method. */

        /**
         * @brief Get DPPPreTriggerSize
         *
         * The Pre Trigger defines the number of samples before the
         * trigger in the waveform saved into memory.
         *
         * @param group:
         * channel group index
         * @returns
         * Number of samples Ns of the Pre Trigger width. The value is
         * expressed in steps of sampling frequency (16 ns).\n
         * NOTE: the Pre Trigger value must be greater than the Gate
         * Offset value by at least 112 ns.
         */
        uint32_t getDPPPreTriggerSize(int group) override
        {
            if (group >= groups() || group < 0)
                errorHandler(CAEN_DGTZ_InvalidChannelNumber);
            uint32_t samples;
            errorHandler(CAEN_DGTZ_ReadRegister(handle_, 0x103C | group<<8 , &samples));
            return samples;
        }
        /**
         * @brief Set DPPPreTriggerSize
         *
         * The Pre Trigger defines the number of samples before the
         * trigger in the waveform saved into memory.
         *
         * @param group:
         * optional channel group index
         * @param samples:
         * Number of samples Ns of the Pre Trigger width. The value is
         * expressed in steps of sampling frequency (16 ns).\n
         * NOTE: the Pre Trigger value must be greater than the Gate
         * Offset value by at least 112 ns.
         */
        void setDPPPreTriggerSize(int group, uint32_t samples) override
        {
            if (group >= groups() || group < 0)
                errorHandler(CAEN_DGTZ_InvalidChannelNumber);
            { errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x103C | group<<8, samples & 0xFFF)); }
        }
        /**
         * @brief Broadcast version: please refer to details in
         * single-group version.
         */
        void setDPPPreTriggerSize(uint32_t samples) override
        { errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x803C, samples & 0xFFF)); }

        /**
         * @brief Get DPPAlgorithmControl
         *
         * Management of the DPP algorithm features.
         *
         * Get the low-level DPPAlgorithmControl mask in line with
         * register docs. It is recommended to use the EasyX wrapper
         * version instead.
         *
         * @returns
         * 32-bit mask with layout described in register docs
         */
        uint32_t getDPPAlgorithmControl(uint32_t group) override
        {
            if (group >= groups())
                errorHandler(CAEN_DGTZ_InvalidChannelNumber);
            uint32_t mask;
            errorHandler(CAEN_DGTZ_ReadRegister(handle_, 0x1040 | group<<8 , &mask));
            return mask;
        }
        /**
         * @brief Set DPPAlgorithmControl mask
         *
         * Management of the DPP algorithm features.
         *
         * Set the low-level DPPAlgorithmControl mask in line with
         * register docs. It is recommended to use the EasyX wrapper
         * version instead.
         *
         * @param group:
         * optional channel group index
         * @param mask:
         * 32-bit mask with layout described in register docs
         */
        void setDPPAlgorithmControl(uint32_t group, uint32_t mask) override
        {
            if (group >= groups())
                errorHandler(CAEN_DGTZ_InvalidChannelNumber);
            errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x1040 | group<<8, mask));
        }
        /**
         * @brief Broadcast version: please refer to details in
         * single-group version.
         */
        void setDPPAlgorithmControl(uint32_t mask) override
        { errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x8040, mask)); }
        /**
         * @brief Easy Get DPPAlgorithmControl
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating from the bit mask returned by the
         * the underlying low-level get funtion.
         *
         * @returns
         * EasyDPPAlgorithmControl object
         */
        EasyDPPAlgorithmControl getEasyDPPAlgorithmControl(uint32_t group) override
        {
            uint32_t mask = getDPPAlgorithmControl(group);
            return EasyDPPAlgorithmControl(mask);
        }
        /**
         * @brief Easy Set DPPAlgorithmControl
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating to the bit mask needed by the
         * the underlying low-level set funtion.
         *
         * @param settings:
         * EasyDPPAlgorithmControl object
         */
        void setEasyDPPAlgorithmControl(uint32_t group, EasyDPPAlgorithmControl settings) override
        {
            uint32_t mask = settings.toBits();
            setDPPAlgorithmControl(group, mask);
        }
        /**
         * @brief Broadcast version: please refer to details in
         * single-group version.
         */
        void setEasyDPPAlgorithmControl(EasyDPPAlgorithmControl settings) override
        {
            uint32_t mask = settings.toBits();
            setDPPAlgorithmControl(mask);
        }

        /**
         * @brief Get DPP TriggerHoldOffWidth
         *
         * The Trigger Hold-Off is a logic signal of programmable width
         * generated by a channel in correspondence with its local
         * self-trigger. Other triggers are inhibited for the overall
         * Trigger Hold-Off duration. 
         *
         * @param group:
         * channel group index
         * @returns
         * The Trigger Hold-Off width in steps of 16 ns - 16 bits
         */
        uint32_t getDPPTriggerHoldOffWidth(uint32_t group) override
        {
            if (group >= groups())
                errorHandler(CAEN_DGTZ_InvalidChannelNumber);
            uint32_t value;
            errorHandler(CAEN_DGTZ_ReadRegister(handle_, 0x1074 | group<<8 , &value));
            return value;
        }
        /**
         * @brief Broadcast version: please refer to details in
         * single-group version.
         */
        uint32_t getDPPTriggerHoldOffWidth() override
        {
            uint32_t value;
            errorHandler(CAEN_DGTZ_ReadRegister(handle_, 0x8074, &value));
            return value;
        }
        /**
         * @brief Set DPP TriggerHoldOffWidth
         *
         * The Trigger Hold-Off is a logic signal of programmable width
         * generated by a channel in correspondence with its local
         * self-trigger. Other triggers are inhibited for the overall
         * Trigger Hold-Off duration. 
         *
         * @param group:
         * optional channel group index
         * @param value:
         * The Trigger Hold-Off width in steps of 16 ns - 16 bits
         */
        void setDPPTriggerHoldOffWidth(uint32_t group, uint32_t value) override
        {
            if (group >= groups())
                errorHandler(CAEN_DGTZ_InvalidChannelNumber);
            errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x1074 | group<<8, value & 0xFFFF));
        }
        /**
         * @brief Broadcast version: please refer to details in
         * single-group version.
         */
        void setDPPTriggerHoldOffWidth(uint32_t value) override
        { errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x8074, value & 0xFFFF)); }

        /**
         * @brief Get DPP ShapedTriggerWidth
         *
         * The Shaped Trigger is a logic signal of programmable width
         * generated by a channel in correspondence to its local
         * self-trigger. It is used to propagate the trigger to the
         * other channels of the board and to other external boards, as
         * well as to feed the coincidence trigger logic.
         *
         * @param group:
         * channel group index
         * @returns
         * The number of samples for the Shaped Trigger width in trigger
         * clock cycles (16 ns step) - 16 bits
         *
         * @bug
         * CAEN_DGTZ_ReadRegister 0x1078 for ShapedTriggerWidth on V1740D
         * causes CommError 
         */
        /* TODO: figure out why ShapedTriggerWidth from 740 DPP register
           docs fails with commerr. Disabled for now to avoid problems. */
        /*
        uint32_t getDPPShapedTriggerWidth(uint32_t group) override
        {
            if (group >= groups())
                errorHandler(CAEN_DGTZ_InvalidChannelNumber);
            uint32_t value;
            errorHandler(CAEN_DGTZ_ReadRegister(handle_, 0x1078 | group<<8 , &value));
            return value;
        }
        */
        /**
         * @brief Broadcast version: please refer to details in
         * single-group version.
         */
        /*
        uint32_t getDPPShapedTriggerWidth() override
        {
            uint32_t value;
            errorHandler(CAEN_DGTZ_ReadRegister(handle_, 0x8078, &value));
            return value;
        }
        */
        /**
         * @brief Set DPP ShapedTriggerWidth
         *
         * The Shaped Trigger is a logic signal of programmable width
         * generated by a channel in correspondence to its local
         * self-trigger. It is used to propagate the trigger to the
         * other channels of the board and to other external boards, as
         * well as to feed the coincidence trigger logic.
         *
         * @param group:
         * optional channel group index
         * @param value:
         * Set the number of samples for the Shaped Trigger width in
         * trigger clock cycles (16 ns step) - 16 bits
         */
        /*
        void setDPPShapedTriggerWidth(uint32_t group, uint32_t value) override
        {
            if (group >= groups())
                errorHandler(CAEN_DGTZ_InvalidChannelNumber);
            errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x1078 | group<<8, value & 0xFFFF));
        }
        */
        /**
         * @brief Broadcast version: please refer to details in
         * single-group version.
         */
        /*
        void setDPPShapedTriggerWidth(uint32_t value) override
        { errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x8078, value & 0xFFFF)); }
        */

        /* NOTE: reuse get AMCFirmwareRevision from parent */
        /* NOTE: disable inherited 740 EasyAMCFirmwareRevision since we only
         * support EasyDPPAMCFirmwareRevision here. */
        /**
         * @brief use getEasyDPPX version instead
         */
        virtual EasyAMCFirmwareRevision getEasyAMCFirmwareRevision(uint32_t group) override { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        /**
         * @brief Easy Get DPP AMCFirmwareRevision
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating from the bit mask returned by the
         * the underlying low-level get funtion.
         *
         * @param group:
         * group index
         * @returns
         * EasyAMCFirmwareRevision object
         */
        EasyDPPAMCFirmwareRevision getEasyDPPAMCFirmwareRevision(uint32_t group) override
        {
            uint32_t mask;
            mask = getAMCFirmwareRevision(group);
            return EasyDPPAMCFirmwareRevision(mask);
        }

        /* TODO: Switch to bitmasks read-in and write-out in struct confs?
         *       ... preferably using the bit length of the field.
         *       Can we support more user-friendly aliases?
         *         EasyX[enableTrigger] = 1
         *         EasyX[Field1,..,FieldN] = {1,..,1}
         *       ... must be parsed into a single set operation if so.
         */

        /* TODO: wrap Individual Trigger Threshold of Group n Sub Channel m from register docs? */

        /* NOTE: reuse get / set BoardConfiguration from parent */
        /* NOTE: disable inherited 740 EasyBoardConfiguration since we only
         * support EasyDPPBoardConfiguration here. */
        /**
         * @brief use getEasyDPPX version instead
         */
        virtual EasyBoardConfiguration getEasyBoardConfiguration() override { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        /**
         * @brief use setEasyDPPX version instead
         */
        virtual void setEasyBoardConfiguration(EasyBoardConfiguration settings) override { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        /**
         * @brief Easy Get DPP BoardConfiguration
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating from the bit mask returned by the
         * the underlying low-level get funtion.
         *
         * @returns
         * EasyDPPBoardConfiguration object
         */
        EasyDPPBoardConfiguration getEasyDPPBoardConfiguration() override
        {
            uint32_t mask;
            mask = getBoardConfiguration();
            return EasyDPPBoardConfiguration(mask);
        }
        /**
         * @brief Easy Set DPP BoardConfiguration
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating to the bit mask needed by the
         * the underlying low-level set funtion.
         *
         * IMPORTANT: this version takes care of both setting and unsetting
         * bits, unlike the low-level set and unset versions.
         *
         * @param settings:
         * EasyDPPBoardConfiguration object
         */
        void setEasyDPPBoardConfiguration(EasyDPPBoardConfiguration settings) override
        {
            /* NOTE: we explicitly unset all bits first since set
             * only enables bits */
            uint32_t mask = settings.toBits();
            unsetBoardConfiguration(0xFFFFFFFF);
            setBoardConfiguration(mask);
        }

        /* TODO: delete AggregateOrganization wrappers? */
        /* NOTE: Event Aggregation is already covered by 
         *       get/setNumEventsPerAggregate
         */
        /**
         * @brief Get DPP AggregateOrganization
         *
         * The internal memory of the digitizer can be divided into a
         * programmable number of aggregates, where each aggregate
         * contains a specific number of events. This register defines
         * how many aggregates can be contained in the memory.\n
         * Note: this register must not be modified while the
         * acquisition is running.
         *
         * @returns
         * Aggregate Organization. Nb: the number of aggregates is equal
         * to N_aggr = 2^Nb . Please refer to register doc for values -
         * 4 bits
         */
        /*
        uint32_t getDPPAggregateOrganization() override
        {
            uint32_t value;
            errorHandler(CAEN_DGTZ_ReadRegister(handle_, 0x800C, &value));
            return value;
        }
        */
        /**
         * @brief Set DPP AggregateOrganization
         *
         * The internal memory of the digitizer can be divided into a
         * programmable number of aggregates, where each aggregate
         * contains a specific number of events. This register defines
         * how many aggregates can be contained in the memory.\n
         * Note: this register must not be modified while the
         * acquisition is running.
         *
         * @param value:
         * Aggregate Organization. Nb: the number of aggregates is equal
         * to N_aggr = 2^Nb . Please refer to register doc for values -
         * 4 bits
         */
        /*
        void setDPPAggregateOrganization(uint32_t value) override
        { errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x800C, value & 0x0F)); }
        */
        /* 
        uint32_t getEventsPerAggregate() override
        {
            uint32_t value;
            errorHandler(CAEN_DGTZ_ReadRegister(handle_, 0x8020, &value));
            return value;
        }
        void setEventsPerAggregate(uint32_t value) override
        { errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x8020, value & 0x07FF)); }
        */

        /* TODO: what to do about these custom functions - now that they
         * are exposed through BoardConfiguration helpers? */
        /**
         * @brief Get DPPAcquisitionMode
         * @returns
         * DPP acquisition mode flag - 4 bits
         *
         * @internal
         * NOTE: According to the CAEN DPP register docs the bits
         * [18:19] should always be 1, and in CAENDigitizer docs it
         * sounds like setDPPAcquisitionMode with the only valid modes
         * there (Mixed or List) should both set them accordingly, but
         * apparently it doesn't really happen on V1740D with DPP
         * firmware. Reported upstream.\n
         */
        /* TODO: switch to the CAEN Get / SetDPPAcquisitionMode when fixed?
         * TODO: switch to get / set BoardConfiguration internally?
         */
        DPPAcquisitionMode getDPPAcquisitionMode() override
        {
            uint32_t boardConf;
            errorHandler(CAEN_DGTZ_ReadRegister(handle_, 0x8000 , &boardConf));
            DPPAcquisitionMode mode;
            if (boardConf & 1<<16)
                mode.mode = CAEN_DGTZ_DPP_ACQ_MODE_Mixed;
            else
                mode.mode = CAEN_DGTZ_DPP_ACQ_MODE_List;
            switch ((boardConf & 3<<18)>>18)
            {
                case 0 : mode.param = CAEN_DGTZ_DPP_SAVE_PARAM_None; break;
                case 1 : mode.param = CAEN_DGTZ_DPP_SAVE_PARAM_TimeOnly; break;
                case 2 : mode.param = CAEN_DGTZ_DPP_SAVE_PARAM_EnergyOnly; break;
                case 3 : mode.param = CAEN_DGTZ_DPP_SAVE_PARAM_EnergyAndTime; break;
                default: assert(false);
            }
            return mode;
        }
        /**
         * @brief Set DPPAcquisitionMode
         * @param mode
         * Set DPP acquisition mode flag - 4 bits
         */
        void setDPPAcquisitionMode(DPPAcquisitionMode mode) override
        {
            // Completely ignore mode.param: CAEN documentation does not match reality
            //if (mode.param != CAEN_DGTZ_DPP_SAVE_PARAM_EnergyAndTime)
            //    errorHandler(CAEN_DGTZ_InvalidParam);
            switch (mode.mode)
            {
                case CAEN_DGTZ_DPP_ACQ_MODE_List:
                    CAEN_DGTZ_WriteRegister(handle_, 0x8008, 1<<16); // bit clear
                    break;
                case CAEN_DGTZ_DPP_ACQ_MODE_Mixed:
                    CAEN_DGTZ_WriteRegister(handle_, 0x8004, 1<<16); // bit set
                    break;
                default:
                    errorHandler(CAEN_DGTZ_InvalidParam);
            }
        }

        /* NOTE: reuse get / set AcquisitionControl from parent class */
        /* NOTE: disable inherited 740 EasyAcquisitionControl since we only
         * support EasyDPPAcquisitionControl here. */
        /**
         * @brief use getEasyDPPX version instead
         */
        virtual EasyAcquisitionControl getEasyAcquisitionControl() override { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        /**
         * @brief use setEasyDPPX version instead
         */
        virtual void setEasyAcquisitionControl(EasyAcquisitionControl settings) override { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        /**
         * @brief Easy Get DPP AcquisitionControl
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating from the bit mask returned by the
         * the underlying low-level get funtion.
         *
         * @returns
         * EasyDPPAcquisitionControl object
         */
        EasyDPPAcquisitionControl getEasyDPPAcquisitionControl() override
        {
            uint32_t mask;
            mask = getAcquisitionControl();
            return EasyDPPAcquisitionControl(mask);
        }
        /**
         * @brief Easy Set DPP AcquisitionControl
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating to the bit mask needed by the
         * the underlying low-level set funtion.
         *
         * @param settings:
         * EasyDPPAcquisitionControl object
         */
        void setEasyDPPAcquisitionControl(EasyDPPAcquisitionControl settings) override
        {
            uint32_t mask = settings.toBits();
            setAcquisitionControl(mask);
        }

        /* NOTE: reuse get / set AcquisitionStatus from parent class */
        /* NOTE: disable inherited 740 EasyAcquisitionStatus since we only
         * support EasyDPPAcquisitionStatus here. */
        /**
         * @brief use getEasyDPPX version instead
         */
        virtual EasyAcquisitionStatus getEasyAcquisitionStatus() override { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        /**
         * @brief Easy Get DPP AcquisitionStatus
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating from the bit mask returned by the
         * the underlying low-level get funtion.
         *
         * @returns
         * EasyDPPAcquisitionStatus object
         */
        EasyDPPAcquisitionStatus getEasyDPPAcquisitionStatus() override
        {
            uint32_t mask;
            mask = getAcquisitionStatus();
            return EasyDPPAcquisitionStatus(mask);
        }

        /* NOTE: Reuse get / set SoftwareTrigger from parent? */

        /* NOTE: Reuse get / set GlobalTriggerMask from parent */
        /* NOTE: disable inherited 740 EasyGlobalTriggerMask since we only
         * support EasyDPPGlobalTriggerMask here. */
        /**
         * @brief use getEasyDPPX version instead
         */
        virtual EasyGlobalTriggerMask getEasyGlobalTriggerMask() override { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        /**
         * @brief use setEasyDPPX version instead
         */
        virtual void setEasyGlobalTriggerMask(EasyGlobalTriggerMask settings) override { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        /**
         * @brief Easy Get DPP GlobalTriggerMask
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating from the bit mask returned by the
         * the underlying low-level get funtion.
         *
         * @returns
         * EasyDPPGlobalTriggerMask object
         */
        EasyDPPGlobalTriggerMask getEasyDPPGlobalTriggerMask() override
        {
            uint32_t mask;
            mask = getGlobalTriggerMask();
            return EasyDPPGlobalTriggerMask(mask);
        }
        /**
         * @brief Easy Set DPP GlobalTriggerMask
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating to the bit mask needed by the
         * the underlying low-level set funtion.
         *
         * @param settings:
         * EasyDPPGlobalTriggerMask object
         */
        void setEasyDPPGlobalTriggerMask(EasyDPPGlobalTriggerMask settings) override
        {
            uint32_t mask = settings.toBits();
            setGlobalTriggerMask(mask);
        }

        /* NOTE: reuse get / set FrontPanelTRGOUTEnableMask from parent */
        /* NOTE: disable inherited 740 EasyFrontPanelTRGOUTEnableMask since we only
         * support EasyDPPFrontPanelTRGOUTEnableMask here. */
        /**
         * @brief use getEasyDPPX version instead
         */
        virtual EasyFrontPanelTRGOUTEnableMask getEasyFrontPanelTRGOUTEnableMask() override { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        /**
         * @brief use setEasyDPPX version instead
         */
        virtual void setEasyFrontPanelTRGOUTEnableMask(EasyFrontPanelTRGOUTEnableMask settings) override { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        /**
         * @brief Easy Get DPP FrontPanelTRGOUTEnableMask
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating from the bit mask returned by the
         * the underlying low-level get funtion.
         *
         * @returns
         * EasyDPPFrontPanelTRGOUTEnableMask object
         */
        EasyDPPFrontPanelTRGOUTEnableMask getEasyDPPFrontPanelTRGOUTEnableMask() override
        {
            uint32_t mask;
            mask = getFrontPanelTRGOUTEnableMask();
            return EasyDPPFrontPanelTRGOUTEnableMask(mask);
        }
        /**
         * @brief Easy Set DPP FrontPanelTRGOUTEnableMask
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating to the bit mask needed by the
         * the underlying low-level set funtion.
         *
         * @param settings:
         * EasyDPPFrontPanelTRGOUTEnableMask object
         */
        void setEasyDPPFrontPanelTRGOUTEnableMask(EasyDPPFrontPanelTRGOUTEnableMask settings) override
        {
            uint32_t mask = settings.toBits();
            setFrontPanelTRGOUTEnableMask(mask);
        }

        /* TODO: disable Post Trigger here if implemented in parent */

        /* TODO: can we use a function reference instead to simplify
         * re-exposure of shared structure EasyX helpers? */

        /* NOTE: reuse get / set FrontPanelIOControl from parent */
        /* NOTE: disable inherited 740 EasyFrontPanelIOControl since we only
         * support EasyDPPFrontPanelIOControl here. */
        /**
         * @brief use getEasyDPPX version instead
         */
        virtual EasyFrontPanelIOControl getEasyFrontPanelIOControl() override { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        /**
         * @brief use setEasyDPPX version instead
         */
        virtual void setEasyFrontPanelIOControl(EasyFrontPanelIOControl settings) override { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        /**
         * @brief Easy Get DPP FrontPanelIOControl
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating from the bit mask returned by the
         * the underlying low-level get funtion.
         *
         * @returns
         * EasyDPPFrontPanelIOControl object
         */
        EasyDPPFrontPanelIOControl getEasyDPPFrontPanelIOControl() override
        {
            uint32_t mask;
            mask = getFrontPanelIOControl();
            return EasyDPPFrontPanelIOControl(mask);
        }
        /**
         * @brief Easy Set DPP FrontPanelIOControl
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating to the bit mask needed by the
         * the underlying low-level set funtion.
         *
         * @param settings:
         * EasyDPPFrontPanelIOControl object
         */
        void setEasyDPPFrontPanelIOControl(EasyDPPFrontPanelIOControl settings) override
        {
            uint32_t mask = settings.toBits();
            setFrontPanelIOControl(mask);
        }

        /* NOTE: ROCFPGAFirmwareRevision is inherited from Digitizer740  */
        /* NOTE: disable inherited 740 EasyROCFPGAFirmwareRevision since we only
         * support EasyDPPROCFPGAFirmwareRevision here. */
        /**
         * @brief use getEasyDPPX version instead
         */
        virtual EasyROCFPGAFirmwareRevision getEasyROCFPGAFirmwareRevision() override { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        /**
         * @brief Easy Get DPP ROCFPGAFirmwareRevision
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating from the bit mask returned by the
         * the underlying low-level get funtion.
         *
         * @returns
         * EasyDPPROCFPGAFirmwareRevision object
         */
        EasyDPPROCFPGAFirmwareRevision getEasyDPPROCFPGAFirmwareRevision() override
        {
            uint32_t mask;
            mask = getROCFPGAFirmwareRevision();
            return EasyDPPROCFPGAFirmwareRevision(mask);
        }
        
        /* TODO: wrap Voltage Level Mode Configuration from register docs? */

        /* NOTE: Analog Monitor Mode from DPP register docs looks equal
         * to Monitor DAC Mode from generic model */

        /* TODO: wrap Time Bomb Downcounter from register docs? */

        /* NOTE: reuse get / set FanSpeedControl from parent */
        /* NOTE: disable inherited 740 EasyFanSpeedControl since we only
         * support EasyDPPFanSpeedControl here. */
        /**
         * @brief use getEasyDPPX version instead
         */
        virtual EasyFanSpeedControl getEasyFanSpeedControl() override { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        /**
         * @brief use setEasyDPPX version instead
         */
        virtual void setEasyFanSpeedControl(EasyFanSpeedControl settings) override { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        /**
         * @brief Easy Get FanSpeedControl
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating from the bit mask returned by the
         * the underlying low-level get funtion.
         *
         * @returns
         * EasyFanSpeedControl object
         */
        EasyDPPFanSpeedControl getEasyDPPFanSpeedControl() override
        {
            uint32_t mask;
            mask = getFanSpeedControl();
            return EasyDPPFanSpeedControl(mask);
        }
        /**
         * @brief Easy Set FanSpeedControl
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating to the bit mask needed by the
         * the underlying low-level set funtion.
         *
         * @param settings:
         * EasyFanSpeedControl object
         */
        void setEasyDPPFanSpeedControl(EasyDPPFanSpeedControl settings) override
        {
            uint32_t mask = settings.toBits();
            setFanSpeedControl(mask);
        }

        /* NOTE: reuse get ReadoutControl from parent */
        /* NOTE: disable inherited 740 EasyReadoutControl since we only
         * support EasyDPPReadoutControl here. */
        /**
         * @brief use getEasyDPPX version instead
         */
        virtual EasyReadoutControl getEasyReadoutControl() override { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        /**
         * @brief use setEasyDPPX version instead
         */
        virtual void setEasyReadoutControl(EasyReadoutControl settings) override { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        /**
         * @brief Easy Get DPP ReadoutControl
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating from the bit mask returned by the
         * the underlying low-level get funtion.
         *
         * @returns
         * EasyDPPReadoutControl object
         */
        EasyDPPReadoutControl getEasyDPPReadoutControl() override
        {
            uint32_t mask;
            mask = getReadoutControl();
            return EasyDPPReadoutControl(mask);
        }
        /**
         * @brief Easy Set DPP ReadoutControl
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating to the bit mask needed by the
         * the underlying low-level set funtion.
         *
         * @param settings:
         * EasyDPPReadoutControl object
         */
        void setEasyDPPReadoutControl(EasyDPPReadoutControl settings) override
        {
            uint32_t mask = settings.toBits();
            setReadoutControl(mask);
        }

        /* NOTE: reuse get ReadoutStatus from parent */
        /* NOTE: disable inherited 740 EasyReadoutStatus since we only
         * support EasyDPPReadoutStatus here. */
        /**
         * @brief use getEasyDPPX version instead
         */
        virtual EasyReadoutStatus getEasyReadoutStatus() override { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        /**
         * @brief Easy Get DPP ReadoutStatus
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating from the bit mask returned by the
         * the underlying low-level get funtion.
         *
         * @returns
         * EasyDPPReadoutStatus object
         */
        virtual EasyDPPReadoutStatus getEasyDPPReadoutStatus() override
        {
            uint32_t mask;
            mask = getReadoutStatus();
            return EasyDPPReadoutStatus(mask);
        }

        /* NOTE: reuse get Scratch from parent */
        /* NOTE: disable inherited 740 EasyScratch since we only
         * support EasyDPPScratch here. */
        /**
         * @brief use getEasyDPPX version instead
         */
        virtual EasyScratch getEasyScratch() override { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        /**
         * @brief use setEasyDPPX version instead
         */
        virtual void setEasyScratch(EasyScratch settings) override { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        /**
         * @brief Easy Get DPP Scratch
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating from the bit mask returned by the
         * the underlying low-level get funtion.
         *
         * @returns
         * EasyDPPScratch object
         */
        EasyDPPScratch getEasyDPPScratch() override
        {
            uint32_t mask;
            mask = getScratch();
            return EasyDPPScratch(mask);
        }
        /**
         * @brief Easy Set DPP Scratch
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating to the bit mask needed by the
         * the underlying low-level set funtion.
         *
         * @param settings:
         * EasyDPPScratch object
         */
        void setEasyDPPScratch(EasyDPPScratch settings) override
        {
            uint32_t mask = settings.toBits();
            setScratch(mask);
        }

        /* NOTE: Get / Set Run/Start/Stop Delay from register docs is
         * already covered by RunDelay. */

        /**
         * @brief Get DPP DisableExternalTrigger
         * @returns
         * A boolean to set external trigger state - 1 bit.
         */
        uint32_t getDPPDisableExternalTrigger() override
        {
            uint32_t value;
            errorHandler(CAEN_DGTZ_ReadRegister(handle_, 0x817C, &value));
            return value;
        }
        /**
         * @brief Set DPP DisableExternalTrigger value
         * @param value:
         * Set the low-level DisableExternalTrigger value - 1 bit.
         */
        void setDPPDisableExternalTrigger(uint32_t value) override
        { errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x817C, value & 0x1)); }

        /* NOTE: Buffer Occupancy Gain from general register docs is NOT
         * in DPP register docs */
        /* TODO: explicitly disable Buffer Occupancy Gain here if
         * implemented in parent? */

        /**
         * @brief Get DPP AggregateNumberPerBLT value
         * @returns
         * Number of complete aggregates to be transferred for
         * each block transfer (BLT) - 10 bits.
         */
        uint32_t getDPPAggregateNumberPerBLT() override
        {
            uint32_t value;
            errorHandler(CAEN_DGTZ_ReadRegister(handle_, 0xEF1C, &value));
            return value;
        }
        /**
         * @brief Set DPP AggregateNumberPerBLT value
         * @param value:
         * Number of complete aggregates to be transferred for
         * each block transfer (BLT) - 10 bits.
         */
        void setDPPAggregateNumberPerBLT(uint32_t value) override
        { errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0xEF1C, value & 0x03FF)); }

    };

} // namespace caen
#endif //_CAEN_HPP
