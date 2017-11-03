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
                default : return "Unknown Error";
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
     */
    struct DPPEvents
    {
        void** ptr;             // CAEN_DGTZ_DPP_XXX_Event_t* ptr[MAX_DPP_XXX_CHANNEL_SIZE]
        uint32_t* nEvents;      // Number of events pr channel
        uint32_t allocatedSize;
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
     * @struct EasyAMCFirmwareRevision
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
     * @var EasyAMCFirmwareRevision::minorRevisionNumber
     * AMC Firmware Minor Revision Number (Y).
     * @var EasyAMCFirmwareRevision::majorRevisionNumber
     * AMC Firmware Major Revision Number (X).
     * @var EasyAMCFirmwareRevision::revisionDate
     * AMC Firmware Revision Date (Y/M/DD).
     */
    struct EasyAMCFirmwareRevision {
        /* Only allow the expected number of bits per field */
        uint8_t minorRevisionNumber : 8;
        uint8_t majorRevisionNumber : 8;
        uint16_t revisionDate : 16;
    };

    /**
     * @struct EasyDPPAMCFirmwareRevision
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
     * @var EasyDPPAMCFirmwareRevision::firmwareRevisionNumber
     * Firmware revision number
     * @var EasyDPPAMCFirmwareRevision::firmwareDPPCode
     * Firmware DPP code. Each DPP firmware has a unique code.
     * @var EasyDPPAMCFirmwareRevision::buildDayLower
     * Build Day (lower digit)
     * @var EasyDPPAMCFirmwareRevision::buildDayUpper
     * Build Day (upper digit)
     * @var EasyDPPAMCFirmwareRevision::buildMonth
     * Build Month. For example: 3 means March, 12 is December.
     * @var EasyDPPAMCFirmwareRevision::buildYear
     * Build Year. For example: 0 means 2000, 12 means 2012. NOTE: since
     * 2016 the build year started again from 0.
     */
    struct EasyDPPAMCFirmwareRevision {
        /* Only allow the expected number of bits per field */
        uint8_t firmwareRevisionNumber : 8;
        uint8_t firmwareDPPCode : 8;
        uint8_t buildDayLower : 4;
        uint8_t buildDayUpper : 4;
        uint8_t buildMonth : 4;
        uint8_t buildYear : 4;
    };

    /**
     * @struct EasyDPPAlgorithmControl
     * @brief For user-friendly configuration of DPP Algorithm Control mask.
     *
     * Management of the DPP algorithm features.
     *
     * @var EasyDPPAlgorithmControl::chargeSensitivity
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
     * @var EasyDPPAlgorithmControl::internalTestPulse
     * Internal Test Pulse. It is possible to enable an internal test
     * pulse for debugging purposes. The ADC counts are replaced with
     * the built-in pulse emulator. Options are:\n
     * 0: disabled\n
     * 1: enabled.
     * @var EasyDPPAlgorithmControl::testPulseRate
     * Test Pulse Rate. Set the rate of the built-in test pulse
     * emulator. Options are:\n
     * 00: 1 kHz\n
     * 01: 10 kHz\n
     * 10: 100 kHz\n
     * 11: 1 MHz.
     * @var EasyDPPAlgorithmControl::chargePedestal
     * Charge Pedestal: when enabled a fixed value of 1024 is added to
     * the charge. This feature is useful in case of energies close to
     * zero.
     * @var EasyDPPAlgorithmControl::inputSmoothingFactor
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
     * @var EasyDPPAlgorithmControl::pulsePolarity
     * Pulse Polarity. Options are:\n
     * 0: positive pulse\n
     * 1: negative pulse.
     * @var EasyDPPAlgorithmControl::triggerMode
     * Trigger Mode. Options are:\n
     * 00: Normal mode. Each channel can self-trigger independently from
     * the other channels.\n
     * 01: Paired mode. Each channel of a couple ’n’ acquire the event
     * in logic OR between its self-trigger and the self-trigger of the
     * other channel of the couple. Couple n corresponds to channel n
     * and channel n+2\n
     * 10: Reserved\n
     * 11: Reserved.
     * @var EasyDPPAlgorithmControl::baselineMean
     * Baseline Mean. Sets the number of events for the baseline mean
     * calculation. Options are:\n
     * 000: Fixed: the baseline value is fixed to the value set in
     * register 0x1n38\n
     * 001: 4 samples\n
     * 010: 16 samples\n
     * 011: 64 samples.
     * @var EasyDPPAlgorithmControl::disableSelfTrigger
     * Disable Self Trigger. If disabled, the self-trigger can be still
     * propagated to the TRG-OUT front panel connector, though it is not
     * used by the channel to acquire the event. Options are:\n
     * 0: self-trigger enabled\n
     * 1: self-trigger disabled.
     * @var EasyDPPAlgorithmControl::triggerHysteresis
     * Trigger Hysteresis. The trigger can be inhibited during the
     * trailing edge of a pulse, to avoid re-triggering on the pulse
     * itself. Options are:\n
     * 0 (default value): enabled\n
     * 1: disabled.
     */
    struct EasyDPPAlgorithmControl {
        /* Only allow the expected number of bits per field */
        uint8_t chargeSensitivity : 3;
        uint8_t internalTestPulse : 1;
        uint8_t testPulseRate : 2;
        uint8_t chargePedestal : 1;
        uint8_t inputSmoothingFactor : 3;
        uint8_t pulsePolarity : 1;
        uint8_t triggerMode : 2;
        uint8_t baselineMean : 3;
        uint8_t disableSelfTrigger : 1;
        uint8_t triggerHysteresis : 1;
    };

    /**
     * @struct EasyBoardConfiguration
     * @brief For user-friendly configuration of Board Configuration mask.
     *
     * This register contains general settings for the board
     * configuration.
     *
     * @var EasyBoardConfiguration::triggerOverlapSetting
     * Trigger Overlap Setting (default value is 0).\n
     * When two acquisition windows are overlapped, the second trigger
     * can be either accepted or rejected. Options are:\n
     * 0 = Trigger Overlapping Not Allowed (no trigger is accepted until
     * the current acquisition window is finished);\n
     * 1 = Trigger Overlapping Allowed (the current acquisition window
     * is prematurely closed by the arrival of a new trigger).\n
     * NOTE: it is suggested to keep this bit cleared in case of using a
     * DPP firmware.
     * @var EasyBoardConfiguration::testPatternEnable
     * Test Pattern Enable (default value is 0).\n
     * This bit enables a triangular (0<-->3FFF) test wave to be
     * provided at the ADCs input for debug purposes. Options are:\n
     * 0 = disabled\n
     * 1 = enabled.
     * @var EasyBoardConfiguration::selfTriggerPolarity
     * Self-trigger Polarity (default value is 0).\n
     * Options are:\n
     * 0 = Positive (the self-trigger is generated upon the input pulse
     * overthreshold)\n
     * 1 = Negative (the self-trigger is generated upon the input pulse
     * underthreshold).
     */
    struct EasyBoardConfiguration {
        /* Only allow the expected number of bits per field */
        uint8_t triggerOverlapSetting : 1;
        uint8_t testPatternEnable : 1;
        uint8_t selfTriggerPolarity : 1;
    };

    /**
     * @struct EasyDPPBoardConfiguration
     * @brief For user-friendly configuration of DPP Board Configuration mask.
     *
     * This register contains general settings for the DPP board
     * configuration.
     *
     * @var EasyDPPBoardConfiguration::individualTrigger
     * Individual trigger: must be 1
     * @var EasyDPPBoardConfiguration::analogProbe
     * Analog Probe: Selects which signal is associated to the Analog
     * trace in the readout data. Options are:\n
     * 00: Input\n
     * 01: Smoothed Input\n
     * 10: Baseline\n
     * 11: Reserved.
     * @var EasyDPPBoardConfiguration::waveformRecording
     * Waveform Recording: enables the data recording of the
     * waveform. The user must define the number of samples to be saved
     * in the Record Length 0x1n24 register. Options are:\n
     * 0: disabled\n
     * 1: enabled.
     * @var EasyDPPBoardConfiguration::extrasRecording
     * Extras Recording: when enabled the EXTRAS word is saved into the
     * event data. Refer to the ”Channel Aggregate Data Format” chapter
     * of the DPP User Manual for more details about the EXTRAS
     * word. Options are:\n
     * 0: disabled\n
     * 1: enabled.
     * @var EasyDPPBoardConfiguration::timeStampRecording
     * Time Stamp Recording: must be 1
     * @var EasyDPPBoardConfiguration::chargeRecording
     * Charge Recording: must be 1
     * @var EasyDPPBoardConfiguration::externalTriggerMode
     * External Trigger mode. The external trigger mode on TRG-IN
     * connector can be used according to the following options:\n
     * 00: Trigger\n
     * 01: Veto\n
     * 10: An -Veto\n
     * 11: Reserved.
     */
    struct EasyDPPBoardConfiguration {
        /* Only allow the expected number of bits per field */
        uint8_t individualTrigger : 1;
        uint8_t analogProbe : 2;
        uint8_t waveformRecording : 1;
        uint8_t extrasRecording : 1;
        uint8_t timeStampRecording : 1;
        uint8_t chargeRecording : 1;
        uint8_t externalTriggerMode : 2;
    };

    /**
     * @struct EasyAcquisitionControl
     * @brief For user-friendly configuration of Acquisition Control mask.
     *
     * This register manages the acquisition settings.
     *
     * @var EasyAcquisitionControl::startStopMode
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
     * @var EasyAcquisitionControl::acquisitionStartArm
     * Acquisition Start/Arm (default value is 0).\n
     * When bits[1:0] = 00, this bit acts as a Run Start/Stop. When
     * bits[1:0] = 01, 10, 11, this bit arms the acquisition and the
     * actual Start/Stop is controlled by an external signal. Options
     * are:\n
     * 0 = Acquisition STOP (if bits[1:0]=00); Acquisition DISARMED (others)\n
     * 1 = Acquisition RUN (if bits[1:0]=00); Acquisition ARMED (others).
     * @var EasyAcquisitionControl::triggerCountingMode
     * Trigger Counting Mode (default value is 0). Through this bit it
     * is possible to count the reading requests from channels to mother
     * board. The reading requests may come from the following options:\n
     * 0 = accepted triggers from combination of channels\n
     * 1 = triggers from combination of channels, in addition to TRG-IN
     * and SW TRG.
     * @var EasyAcquisitionControl::memoryFullModeSelection
     * Memory Full Mode Selection (default value is 0). Options are:\n
     * 0 = NORMAL. The board is full whenever all buffers are full\n
     * 1 = ONE BUFFER FREE. The board is full whenever Nb-1 buffers are
     * full, where Nb is the overall number of buffers in which the
     * channel memory is divided.
     * @var EasyAcquisitionControl::pLLRefererenceClockSource
     * PLL Reference Clock Source (Desktop/NIM only). Default value is 0.\n
     * Options are:\n
     * 0 = internal oscillator (50 MHz)\n
     * 1 = external clock from front panel CLK-IN connector.\n
     * NOTE: this bit is reserved in case of VME boards.
     * @var EasyAcquisitionControl::lVDSIOBusyEnable
     * LVDS I/O Busy Enable (VME only). Default value is 0.\n
     * The LVDS I/Os can be programmed to accept a Busy signal as input,
     * or to propagate it as output. Options are:\n
     * 0 = disabled\n
     * 1 = enabled.\n
     * NOTE: this bit is supported only by VME boards and meaningful
     * only if the LVDS new features are enabled (bit[8]=1 of register
     * 0x811C). Register 0x81A0 should also be configured for nBusy/nVeto.
     * @var EasyAcquisitionControl::lVDSVetoEnable
     * LVDS I/O Veto Enable (VME only). Default value is 0.\n
     * The LVDS I/Os can be programmed to accept a Veto signal as input,
     * or to transfer it as output. Options are:\n
     * 0 = disabled (default)\n
     * 1 = enabled.\n
     * NOTE: this bit is supported only by VME boards and meaningful
     * only if the LVDS new features are enabled (bit[8]=1 of register
     * 0x811C). Register 0x81A0 should also be configured for nBusy/nVeto.
     * @var EasyAcquisitionControl::lVDSIORunInEnable
     * LVDS I/O RunIn Enable Mode (VME only). Default value is 0.\n
     * The LVDS I/Os can be programmed to accept a RunIn signal as
     * input, or to transfer it as output. Options are:\n
     * 0 = starts on RunIn level (default)\n
     * 1 = starts on RunIn rising edge.\n
     * NOTE: this bit is supported only by VME boards and meaningful
     * only if the LVDS new features are enabled (bit[8]=1 of register
     * 0x811C). Register 0x81A0 must also be configured for nBusy/nVeto.
     */
    struct EasyAcquisitionControl {
        /* Only allow the expected number of bits per field */
        uint8_t startStopMode : 2;
        uint8_t acquisitionStartArm : 1;
        uint8_t triggerCountingMode : 1;
        uint8_t memoryFullModeSelection : 1;
        uint8_t pLLRefererenceClockSource : 1;
        uint8_t lVDSIOBusyEnable : 1;
        uint8_t lVDSVetoEnable : 1;
        uint8_t lVDSIORunInEnable : 1;
    };

    /**
     * @struct EasyDPPAcquisitionControl
     * @brief For user-friendly configuration of Acquisition Control mask.
     *
     * This register manages the acquisition settings.
     *
     * @var EasyDPPAcquisitionControl::startStopMode
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
     * @var EasyDPPAcquisitionControl::acquisitionStartArm
     * Acquisition Start/Arm (default value is 0).\n
     * When bits[1:0] = 00, this bit acts as a Run Start/Stop. When
     * bits[1:0] = 01, 10, 11, this bit arms the acquisition and the
     * actual Start/Stop is controlled by an external signal. Options
     * are:\n
     * 0 = Acquisition STOP (if bits[1:0]=00); Acquisition DISARMED (others)\n
     * 1 = Acquisition RUN (if bits[1:0]=00); Acquisition ARMED (others).
     * @var EasyDPPAcquisitionControl::triggerCountingMode
     * Trigger Counting Mode (default value is 0). Through this bit it
     * is possible to count the reading requests from channels to mother
     * board. The reading requests may come from the following options:\n
     * 0 = accepted triggers from combination of channels\n
     * 1 = triggers from combination of channels, in addition to TRG-IN
     * and SW TRG.
     * @var EasyDPPAcquisitionControl::pLLRefererenceClockSource
     * PLL Reference Clock Source (Desktop/NIM only). Default value is 0.\n
     * Options are:\n
     * 0 = internal oscillator (50 MHz)\n
     * 1 = external clock from front panel CLK-IN connector.\n
     * NOTE: this bit is reserved in case of VME boards.
     * @var EasyDPPAcquisitionControl::lVDSIOBusyEnable
     * LVDS I/O Busy Enable (VME only). Default value is 0.\n
     * The LVDS I/Os can be programmed to accept a Busy signal as input,
     * or to propagate it as output. Options are:\n
     * 0 = disabled\n
     * 1 = enabled.\n
     * NOTE: this bit is supported only by VME boards and meaningful
     * only if the LVDS new features are enabled (bit[8]=1 of register
     * 0x811C). Register 0x81A0 should also be configured for nBusy/nVeto.
     * @var EasyDPPAcquisitionControl::lVDSVetoEnable
     * LVDS I/O Veto Enable (VME only). Default value is 0.\n
     * The LVDS I/Os can be programmed to accept a Veto signal as input,
     * or to transfer it as output. Options are:\n
     * 0 = disabled (default)\n
     * 1 = enabled.\n
     * NOTE: this bit is supported only by VME boards and meaningful
     * only if the LVDS new features are enabled (bit[8]=1 of register
     * 0x811C). Register 0x81A0 should also be configured for nBusy/nVeto.
     * @var EasyDPPAcquisitionControl::lVDSIORunInEnable
     * LVDS I/O RunIn Enable Mode (VME only). Default value is 0.\n
     * The LVDS I/Os can be programmed to accept a RunIn signal as
     * input, or to transfer it as output. Options are:\n
     * 0 = starts on RunIn level (default)\n
     * 1 = starts on RunIn rising edge.\n
     * NOTE: this bit is supported only by VME boards and meaningful
     * only if the LVDS new features are enabled (bit[8]=1 of register
     * 0x811C). Register 0x81A0 must also be configured for nBusy/nVeto.
     */
    struct EasyDPPAcquisitionControl {
        /* Only allow the expected number of bits per field */
        uint8_t startStopMode : 2;
        uint8_t acquisitionStartArm : 1;
        uint8_t triggerCountingMode : 1;
        uint8_t pLLRefererenceClockSource : 1;
        uint8_t lVDSIOBusyEnable : 1;
        uint8_t lVDSVetoEnable : 1;
        uint8_t lVDSIORunInEnable : 1;
    };

    /**
     * @struct EasyAcquisitionStatus
     * @brief For user-friendly configuration of Acquisition Status mask.
     *
     * This register monitors a set of conditions related to the
     * acquisition status.
     *
     * @var EasyAcquisitionStatus::acquisitionStatus
     * Acquisition Status. It reflects the status of the acquisition and
     * drivers the front panel ’RUN’ LED. Options are:\n
     * 0 = acquisition is stopped (’RUN’ is off)\n
     * 1 = acquisition is running (’RUN’ lits).
     * @var EasyAcquisitionStatus::eventReady
     * Event Ready. Indicates if any events are available for
     * readout. Options are:\n
     * 0 = no event is available for readout\n
     * 1 = at least one event is available for readout.\n
     * NOTE: the status of this bit must be considered when managing the
     * readout from the digitizer.
     * @var EasyAcquisitionStatus::eventFull
     * Event Full. Indicates if at least one channel has reached the
     * FULL condition. Options are:\n
     * 0 = no channel has reached the FULL condition\n
     * 1 = the maximum number of events to be read is reached.
     * @var EasyAcquisitionStatus::clockSource
     * Clock Source. Indicates the clock source status. Options are:\n
     * 0 = internal (PLL uses the internal 50 MHz oscillator as reference)\n
     * 1 = external (PLL uses the external clock on CLK-IN connector as
     * reference).
     * @var EasyAcquisitionStatus::pLLBypassMode
     * PLL Bypass Mode. This bit drives the front panel 'PLL BYPS' LED.\n
     * Options are:\n
     * 0 = PLL bypass mode is not ac ve ('PLL BYPS' is off)\n
     * 1 = PLL bypass mode is ac ve and the VCXO frequency directly
     * drives the clock distribution chain ('PLL BYPS' lits).\n
     * WARNING: before operating in PLL Bypass Mode, it is recommended
     * to contact CAEN for feasibility.
     * @var EasyAcquisitionStatus::pLLUnlockDetect
     * PLL Unlock Detect. This bit flags a PLL unlock condition. Options
     * are:\n
     * 0 = PLL has had an unlock condition since the last register read
     * access\n
     * 1 = PLL has not had any unlock condition since the last register
     * read access.\n
     * NOTE: flag can be restored to 1 via read access to register 0xEF04.
     * @var EasyAcquisitionStatus::boardReady
     * Board Ready. This flag indicates if the board is ready for
     * acquisition (PLL and ADCs are correctly synchronised). Options
     * are:\n
     * 0 = board is not ready to start the acquisition\n
     * 1 = board is ready to start the acquisition.\n
     * NOTE: this bit should be checked a er software reset to ensure
     * that the board will enter immediately in run mode a er the RUN
     * mode setting; otherwise, a latency between RUN mode setting and
     * Acquisition start might occur.
     * @var EasyAcquisitionStatus::s_IN
     * S-IN (VME boards) or GPI (DT/NIM boards) Status. Reads the
     * current logical level on S-IN (GPI) front panel connector.
     * @var EasyAcquisitionStatus::tRG_IN
     * TRG-IN Status. Reads the current logical level on TRG-IN front
     * panel connector.
     */
    struct EasyAcquisitionStatus {
        /* Only allow the expected number of bits per field */
        uint8_t acquisitionStatus : 1;
        uint8_t eventReady : 1;
        uint8_t eventFull : 1;
        uint8_t clockSource : 1;
        uint8_t pLLBypassMode : 1;
        uint8_t pLLUnlockDetect : 1;
        uint8_t boardReady : 1;
        uint8_t s_IN : 1;
        uint8_t tRG_IN : 1;
    };

    /**
     * @struct EasyDPPAcquisitionStatus
     * @brief For user-friendly configuration of Acquisition Status mask.
     *
     * This register monitors a set of conditions related to the
     * acquisition status.
     *
     * @var EasyDPPAcquisitionStatus::acquisitionStatus
     * Acquisition Status. It reflects the status of the acquisition and
     * drivers the front panel ’RUN’ LED. Options are:\n
     * 0 = acquisition is stopped (’RUN’ is off)\n
     * 1 = acquisition is running (’RUN’ lits).
     * @var EasyDPPAcquisitionStatus::eventReady
     * Event Ready. Indicates if any events are available for
     * readout. Options are:\n
     * 0 = no event is available for readout\n
     * 1 = at least one event is available for readout.\n
     * NOTE: the status of this bit must be considered when managing the
     * readout from the digitizer.
     * @var EasyDPPAcquisitionStatus::eventFull
     * Event Full. Indicates if at least one channel has reached the
     * FULL condition. Options are:\n
     * 0 = no channel has reached the FULL condition\n
     * 1 = the maximum number of events to be read is reached.
     * @var EasyDPPAcquisitionStatus::clockSource
     * Clock Source. Indicates the clock source status. Options are:\n
     * 0 = internal (PLL uses the internal 50 MHz oscillator as reference)\n
     * 1 = external (PLL uses the external clock on CLK-IN connector as
     * reference).
     * @var EasyDPPAcquisitionStatus::pLLUnlockDetect
     * PLL Unlock Detect. This bit flags a PLL unlock condition. Options
     * are:\n
     * 0 = PLL has had an unlock condition since the last register read
     * access\n
     * 1 = PLL has not had any unlock condition since the last register
     * read access.\n
     * NOTE: flag can be restored to 1 via read access to register 0xEF04.
     * @var EasyDPPAcquisitionStatus::boardReady
     * Board Ready. This flag indicates if the board is ready for
     * acquisition (PLL and ADCs are correctly synchronised). Options
     * are:\n
     * 0 = board is not ready to start the acquisition\n
     * 1 = board is ready to start the acquisition.\n
     * NOTE: this bit should be checked a er software reset to ensure
     * that the board will enter immediately in run mode a er the RUN
     * mode setting; otherwise, a latency between RUN mode setting and
     * Acquisition start might occur.
     * @var EasyDPPAcquisitionStatus::s_IN
     * S-IN (VME boards) or GPI (DT/NIM boards) Status. Reads the
     * current logical level on S-IN (GPI) front panel connector.
     * @var EasyDPPAcquisitionStatus::tRG_IN
     * TRG-IN Status. Reads the current logical level on TRG-IN front
     * panel connector.
     */
    struct EasyDPPAcquisitionStatus {
        /* Only allow the expected number of bits per field */
        uint8_t acquisitionStatus : 1;
        uint8_t eventReady : 1;
        uint8_t eventFull : 1;
        uint8_t clockSource : 1;
        uint8_t pLLUnlockDetect : 1;
        uint8_t boardReady : 1;
        uint8_t s_IN : 1;
        uint8_t tRG_IN : 1;
    };

    /**
     * @struct EasyGlobalTriggerMask
     * @brief For user-friendly configuration of Global Trigger Mask.
     *
     * This register sets which signal can contribute to the global
     * trigger generation.
     *
     * @var EasyGlobalTriggerMask::groupTriggerMask
     * Bit n corresponds to the trigger request from group n that
     * participates to the global trigger generation (n = 0,...,3 for DT
     * and NIM; n = 0,...,7 for VME boards). Options are:\n
     * 0 = trigger request is not sensed for global trigger generation\n
     * 1 = trigger request participates in the global trigger generation.\n
     * NOTE: in case of DT and NIMboards, only bits[3:0] are meaningful,
     * while bits[7:4] are reserved.
     * @var EasyGlobalTriggerMask::majorityCoincidenceWindow
     * Majority Coincidence Window. Sets the me window (in units of the
     * trigger clock) for the majority coincidence. Majority level must
     * be set different from 0 through bits[26:24].
     * @var EasyGlobalTriggerMask::majorityLevel
     * Majority Level. Sets the majority level for the global trigger
     * generation. For a level m, the trigger fires when at least m+1 of
     * the enabled trigger requests (bits[7:0] or [3:0]) are
     * over-threshold inside the majority coincidence window
     * (bits[23:20]).\n
     * NOTE: the majority level must be smaller than the number of
     * trigger requests enabled via bits[7:0] mask (or [3:0]).
     * @var EasyGlobalTriggerMask::lVDSTrigger
     * LVDS Trigger (VME boards only). When enabled, the trigger from
     * LVDS I/O participates to the global trigger generation (in logic
     * OR). Options are:\n
     * 0 = disabled\n
     * 1 = enabled.
     * @var EasyGlobalTriggerMask::externalTrigger
     * External Trigger (default value is 1). When enabled, the external
     * trigger on TRG-IN participates to the global trigger generation
     * in logic OR with the other enabled signals. Options are:\n
     * 0 = disabled\n
     * 1 = enabled.
     * @var EasyGlobalTriggerMask::softwareTrigger
     * Software Trigger (default value is 1). When enabled, the software
     * trigger participates to the global trigger signal generation in
     * logic OR with the other enabled signals. Options are:\n
     * 0 = disabled\n
     * 1 = enabled.
     */
    struct EasyGlobalTriggerMask {
        /* Only allow the expected number of bits per field */
        uint8_t groupTriggerMask : 8;
        uint8_t majorityCoincidenceWindow : 4;
        uint8_t majorityLevel : 3;
        uint8_t lVDSTrigger : 1;
        uint8_t externalTrigger : 1;
        uint8_t softwareTrigger : 1;
    };

    /**
     * @struct EasyDPPGlobalTriggerMask
     * @brief For user-friendly configuration of Global Trigger Mask.
     *
     * This register sets which signal can contribute to the global
     * trigger generation.
     *
     * @var EasyDPPGlobalTriggerMask::lVDSTrigger
     * LVDS Trigger (VME boards only). When enabled, the trigger from
     * LVDS I/O participates to the global trigger generation (in logic
     * OR). Options are:\n
     * 0 = disabled\n
     * 1 = enabled.
     * @var EasyDPPGlobalTriggerMask::externalTrigger
     * External Trigger (default value is 1). When enabled, the external
     * trigger on TRG-IN participates to the global trigger generation
     * in logic OR with the other enabled signals. Options are:\n
     * 0 = disabled\n
     * 1 = enabled.
     * @var EasyDPPGlobalTriggerMask::softwareTrigger
     * Software Trigger (default value is 1). When enabled, the software
     * trigger participates to the global trigger signal generation in
     * logic OR with the other enabled signals. Options are:\n
     * 0 = disabled\n
     * 1 = enabled.
     */
    struct EasyDPPGlobalTriggerMask {
        /* Only allow the expected number of bits per field */
        uint8_t lVDSTrigger : 1;
        uint8_t externalTrigger : 1;
        uint8_t softwareTrigger : 1;
    };

    /**
     * @struct EasyFrontPanelTRGOUTEnableMask
     * @brief For user-friendly configuration of Front Panel TRG-OUT Enable Mask.
     *
     * This register sets which signal can contribute to
     * generate the signal on the front panel TRG-OUT LEMO connector
     * (GPO in case of DT and NIM boards).
     *
     * @var EasyFrontPanelTRGOUTEnableMask::groupTriggerMask
     * This mask sets the trigger requests participating in the TRG-OUT
     * (GPO). Bit n corresponds to the trigger request from group n.\n
     * Options are:\n
     * 0 = Trigger request does not participate to the TRG-OUT (GPO) signal\n
     * 1 = Trigger request participates to the TRG-OUT (GPO) signal.\n
     * NOTE: in case of DT and NIM boards, only bits[3:0] are meaningful
     * while bis[7:4] are reserved.
     * @var EasyFrontPanelTRGOUTEnableMask::tRGOUTGenerationLogic
     * TRG-OUT (GPO) Generation Logic. The enabled trigger requests
     * (bits [7:0] or [3:0]) can be combined to generate the TRG-OUT
     * (GPO) signal. Options are:\n
     * 00 = OR\n
     * 01 = AND\n
     * 10 = Majority\n
     * 11 = reserved.
     * @var EasyFrontPanelTRGOUTEnableMask::majorityLevel
     * Majority Level. Sets the majority level for the TRG-OUT (GPO)
     * signal generation. Allowed level values are between 0 and 7 for
     * VME boards, and between 0 and 3 for DT and NIM boards. For a
     * level m, the trigger fires when at least m+1 of the trigger
     * requests are generated by the enabled channels (bits [7:0] or [3:0]).
     * @var EasyFrontPanelTRGOUTEnableMask::lVDSTriggerEnable
     * LVDS Trigger Enable (VME boards only). If the LVDS I/Os are
     * programmed as outputs, they can participate in the TRG-OUT (GPO)
     * signal generation. They are in logic OR with the other enabled
     * signals. Options are:\n
     * 0 = disabled\n
     * 1 = enabled.
     * @var EasyFrontPanelTRGOUTEnableMask::externalTrigger
     * External Trigger. When enabled, the external trigger on TRG-IN
     * can participate in the TRG-OUT (GPO) signal generation in logic
     * OR with the other enabled signals. Options are:\n
     * 0 = disabled\n
     * 1 = enabled.
     * @var EasyFrontPanelTRGOUTEnableMask::softwareTrigger
     * Software Trigger. When enabled, the software trigger can
     * participate in the TRG-OUT (GPO) signal generation in logic OR
     * with the other enabled signals. Options are:\n
     * 0 = disabled\n
     * 1 = enabled.
     */
    struct EasyFrontPanelTRGOUTEnableMask {
        /* Only allow the expected number of bits per field */
        uint8_t groupTriggerMask : 8;
        uint8_t tRGOUTGenerationLogic : 2;
        uint8_t majorityLevel : 3;
        uint8_t lVDSTriggerEnable : 1;
        uint8_t externalTrigger : 1;
        uint8_t softwareTrigger : 1;
    };

    /**
     * @struct EasyDPPFrontPanelTRGOUTEnableMask
     * @brief For user-friendly configuration of Front Panel TRG-OUT Enable Mask.
     *
     * This register sets which signal can contribute to
     * generate the signal on the front panel TRG-OUT LEMO connector
     * (GPO in case of DT and NIM boards).
     *
     * @var EasyDPPFrontPanelTRGOUTEnableMask::lVDSTriggerEnable
     * LVDS Trigger Enable (VME boards only). If the LVDS I/Os are
     * programmed as outputs, they can participate in the TRG-OUT (GPO)
     * signal generation. They are in logic OR with the other enabled
     * signals. Options are:\n
     * 0 = disabled\n
     * 1 = enabled.
     * @var EasyDPPFrontPanelTRGOUTEnableMask::externalTrigger
     * External Trigger. When enabled, the external trigger on TRG-IN
     * can participate in the TRG-OUT (GPO) signal generation in logic
     * OR with the other enabled signals. Options are:\n
     * 0 = disabled\n
     * 1 = enabled.
     * @var EasyDPPFrontPanelTRGOUTEnableMask::softwareTrigger
     * Software Trigger. When enabled, the software trigger can
     * participate in the TRG-OUT (GPO) signal generation in logic OR
     * with the other enabled signals. Options are:\n
     * 0 = disabled\n
     * 1 = enabled.
     */
    struct EasyDPPFrontPanelTRGOUTEnableMask {
        /* Only allow the expected number of bits per field */
        uint8_t lVDSTriggerEnable : 1;
        uint8_t externalTrigger : 1;
        uint8_t softwareTrigger : 1;
    };

    /**
     * @struct EasyFrontPanelIOControl
     * @brief For user-friendly configuration of Acquisition Control mask.
     *
     * This register manages the front panel I/O connectors. Default
     * value is 0x000000.
     *
     * @var EasyFrontPanelIOControl::lEMOIOElectricalLevel
     * LEMO I/Os Electrical Level. This bit sets the electrical level of
     * the front panel LEMO connectors: TRG-IN, TRG-OUT (GPO in case of
     * DT and NIM boards), S-IN (GPI in case of DT and NIM
     * boards). Options are:\n
     * 0 = NIM I/O levels\n
     * 1 = TTL I/O levels.
     * @var EasyFrontPanelIOControl::tRGOUTEnable
     * TRG-OUT Enable (VME boards only). Enables the TRG-OUT LEMO front
     * panel connector. Options are:\n
     * 0 = enabled (default)\n
     * 1 = high impedance.\n
     * NOTE: this bit is reserved in case of DT and NIM boards.
     * @var EasyFrontPanelIOControl::lVDSIODirectionFirst
     * LVDS I/O [3:0] Direction (VME boards only). Sets the direction of
     * the signals on the first 4-pin group of the LVDS I/O
     * connector. Options are:\n
     * 0 = input\n
     * 1 = output.\n
     * NOTE: this bit is reserved in case of DT and NIM boards.
     * @var EasyFrontPanelIOControl::lVDSIODirectionSecond
     * LVDS I/O [7:4] Direction (VME boards only). Sets the direction of
     * the second 4-pin group of the LVDS I/O connector. Options are:\n
     * 0 = input\n
     * 1 = output.\n
     * NOTE: this bit is reserved in case of DT and NIM boards.
     * @var EasyFrontPanelIOControl::lVDSIODirectionThird
     * LVDS I/O [11:8] Direction (VME boards only). Sets the direction of
     * the third 4-pin group of the LVDS I/O connector. Options are:\n
     * 0 = input\n
     * 1 = output.\n
     * NOTE: this bit is reserved in case of DT and NIM boards.
     * @var EasyFrontPanelIOControl::lVDSIODirectionFourth
     * LVDS I/O [15:12] Direction (VME boards only). Sets the direction of
     * the fourth 4-pin group of the LVDS I/O connector. Options are:\n
     * 0 = input\n
     * 1 = output.\n
     * NOTE: this bit is reserved in case of DT and NIM boards.
     * @var EasyFrontPanelIOControl::lVDSIOSignalConfiguration
     * LVDS I/O Signal Configuration (VME boards and LVDS I/O old
     * features only). This configuration must be enabled through bit[8]
     * set to 0. Options are:\n
     * 00 = general purpose I/O\n
     * 01 = programmed I/O\n
     * 10 = pattern mode: LVDS signals are input and their value is
     * written into the header PATTERN field\n
     * 11 = reserved.\n
     * NOTE: these bits are reserved in case of DT and NIM boards.
     * @var EasyFrontPanelIOControl::lVDSIONewFeaturesSelection
     * LVDS I/O New Features Selection (VME boards only). Options are:\n
     * 0 = LVDS old features\n
     * 1 = LVDS new features.\n
     * The new features options can be configured through register
     * 0x81A0. Please, refer to the User Manual for all details.\n
     * NOTE: LVDS I/O New Features option is valid from motherboard
     * firmware revision 3.8 on.\n
     * NOTE: this bit is reserved in case of DT and NIM boards.
     * @var EasyFrontPanelIOControl::lVDSIOPatternLatchMode
     * LVDS I/Os Pattern Latch Mode (VME boards only). Options are:\n
     * 0 = Pattern (i.e. 16-pin LVDS status) is latched when the
     * (internal) global trigger is sent to channels, in consequence of
     * an external trigger. It accounts for post-trigger settings and
     * input latching delays\n
     * 1 = Pattern (i.e. 16-pin LVDS status) is latched when an external
     * trigger arrives.\n
     * NOTE: this bit is reserved in case of DT and NIM boards.
     * @var EasyFrontPanelIOControl::tRGINControl
     * TRG-IN control. The board trigger logic can be synchronized
     * either with the edge of the TRG-IN signal, or with its whole
     * duration.\n
     * Note: this bit must be used in conjunction with bit[11] =
     * 0. Options are:\n
     * 0 = trigger is synchronized with the edge of the TRG-IN signal\n
     * 1 = trigger is synchronized with the whole duration of the TRG-IN
     * signal.
     * @var EasyFrontPanelIOControl::tRGINMezzanines
     * TRG-IN to Mezzanines (channels). Options are:\n
     * 0 = TRG-IN signal is processed by the motherboard and sent to
     * mezzanine (default). The trigger logic is then synchronized with
     * TRG-IN\n
     * 1 = TRG-IN is directly sent to the mezzanines with no mother
     * board processing nor delay. This option can be useful when TRG-IN
     * is used to veto the acquisition.\n
     * NOTE: if this bit is set to 1, then bit[10] is ignored.
     * @var EasyFrontPanelIOControl::forceTRGOUT
     * Force TRG-OUT (GPO). This bit can force TRG-OUT (GPO in case of
     * DT and NIM boards) test logical level if bit[15] = 1. Options
     * are:\n
     * 0 = Force TRG-OUT (GPO) to 0\n
     * 1 = Force TRG-OUT (GPO) to 1.
     * @var EasyFrontPanelIOControl::tRGOUTMode
     * TRG-OUT (GPO) Mode. Options are:\n
     * 0 = TRG-OUT (GPO) is an internal signal (according to bits[17:16])\n
     * 1= TRG-OUT (GPO) is a test logic level set via bit[14].
     * @var EasyFrontPanelIOControl::tRGOUTModeSelection
     * TRG-OUT (GPO) Mode Selection. Options are:\n
     * 00 = Trigger: TRG-OUT/GPO propagates the internal trigger sources
     * according to register 0x8110\n
     * 01 = Motherboard Probes: TRG-OUT/GPO is used to propagate signals
     * of the motherboards according to bits[19:18]\n
     * 10 = Channel Probes: TRG-OUT/GPO is used to propagate signals of
     * the mezzanines (Channel Signal Virtual Probe)\n
     * 11 = S-IN (GPI) propagation.
     * @var EasyFrontPanelIOControl::motherboardVirtualProbeSelection
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
     * @var EasyFrontPanelIOControl::motherboardVirtualProbePropagation
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
     * @var EasyFrontPanelIOControl::patternConfiguration
     * Pattern Configuration. Configures the information given by the
     * 16-bit PATTERN field in the header of the event format (VME
     * only). Option are:\n
     * 00 = PATTERN: 16-bit pattern latched on the 16 LVDS signals as
     * one trigger arrives (default)\n
     * Other options are reserved.
     */
    struct EasyFrontPanelIOControl {
        /* Only allow the expected number of bits per field */
        uint8_t lEMOIOElectricalLevel : 1;
        uint8_t tRGOUTEnable : 1;
        uint8_t lVDSIODirectionFirst : 1;
        uint8_t lVDSIODirectionSecond : 1;
        uint8_t lVDSIODirectionThird : 1;
        uint8_t lVDSIODirectionFourth : 1;
        uint8_t lVDSIOSignalConfiguration : 2;
        uint8_t lVDSIONewFeaturesSelection : 1;
        uint8_t lVDSIOPatternLatchMode : 1;
        uint8_t tRGINControl : 1;
        uint8_t tRGINMezzanines : 1;
        uint8_t forceTRGOUT : 1;
        uint8_t tRGOUTMode : 1;
        uint8_t tRGOUTModeSelection : 2;
        uint8_t motherboardVirtualProbeSelection : 2;
        uint8_t motherboardVirtualProbePropagation : 1;
        uint8_t patternConfiguration : 2;
    };
    /* NOTE: 740 and 740DPP layout is identical */
    using EasyDPPFrontPanelIOControl = EasyFrontPanelIOControl;

    /**
     * @struct EasyROCFPGAFirmwareRevision
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
     * @var EasyROCFPGAFirmwareRevision::minorRevisionNumber
     * ROC Firmware Minor Revision Number (Y).
     * @var EasyROCFPGAFirmwareRevision::majorRevisionNumber
     * ROC Firmware Major Revision Number (X).
     * @var EasyROCFPGAFirmwareRevision::revisionDate
     * ROC Firmware Revision Date (Y/M/DD).
     */
    struct EasyROCFPGAFirmwareRevision {
        /* Only allow the expected number of bits per field */
        uint8_t minorRevisionNumber : 8;
        uint8_t majorRevisionNumber : 8;
        uint16_t revisionDate : 16;
    };
    /* NOTE: 740 and 740DPP layout is identical */
    using EasyDPPROCFPGAFirmwareRevision = EasyROCFPGAFirmwareRevision;

    /**
     * @struct EasyFanSpeedControl
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
     * @var EasyFanSpeedControl::fanSpeedMode
     * Fan Speed Mode. Options are:\n
     * 0 = slow speed or automatic speed tuning\n
     * 1 = high speed.
     */
    struct EasyFanSpeedControl {
        /* Only allow the expected number of bits per field */
        uint8_t fanSpeedMode : 1;
    };
    /* NOTE: 740 and 740DPP layout is identical */
    using EasyDPPFanSpeedControl = EasyFanSpeedControl;

    /**
     * @class EasyHelper
     * @brief For user-friendly configuration of various bit masks.
     *
     * Base class to handle all the translation between named variables
     * and bit masks. All actual such mask helpers should just inherit
     * from it and implment the specific variable names and specifications.
     *
     * @param EasyHelper::mask
     * Initialize from bit mask
     */
    class EasyHelper
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
            initLayout();
            autoInit(mask);
        };
    public:
        virtual const std::string getClassName() const { return "EasyHelper"; };
        EasyHelper()
        {
            /* Default constructor - do nothing */
            construct();
        }
        EasyHelper(const uint32_t mask)
        {
            /* Construct from bit mask - override in children if needed */
            constructFromMask(mask);
        }
        ~EasyHelper()
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
            uint32_t mask = 0;
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
    }; // class EasyHelper


    /**
     * @class EasyBoardConfigurationHelper
     * @brief For user-friendly configuration of Board Configuration mask.
     *
     * This register contains general settings for the board
     * configuration.
     *
     * @param EasyBoardConfigurationHelper::triggerOverlapSetting
     * Trigger Overlap Setting (default value is 0).\n
     * When two acquisition windows are overlapped, the second trigger
     * can be either accepted or rejected. Options are:\n
     * 0 = Trigger Overlapping Not Allowed (no trigger is accepted until
     * the current acquisition window is finished);\n
     * 1 = Trigger Overlapping Allowed (the current acquisition window
     * is prematurely closed by the arrival of a new trigger).\n
     * NOTE: it is suggested to keep this bit cleared in case of using a
     * DPP firmware.
     * @param EasyBoardConfigurationHelper::testPatternEnable
     * Test Pattern Enable (default value is 0).\n
     * This bit enables a triangular (0<-->3FFF) test wave to be
     * provided at the ADCs input for debug purposes. Options are:\n
     * 0 = disabled\n
     * 1 = enabled.
     * @param EasyBoardConfigurationHelper::selfTriggerPolarity
     * Self-trigger Polarity (default value is 0).\n
     * Options are:\n
     * 0 = Positive (the self-trigger is generated upon the input pulse
     * overthreshold)\n
     * 1 = Negative (the self-trigger is generated upon the input pulse
     * underthreshold).
     */
    class EasyBoardConfigurationHelper : public EasyHelper
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
        virtual const std::string getClassName() const override { return "EasyBoardConfigurationHelper"; };
        /* Construct using default values from docs */
        EasyBoardConfigurationHelper(const uint8_t triggerOverlapSetting, const uint8_t testPatternEnable, const uint8_t selfTriggerPolarity)
        {
            construct(triggerOverlapSetting, testPatternEnable, selfTriggerPolarity);
        }
        /* Construct from low-level bit mask in line with docs */
        EasyBoardConfigurationHelper(const uint32_t mask)
        {
            constructFromMask(mask);
        }
    }; // class EasyBoardConfigurationHelper


    /**
     * @class EasyDPPBoardConfigurationHelper
     * @brief For user-friendly configuration of DPP Board Configuration mask.
     *
     * This register contains general settings for the DPP board
     * configuration.
     *
     * @param EasyDPPBoardConfigurationHelper::individualTrigger
     * Individual trigger: must be 1
     * @param EasyDPPBoardConfigurationHelper::analogProbe
     * Analog Probe: Selects which signal is associated to the Analog
     * trace in the readout data. Options are:\n
     * 00: Input\n
     * 01: Smoothed Input\n
     * 10: Baseline\n
     * 11: Reserved.
     * @param EasyDPPBoardConfigurationHelper::waveformRecording
     * Waveform Recording: enables the data recording of the
     * waveform. The user must define the number of samples to be saved
     * in the Record Length 0x1n24 register. Options are:\n
     * 0: disabled\n
     * 1: enabled.
     * @param EasyDPPBoardConfigurationHelper::extrasRecording
     * Extras Recording: when enabled the EXTRAS word is saved into the
     * event data. Refer to the ”Channel Aggregate Data Format” chapter
     * of the DPP User Manual for more details about the EXTRAS
     * word. Options are:\n
     * 0: disabled\n
     * 1: enabled.
     * @param EasyDPPBoardConfigurationHelper::timeStampRecording
     * Time Stamp Recording: must be 1
     * @param EasyDPPBoardConfigurationHelper::chargeRecording
     * Charge Recording: must be 1
     * @param EasyDPPBoardConfigurationHelper::externalTriggerMode
     * External Trigger mode. The external trigger mode on TRG-IN
     * connector can be used according to the following options:\n
     * 00: Trigger\n
     * 01: Veto\n
     * 10: An -Veto\n
     * 11: Reserved.
     */
    class EasyDPPBoardConfigurationHelper : public EasyHelper
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
        virtual const std::string getClassName() const override { return "EasyDPPBoardConfigurationHelper"; };
        /* Construct using default values from docs */
        EasyDPPBoardConfigurationHelper(const uint8_t individualTrigger, const uint8_t analogProbe, const uint8_t waveformRecording, const uint8_t extrasRecording, const uint8_t timeStampRecording, const uint8_t chargeRecording, const uint8_t externalTriggerMode)
        {
            construct(individualTrigger, analogProbe, waveformRecording, extrasRecording, timeStampRecording, chargeRecording, externalTriggerMode);
        }
        /* Construct from low-level bit mask in line with docs */
        EasyDPPBoardConfigurationHelper(const uint32_t mask)
        {
            constructFromMask(mask);
        }
    }; // class EasyDPPBoardConfigurationHelper


    /**
     * @class EasyAcquisitionControlHelper
     * @brief For user-friendly configuration of Acquisition Control mask.
     *
     * This register manages the acquisition settings.
     *
     * @param EasyAcquisitionControlHelper::startStopMode
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
     * @param EasyAcquisitionControlHelper::acquisitionStartArm
     * Acquisition Start/Arm (default value is 0).\n
     * When bits[1:0] = 00, this bit acts as a Run Start/Stop. When
     * bits[1:0] = 01, 10, 11, this bit arms the acquisition and the
     * actual Start/Stop is controlled by an external signal. Options
     * are:\n
     * 0 = Acquisition STOP (if bits[1:0]=00); Acquisition DISARMED (others)\n
     * 1 = Acquisition RUN (if bits[1:0]=00); Acquisition ARMED (others).
     * @param EasyAcquisitionControlHelper::triggerCountingMode
     * Trigger Counting Mode (default value is 0). Through this bit it
     * is possible to count the reading requests from channels to mother
     * board. The reading requests may come from the following options:\n
     * 0 = accepted triggers from combination of channels\n
     * 1 = triggers from combination of channels, in addition to TRG-IN
     * and SW TRG.
     * @param EasyAcquisitionControlHelper::memoryFullModeSelection
     * Memory Full Mode Selection (default value is 0). Options are:\n
     * 0 = NORMAL. The board is full whenever all buffers are full\n
     * 1 = ONE BUFFER FREE. The board is full whenever Nb-1 buffers are
     * full, where Nb is the overall number of buffers in which the
     * channel memory is divided.
     * @param EasyAcquisitionControlHelper::pLLRefererenceClockSource
     * PLL Reference Clock Source (Desktop/NIM only). Default value is 0.\n
     * Options are:\n
     * 0 = internal oscillator (50 MHz)\n
     * 1 = external clock from front panel CLK-IN connector.\n
     * NOTE: this bit is reserved in case of VME boards.
     * @param EasyAcquisitionControlHelper::lVDSIOBusyEnable
     * LVDS I/O Busy Enable (VME only). Default value is 0.\n
     * The LVDS I/Os can be programmed to accept a Busy signal as input,
     * or to propagate it as output. Options are:\n
     * 0 = disabled\n
     * 1 = enabled.\n
     * NOTE: this bit is supported only by VME boards and meaningful
     * only if the LVDS new features are enabled (bit[8]=1 of register
     * 0x811C). Register 0x81A0 should also be configured for nBusy/nVeto.
     * @param EasyAcquisitionControlHelper::lVDSVetoEnable
     * LVDS I/O Veto Enable (VME only). Default value is 0.\n
     * The LVDS I/Os can be programmed to accept a Veto signal as input,
     * or to transfer it as output. Options are:\n
     * 0 = disabled (default)\n
     * 1 = enabled.\n
     * NOTE: this bit is supported only by VME boards and meaningful
     * only if the LVDS new features are enabled (bit[8]=1 of register
     * 0x811C). Register 0x81A0 should also be configured for nBusy/nVeto.
     * @param EasyAcquisitionControlHelper::lVDSIORunInEnable
     * LVDS I/O RunIn Enable Mode (VME only). Default value is 0.\n
     * The LVDS I/Os can be programmed to accept a RunIn signal as
     * input, or to transfer it as output. Options are:\n
     * 0 = starts on RunIn level (default)\n
     * 1 = starts on RunIn rising edge.\n
     * NOTE: this bit is supported only by VME boards and meaningful
     * only if the LVDS new features are enabled (bit[8]=1 of register
     * 0x811C). Register 0x81A0 must also be configured for nBusy/nVeto.
     */
    class EasyAcquisitionControlHelper : public EasyHelper
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
        virtual const std::string getClassName() const override { return "EasyAcquisitionControlHelper"; };
        /* Construct using default values from docs */
        EasyAcquisitionControlHelper(const uint8_t startStopMode, const uint8_t acquisitionStartArm, const uint8_t triggerCountingMode, const uint8_t memoryFullModeSelection, const uint8_t pLLRefererenceClockSource, const uint8_t lVDSIOBusyEnable, const uint8_t lVDSVetoEnable, const uint8_t lVDSIORunInEnable)
        {
            construct(startStopMode, acquisitionStartArm, triggerCountingMode, memoryFullModeSelection, pLLRefererenceClockSource, lVDSIOBusyEnable, lVDSVetoEnable, lVDSIORunInEnable);
        }
        /* Construct from low-level bit mask in line with docs */
        EasyAcquisitionControlHelper(const uint32_t mask)
        {
            constructFromMask(mask);
        }
    }; // class EasyAcquisitionControlHelper


    /**
     * @class EasyDPPAcquisitionControlHelper
     * @brief For user-friendly configuration of Acquisition Control mask.
     *
     * This register manages the acquisition settings.
     *
     * @param EasyDPPAcquisitionControlHelper::startStopMode
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
     * @param EasyDPPAcquisitionControlHelper::acquisitionStartArm
     * Acquisition Start/Arm (default value is 0).\n
     * When bits[1:0] = 00, this bit acts as a Run Start/Stop. When
     * bits[1:0] = 01, 10, 11, this bit arms the acquisition and the
     * actual Start/Stop is controlled by an external signal. Options
     * are:\n
     * 0 = Acquisition STOP (if bits[1:0]=00); Acquisition DISARMED (others)\n
     * 1 = Acquisition RUN (if bits[1:0]=00); Acquisition ARMED (others).
     * @param EasyDPPAcquisitionControlHelper::triggerCountingMode
     * Trigger Counting Mode (default value is 0). Through this bit it
     * is possible to count the reading requests from channels to mother
     * board. The reading requests may come from the following options:\n
     * 0 = accepted triggers from combination of channels\n
     * 1 = triggers from combination of channels, in addition to TRG-IN
     * and SW TRG.
     * @param EasyDPPAcquisitionControlHelper::pLLRefererenceClockSource
     * PLL Reference Clock Source (Desktop/NIM only). Default value is 0.\n
     * Options are:\n
     * 0 = internal oscillator (50 MHz)\n
     * 1 = external clock from front panel CLK-IN connector.\n
     * NOTE: this bit is reserved in case of VME boards.
     * @param EasyDPPAcquisitionControlHelper::lVDSIOBusyEnable
     * LVDS I/O Busy Enable (VME only). Default value is 0.\n
     * The LVDS I/Os can be programmed to accept a Busy signal as input,
     * or to propagate it as output. Options are:\n
     * 0 = disabled\n
     * 1 = enabled.\n
     * NOTE: this bit is supported only by VME boards and meaningful
     * only if the LVDS new features are enabled (bit[8]=1 of register
     * 0x811C). Register 0x81A0 should also be configured for nBusy/nVeto.
     * @param EasyDPPAcquisitionControlHelper::lVDSVetoEnable
     * LVDS I/O Veto Enable (VME only). Default value is 0.\n
     * The LVDS I/Os can be programmed to accept a Veto signal as input,
     * or to transfer it as output. Options are:\n
     * 0 = disabled (default)\n
     * 1 = enabled.\n
     * NOTE: this bit is supported only by VME boards and meaningful
     * only if the LVDS new features are enabled (bit[8]=1 of register
     * 0x811C). Register 0x81A0 should also be configured for nBusy/nVeto.
     * @param EasyDPPAcquisitionControlHelper::lVDSIORunInEnable
     * LVDS I/O RunIn Enable Mode (VME only). Default value is 0.\n
     * The LVDS I/Os can be programmed to accept a RunIn signal as
     * input, or to transfer it as output. Options are:\n
     * 0 = starts on RunIn level (default)\n
     * 1 = starts on RunIn rising edge.\n
     * NOTE: this bit is supported only by VME boards and meaningful
     * only if the LVDS new features are enabled (bit[8]=1 of register
     * 0x811C). Register 0x81A0 must also be configured for nBusy/nVeto.
     */
    class EasyDPPAcquisitionControlHelper : public EasyHelper
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
        virtual const std::string getClassName() const override { return "EasyDPPAcquisitionControlHelper"; };
        /* Construct using default values from docs */
        EasyDPPAcquisitionControlHelper(const uint8_t startStopMode, const uint8_t acquisitionStartArm, const uint8_t triggerCountingMode, const uint8_t pLLRefererenceClockSource, const uint8_t lVDSIOBusyEnable, const uint8_t lVDSVetoEnable, const uint8_t lVDSIORunInEnable)
        {
            construct(startStopMode, acquisitionStartArm, triggerCountingMode, pLLRefererenceClockSource, lVDSIOBusyEnable, lVDSVetoEnable, lVDSIORunInEnable);
        }
        /* Construct from low-level bit mask in line with docs */
        EasyDPPAcquisitionControlHelper(const uint32_t mask)
        {
            constructFromMask(mask);
        }
    }; // class EasyDPPAcquisitionControlHelper


    /**
     * @class EasyAcquisitionStatusHelper
     * @brief For user-friendly configuration of Acquisition Status mask.
     *
     * This register monitors a set of conditions related to the
     * acquisition status.
     *
     * @param EasyAcquisitionStatusHelper::acquisitionStatus
     * Acquisition Status. It reflects the status of the acquisition and
     * drivers the front panel ’RUN’ LED. Options are:\n
     * 0 = acquisition is stopped (’RUN’ is off)\n
     * 1 = acquisition is running (’RUN’ lits).
     * @param EasyAcquisitionStatusHelper::eventReady
     * Event Ready. Indicates if any events are available for
     * readout. Options are:\n
     * 0 = no event is available for readout\n
     * 1 = at least one event is available for readout.\n
     * NOTE: the status of this bit must be considered when managing the
     * readout from the digitizer.
     * @param EasyAcquisitionStatusHelper::eventFull
     * Event Full. Indicates if at least one channel has reached the
     * FULL condition. Options are:\n
     * 0 = no channel has reached the FULL condition\n
     * 1 = the maximum number of events to be read is reached.
     * @param EasyAcquisitionStatusHelper::clockSource
     * Clock Source. Indicates the clock source status. Options are:\n
     * 0 = internal (PLL uses the internal 50 MHz oscillator as reference)\n
     * 1 = external (PLL uses the external clock on CLK-IN connector as
     * reference).
     * @param EasyAcquisitionStatusHelper::pLLBypassMode
     * PLL Bypass Mode. This bit drives the front panel 'PLL BYPS' LED.\n
     * Options are:\n
     * 0 = PLL bypass mode is not ac ve ('PLL BYPS' is off)\n
     * 1 = PLL bypass mode is ac ve and the VCXO frequency directly
     * drives the clock distribution chain ('PLL BYPS' lits).\n
     * WARNING: before operating in PLL Bypass Mode, it is recommended
     * to contact CAEN for feasibility.
     * @param EasyAcquisitionStatusHelper::pLLUnlockDetect
     * PLL Unlock Detect. This bit flags a PLL unlock condition. Options
     * are:\n
     * 0 = PLL has had an unlock condition since the last register read
     * access\n
     * 1 = PLL has not had any unlock condition since the last register
     * read access.\n
     * NOTE: flag can be restored to 1 via read access to register 0xEF04.
     * @param EasyAcquisitionStatusHelper::boardReady
     * Board Ready. This flag indicates if the board is ready for
     * acquisition (PLL and ADCs are correctly synchronised). Options
     * are:\n
     * 0 = board is not ready to start the acquisition\n
     * 1 = board is ready to start the acquisition.\n
     * NOTE: this bit should be checked a er software reset to ensure
     * that the board will enter immediately in run mode a er the RUN
     * mode setting; otherwise, a latency between RUN mode setting and
     * Acquisition start might occur.
     * @param EasyAcquisitionStatusHelper::s_IN
     * S-IN (VME boards) or GPI (DT/NIM boards) Status. Reads the
     * current logical level on S-IN (GPI) front panel connector.
     * @param EasyAcquisitionStatusHelper::tRG_IN
     * TRG-IN Status. Reads the current logical level on TRG-IN front
     * panel connector.
     */
    class EasyAcquisitionStatusHelper : public EasyHelper
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
        virtual const std::string getClassName() const override { return "EasyAcquisitionStatusHelper"; };
        /* Construct using default values from docs */
        EasyAcquisitionStatusHelper(const uint8_t acquisitionStatus, const uint8_t eventReady, const uint8_t eventFull, const uint8_t clockSource, const uint8_t pLLBypassMode, const uint8_t pLLUnlockDetect, const uint8_t boardReady, const uint8_t s_IN, const uint8_t tRG_IN)
        {
            construct(acquisitionStatus, eventReady, eventFull, clockSource, pLLBypassMode, pLLUnlockDetect, boardReady, s_IN, tRG_IN);
        }
        /* Construct from low-level bit mask in line with docs */
        EasyAcquisitionStatusHelper(const uint32_t mask)
        {
            constructFromMask(mask);
        }
    }; // class EasyAcquisitionStatusHelper


    /**
     * @class EasyDPPAcquisitionStatusHelper
     * @brief For user-friendly configuration of Acquisition Status mask.
     *
     * This register monitors a set of conditions related to the
     * acquisition status.
     *
     * @param EasyDPPAcquisitionStatusHelper::acquisitionStatus
     * Acquisition Status. It reflects the status of the acquisition and
     * drivers the front panel ’RUN’ LED. Options are:\n
     * 0 = acquisition is stopped (’RUN’ is off)\n
     * 1 = acquisition is running (’RUN’ lits).
     * @param EasyDPPAcquisitionStatusHelper::eventReady
     * Event Ready. Indicates if any events are available for
     * readout. Options are:\n
     * 0 = no event is available for readout\n
     * 1 = at least one event is available for readout.\n
     * NOTE: the status of this bit must be considered when managing the
     * readout from the digitizer.
     * @param EasyDPPAcquisitionStatusHelper::eventFull
     * Event Full. Indicates if at least one channel has reached the
     * FULL condition. Options are:\n
     * 0 = no channel has reached the FULL condition\n
     * 1 = the maximum number of events to be read is reached.
     * @param EasyDPPAcquisitionStatusHelper::clockSource
     * Clock Source. Indicates the clock source status. Options are:\n
     * 0 = internal (PLL uses the internal 50 MHz oscillator as reference)\n
     * 1 = external (PLL uses the external clock on CLK-IN connector as
     * reference).
     * @param EasyDPPAcquisitionStatusHelper::pLLUnlockDetect
     * PLL Unlock Detect. This bit flags a PLL unlock condition. Options
     * are:\n
     * 0 = PLL has had an unlock condition since the last register read
     * access\n
     * 1 = PLL has not had any unlock condition since the last register
     * read access.\n
     * NOTE: flag can be restored to 1 via read access to register 0xEF04.
     * @param EasyDPPAcquisitionStatusHelper::boardReady
     * Board Ready. This flag indicates if the board is ready for
     * acquisition (PLL and ADCs are correctly synchronised). Options
     * are:\n
     * 0 = board is not ready to start the acquisition\n
     * 1 = board is ready to start the acquisition.\n
     * NOTE: this bit should be checked a er software reset to ensure
     * that the board will enter immediately in run mode a er the RUN
     * mode setting; otherwise, a latency between RUN mode setting and
     * Acquisition start might occur.
     * @param EasyDPPAcquisitionStatusHelper::s_IN
     * S-IN (VME boards) or GPI (DT/NIM boards) Status. Reads the
     * current logical level on S-IN (GPI) front panel connector.
     * @param EasyDPPAcquisitionStatusHelper::tRG_IN
     * TRG-IN Status. Reads the current logical level on TRG-IN front
     * panel connector.
     */
    class EasyDPPAcquisitionStatusHelper : public EasyHelper
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
        virtual const std::string getClassName() const override { return "EasyDPPAcquisitionStatusHelper"; };
        /* Construct using default values from docs */
        EasyDPPAcquisitionStatusHelper(const uint8_t acquisitionStatus, const uint8_t eventReady, const uint8_t eventFull, const uint8_t clockSource, const uint8_t pLLUnlockDetect, const uint8_t boardReady, const uint8_t s_IN, const uint8_t tRG_IN)
        {
            construct(acquisitionStatus, eventReady, eventFull, clockSource, pLLUnlockDetect, boardReady, s_IN, tRG_IN);
        }
        /* Construct from low-level bit mask in line with docs */
        EasyDPPAcquisitionStatusHelper(const uint32_t mask)
        {
            constructFromMask(mask);
        }
    }; // class EasyDPPAcquisitionStatusHelper


    /**
     * @class EasyGlobalTriggerMaskHelper
     * @brief For user-friendly configuration of Global Trigger Mask.
     *
     * This register sets which signal can contribute to the global
     * trigger generation.
     *
     * @param EasyGlobalTriggerMaskHelper::groupTriggerMask
     * Bit n corresponds to the trigger request from group n that
     * participates to the global trigger generation (n = 0,...,3 for DT
     * and NIM; n = 0,...,7 for VME boards). Options are:\n
     * 0 = trigger request is not sensed for global trigger generation\n
     * 1 = trigger request participates in the global trigger generation.\n
     * NOTE: in case of DT and NIMboards, only bits[3:0] are meaningful,
     * while bits[7:4] are reserved.
     * @param EasyGlobalTriggerMaskHelper::majorityCoincidenceWindow
     * Majority Coincidence Window. Sets the me window (in units of the
     * trigger clock) for the majority coincidence. Majority level must
     * be set different from 0 through bits[26:24].
     * @param EasyGlobalTriggerMaskHelper::majorityLevel
     * Majority Level. Sets the majority level for the global trigger
     * generation. For a level m, the trigger fires when at least m+1 of
     * the enabled trigger requests (bits[7:0] or [3:0]) are
     * over-threshold inside the majority coincidence window
     * (bits[23:20]).\n
     * NOTE: the majority level must be smaller than the number of
     * trigger requests enabled via bits[7:0] mask (or [3:0]).
     * @param EasyGlobalTriggerMaskHelper::lVDSTrigger
     * LVDS Trigger (VME boards only). When enabled, the trigger from
     * LVDS I/O participates to the global trigger generation (in logic
     * OR). Options are:\n
     * 0 = disabled\n
     * 1 = enabled.
     * @param EasyGlobalTriggerMaskHelper::externalTrigger
     * External Trigger (default value is 1). When enabled, the external
     * trigger on TRG-IN participates to the global trigger generation
     * in logic OR with the other enabled signals. Options are:\n
     * 0 = disabled\n
     * 1 = enabled.
     * @param EasyGlobalTriggerMaskHelper::softwareTrigger
     * Software Trigger (default value is 1). When enabled, the software
     * trigger participates to the global trigger signal generation in
     * logic OR with the other enabled signals. Options are:\n
     * 0 = disabled\n
     * 1 = enabled.
     */
    class EasyGlobalTriggerMaskHelper : public EasyHelper
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
        virtual const std::string getClassName() const override { return "EasyGlobalTriggerMaskHelper"; };
        /* Construct using default values from docs */
        EasyGlobalTriggerMaskHelper(const uint8_t groupTriggerMask, const uint8_t majorityCoincidenceWindow, const uint8_t majorityLevel, const uint8_t lVDSTrigger, const uint8_t externalTrigger, const uint8_t softwareTrigger)
        {
            construct(groupTriggerMask, majorityCoincidenceWindow, majorityLevel, lVDSTrigger, externalTrigger, softwareTrigger);
        }
        /* Construct from low-level bit mask in line with docs */
        EasyGlobalTriggerMaskHelper(const uint32_t mask)
        {
            constructFromMask(mask);
        }
    }; // class EasyGlobalTriggerMaskHelper


    /**
     * @class EasyDPPGlobalTriggerMaskHelper
     * @brief For user-friendly configuration of Global Trigger Mask.
     *
     * This register sets which signal can contribute to the global
     * trigger generation.
     *
     * @param EasyDPPGlobalTriggerMaskHelper::lVDSTrigger
     * LVDS Trigger (VME boards only). When enabled, the trigger from
     * LVDS I/O participates to the global trigger generation (in logic
     * OR). Options are:\n
     * 0 = disabled\n
     * 1 = enabled.
     * @param EasyDPPGlobalTriggerMaskHelper::externalTrigger
     * External Trigger (default value is 1). When enabled, the external
     * trigger on TRG-IN participates to the global trigger generation
     * in logic OR with the other enabled signals. Options are:\n
     * 0 = disabled\n
     * 1 = enabled.
     * @param EasyDPPGlobalTriggerMaskHelper::softwareTrigger
     * Software Trigger (default value is 1). When enabled, the software
     * trigger participates to the global trigger signal generation in
     * logic OR with the other enabled signals. Options are:\n
     * 0 = disabled\n
     * 1 = enabled.
     */
    class EasyDPPGlobalTriggerMaskHelper : public EasyHelper
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
        virtual const std::string getClassName() const override { return "EasyDPPGlobalTriggerMaskHelper"; };
        /* Construct using default values from docs */
        EasyDPPGlobalTriggerMaskHelper(const uint8_t lVDSTrigger, const uint8_t externalTrigger, const uint8_t softwareTrigger)
        {
            construct(lVDSTrigger, externalTrigger, softwareTrigger);
        }
        /* Construct from low-level bit mask in line with docs */
        EasyDPPGlobalTriggerMaskHelper(const uint32_t mask)
        {
            constructFromMask(mask);
        }
    }; // class EasyDPPGlobalTriggerMaskHelper


    /**
     * @class EasyFrontPanelTRGOUTEnableMaskHelper
     * @brief For user-friendly configuration of Front Panel TRG-OUT Enable Mask.
     *
     * This register sets which signal can contribute to
     * generate the signal on the front panel TRG-OUT LEMO connector
     * (GPO in case of DT and NIM boards).
     *
     * @param EasyFrontPanelTRGOUTEnableMaskHelper::groupTriggerMask
     * This mask sets the trigger requests participating in the TRG-OUT
     * (GPO). Bit n corresponds to the trigger request from group n.\n
     * Options are:\n
     * 0 = Trigger request does not participate to the TRG-OUT (GPO) signal\n
     * 1 = Trigger request participates to the TRG-OUT (GPO) signal.\n
     * NOTE: in case of DT and NIM boards, only bits[3:0] are meaningful
     * while bis[7:4] are reserved.
     * @param EasyFrontPanelTRGOUTEnableMaskHelper::tRGOUTGenerationLogic
     * TRG-OUT (GPO) Generation Logic. The enabled trigger requests
     * (bits [7:0] or [3:0]) can be combined to generate the TRG-OUT
     * (GPO) signal. Options are:\n
     * 00 = OR\n
     * 01 = AND\n
     * 10 = Majority\n
     * 11 = reserved.
     * @param EasyFrontPanelTRGOUTEnableMaskHelper::majorityLevel
     * Majority Level. Sets the majority level for the TRG-OUT (GPO)
     * signal generation. Allowed level values are between 0 and 7 for
     * VME boards, and between 0 and 3 for DT and NIM boards. For a
     * level m, the trigger fires when at least m+1 of the trigger
     * requests are generated by the enabled channels (bits [7:0] or [3:0]).
     * @param EasyFrontPanelTRGOUTEnableMaskHelper::lVDSTriggerEnable
     * LVDS Trigger Enable (VME boards only). If the LVDS I/Os are
     * programmed as outputs, they can participate in the TRG-OUT (GPO)
     * signal generation. They are in logic OR with the other enabled
     * signals. Options are:\n
     * 0 = disabled\n
     * 1 = enabled.
     * @param EasyFrontPanelTRGOUTEnableMaskHelper::externalTrigger
     * External Trigger. When enabled, the external trigger on TRG-IN
     * can participate in the TRG-OUT (GPO) signal generation in logic
     * OR with the other enabled signals. Options are:\n
     * 0 = disabled\n
     * 1 = enabled.
     * @param EasyFrontPanelTRGOUTEnableMaskHelper::softwareTrigger
     * Software Trigger. When enabled, the software trigger can
     * participate in the TRG-OUT (GPO) signal generation in logic OR
     * with the other enabled signals. Options are:\n
     * 0 = disabled\n
     * 1 = enabled.
     */
    class EasyFrontPanelTRGOUTEnableMaskHelper : public EasyHelper
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
        virtual const std::string getClassName() const override { return "EasyFrontPanelTRGOUTEnableMaskHelper"; };
        /* Construct using default values from docs */
        EasyFrontPanelTRGOUTEnableMaskHelper(const uint8_t groupTriggerMask, const uint8_t tRGOUTGenerationLogic, const uint8_t majorityLevel, const uint8_t lVDSTriggerEnable, const uint8_t externalTrigger, const uint8_t softwareTrigger)
        {
            construct(groupTriggerMask, tRGOUTGenerationLogic, majorityLevel, lVDSTriggerEnable, externalTrigger, softwareTrigger);
        }
        /* Construct from low-level bit mask in line with docs */
        EasyFrontPanelTRGOUTEnableMaskHelper(const uint32_t mask)
        {
            constructFromMask(mask);
        }
    }; // class EasyFrontPanelTRGOUTEnableMaskHelper


    /**
     * @class EasyDPPFrontPanelTRGOUTEnableMaskHelper
     * @brief For user-friendly configuration of Front Panel TRG-OUT Enable Mask.
     *
     * This register sets which signal can contribute to
     * generate the signal on the front panel TRG-OUT LEMO connector
     * (GPO in case of DT and NIM boards).
     *
     * @param EasyDPPFrontPanelTRGOUTEnableMaskHelper::lVDSTriggerEnable
     * LVDS Trigger Enable (VME boards only). If the LVDS I/Os are
     * programmed as outputs, they can participate in the TRG-OUT (GPO)
     * signal generation. They are in logic OR with the other enabled
     * signals. Options are:\n
     * 0 = disabled\n
     * 1 = enabled.
     * @param EasyDPPFrontPanelTRGOUTEnableMaskHelper::externalTrigger
     * External Trigger. When enabled, the external trigger on TRG-IN
     * can participate in the TRG-OUT (GPO) signal generation in logic
     * OR with the other enabled signals. Options are:\n
     * 0 = disabled\n
     * 1 = enabled.
     * @param EasyDPPFrontPanelTRGOUTEnableMaskHelper::softwareTrigger
     * Software Trigger. When enabled, the software trigger can
     * participate in the TRG-OUT (GPO) signal generation in logic OR
     * with the other enabled signals. Options are:\n
     * 0 = disabled\n
     * 1 = enabled.
     */
    class EasyDPPFrontPanelTRGOUTEnableMaskHelper : public EasyHelper
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
        virtual const std::string getClassName() const override { return "EasyDPPFrontPanelTRGOUTEnableMaskHelper"; };
        /* Construct using default values from docs */
        EasyDPPFrontPanelTRGOUTEnableMaskHelper(const uint8_t lVDSTriggerEnable, const uint8_t externalTrigger, const uint8_t softwareTrigger)
        {
            construct(lVDSTriggerEnable, externalTrigger, softwareTrigger);
        }
        /* Construct from low-level bit mask in line with docs */
        EasyDPPFrontPanelTRGOUTEnableMaskHelper(const uint32_t mask)
        {
            constructFromMask(mask);
        }
    }; // class EasyDPPFrontPanelTRGOUTEnableMaskHelper


    /**
     * @class EasyFrontPanelIOControlHelper
     * @brief For user-friendly configuration of Acquisition Control mask.
     *
     * This register manages the front panel I/O connectors. Default
     * value is 0x000000.
     *
     * @param EasyFrontPanelIOControlHelper::lEMOIOElectricalLevel
     * LEMO I/Os Electrical Level. This bit sets the electrical level of
     * the front panel LEMO connectors: TRG-IN, TRG-OUT (GPO in case of
     * DT and NIM boards), S-IN (GPI in case of DT and NIM
     * boards). Options are:\n
     * 0 = NIM I/O levels\n
     * 1 = TTL I/O levels.
     * @param EasyFrontPanelIOControlHelper::tRGOUTEnable
     * TRG-OUT Enable (VME boards only). Enables the TRG-OUT LEMO front
     * panel connector. Options are:\n
     * 0 = enabled (default)\n
     * 1 = high impedance.\n
     * NOTE: this bit is reserved in case of DT and NIM boards.
     * @param EasyFrontPanelIOControlHelper::lVDSIODirectionFirst
     * LVDS I/O [3:0] Direction (VME boards only). Sets the direction of
     * the signals on the first 4-pin group of the LVDS I/O
     * connector. Options are:\n
     * 0 = input\n
     * 1 = output.\n
     * NOTE: this bit is reserved in case of DT and NIM boards.
     * @param EasyFrontPanelIOControlHelper::lVDSIODirectionSecond
     * LVDS I/O [7:4] Direction (VME boards only). Sets the direction of
     * the second 4-pin group of the LVDS I/O connector. Options are:\n
     * 0 = input\n
     * 1 = output.\n
     * NOTE: this bit is reserved in case of DT and NIM boards.
     * @param EasyFrontPanelIOControlHelper::lVDSIODirectionThird
     * LVDS I/O [11:8] Direction (VME boards only). Sets the direction of
     * the third 4-pin group of the LVDS I/O connector. Options are:\n
     * 0 = input\n
     * 1 = output.\n
     * NOTE: this bit is reserved in case of DT and NIM boards.
     * @param EasyFrontPanelIOControlHelper::lVDSIODirectionFourth
     * LVDS I/O [15:12] Direction (VME boards only). Sets the direction of
     * the fourth 4-pin group of the LVDS I/O connector. Options are:\n
     * 0 = input\n
     * 1 = output.\n
     * NOTE: this bit is reserved in case of DT and NIM boards.
     * @param EasyFrontPanelIOControlHelper::lVDSIOSignalConfiguration
     * LVDS I/O Signal Configuration (VME boards and LVDS I/O old
     * features only). This configuration must be enabled through bit[8]
     * set to 0. Options are:\n
     * 00 = general purpose I/O\n
     * 01 = programmed I/O\n
     * 10 = pattern mode: LVDS signals are input and their value is
     * written into the header PATTERN field\n
     * 11 = reserved.\n
     * NOTE: these bits are reserved in case of DT and NIM boards.
     * @param EasyFrontPanelIOControlHelper::lVDSIONewFeaturesSelection
     * LVDS I/O New Features Selection (VME boards only). Options are:\n
     * 0 = LVDS old features\n
     * 1 = LVDS new features.\n
     * The new features options can be configured through register
     * 0x81A0. Please, refer to the User Manual for all details.\n
     * NOTE: LVDS I/O New Features option is valid from motherboard
     * firmware revision 3.8 on.\n
     * NOTE: this bit is reserved in case of DT and NIM boards.
     * @param EasyFrontPanelIOControlHelper::lVDSIOPatternLatchMode
     * LVDS I/Os Pattern Latch Mode (VME boards only). Options are:\n
     * 0 = Pattern (i.e. 16-pin LVDS status) is latched when the
     * (internal) global trigger is sent to channels, in consequence of
     * an external trigger. It accounts for post-trigger settings and
     * input latching delays\n
     * 1 = Pattern (i.e. 16-pin LVDS status) is latched when an external
     * trigger arrives.\n
     * NOTE: this bit is reserved in case of DT and NIM boards.
     * @param EasyFrontPanelIOControlHelper::tRGINControl
     * TRG-IN control. The board trigger logic can be synchronized
     * either with the edge of the TRG-IN signal, or with its whole
     * duration.\n
     * Note: this bit must be used in conjunction with bit[11] =
     * 0. Options are:\n
     * 0 = trigger is synchronized with the edge of the TRG-IN signal\n
     * 1 = trigger is synchronized with the whole duration of the TRG-IN
     * signal.
     * @param EasyFrontPanelIOControlHelper::tRGINMezzanines
     * TRG-IN to Mezzanines (channels). Options are:\n
     * 0 = TRG-IN signal is processed by the motherboard and sent to
     * mezzanine (default). The trigger logic is then synchronized with
     * TRG-IN\n
     * 1 = TRG-IN is directly sent to the mezzanines with no mother
     * board processing nor delay. This option can be useful when TRG-IN
     * is used to veto the acquisition.\n
     * NOTE: if this bit is set to 1, then bit[10] is ignored.
     * @param EasyFrontPanelIOControlHelper::forceTRGOUT
     * Force TRG-OUT (GPO). This bit can force TRG-OUT (GPO in case of
     * DT and NIM boards) test logical level if bit[15] = 1. Options
     * are:\n
     * 0 = Force TRG-OUT (GPO) to 0\n
     * 1 = Force TRG-OUT (GPO) to 1.
     * @param EasyFrontPanelIOControlHelper::tRGOUTMode
     * TRG-OUT (GPO) Mode. Options are:\n
     * 0 = TRG-OUT (GPO) is an internal signal (according to bits[17:16])\n
     * 1= TRG-OUT (GPO) is a test logic level set via bit[14].
     * @param EasyFrontPanelIOControlHelper::tRGOUTModeSelection
     * TRG-OUT (GPO) Mode Selection. Options are:\n
     * 00 = Trigger: TRG-OUT/GPO propagates the internal trigger sources
     * according to register 0x8110\n
     * 01 = Motherboard Probes: TRG-OUT/GPO is used to propagate signals
     * of the motherboards according to bits[19:18]\n
     * 10 = Channel Probes: TRG-OUT/GPO is used to propagate signals of
     * the mezzanines (Channel Signal Virtual Probe)\n
     * 11 = S-IN (GPI) propagation.
     * @param EasyFrontPanelIOControlHelper::motherboardVirtualProbeSelection
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
     * @param EasyFrontPanelIOControlHelper::motherboardVirtualProbePropagation
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
     * @param EasyFrontPanelIOControlHelper::patternConfiguration
     * Pattern Configuration. Configures the information given by the
     * 16-bit PATTERN field in the header of the event format (VME
     * only). Option are:\n
     * 00 = PATTERN: 16-bit pattern latched on the 16 LVDS signals as
     * one trigger arrives (default)\n
     * Other options are reserved.
     */
    class EasyFrontPanelIOControlHelper : public EasyHelper
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
        virtual const std::string getClassName() const override { return "EasyFrontPanelIOControlHelper"; };
        /* Construct using default values from docs */
        EasyFrontPanelIOControlHelper(const uint8_t lEMOIOElectricalLevel, const uint8_t tRGOUTEnable, const uint8_t lVDSIODirectionFirst, const uint8_t lVDSIODirectionSecond, const uint8_t lVDSIODirectionThird, const uint8_t lVDSIODirectionFourth, const uint8_t lVDSIOSignalConfiguration, const uint8_t lVDSIONewFeaturesSelection, const uint8_t lVDSIOPatternLatchMode, const uint8_t tRGINControl, const uint8_t tRGINMezzanines, const uint8_t forceTRGOUT, const uint8_t tRGOUTMode, const uint8_t tRGOUTModeSelection, const uint8_t motherboardVirtualProbeSelection, const uint8_t motherboardVirtualProbePropagation, const uint8_t patternConfiguration)
        {
            construct(lEMOIOElectricalLevel, tRGOUTEnable, lVDSIODirectionFirst, lVDSIODirectionSecond, lVDSIODirectionThird, lVDSIODirectionFourth, lVDSIOSignalConfiguration, lVDSIONewFeaturesSelection, lVDSIOPatternLatchMode, tRGINControl, tRGINMezzanines, forceTRGOUT, tRGOUTMode, tRGOUTModeSelection, motherboardVirtualProbeSelection, motherboardVirtualProbePropagation, patternConfiguration);
        }
        /* Construct from low-level bit mask in line with docs */
        EasyFrontPanelIOControlHelper(const uint32_t mask)
        {
            constructFromMask(mask);
        }
    }; // class EasyFrontPanelIOControlHelper


    /**
     * @class EasyDPPFrontPanelIOControlHelper
     * @brief For user-friendly configuration of Acquisition Control mask.
     *
     * NOTE: Identical to EasyFrontPanelIOControlHelper
     */
    class EasyDPPFrontPanelIOControlHelper : public EasyFrontPanelIOControlHelper
    {
    /* Just inherit everything else from parent class */
    public:
        virtual const std::string getClassName() const override { return "EasyDPPFrontPanelIOControlHelper"; };
        EasyDPPFrontPanelIOControlHelper(const uint8_t lEMOIOElectricalLevel, const uint8_t tRGOUTEnable, const uint8_t lVDSIODirectionFirst, const uint8_t lVDSIODirectionSecond, const uint8_t lVDSIODirectionThird, const uint8_t lVDSIODirectionFourth, const uint8_t lVDSIOSignalConfiguration, const uint8_t lVDSIONewFeaturesSelection, const uint8_t lVDSIOPatternLatchMode, const uint8_t tRGINControl, const uint8_t tRGINMezzanines, const uint8_t forceTRGOUT, const uint8_t tRGOUTMode, const uint8_t tRGOUTModeSelection, const uint8_t motherboardVirtualProbeSelection, const uint8_t motherboardVirtualProbePropagation, const uint8_t patternConfiguration) : EasyFrontPanelIOControlHelper(lEMOIOElectricalLevel, tRGOUTEnable, lVDSIODirectionFirst, lVDSIODirectionSecond, lVDSIODirectionThird, lVDSIODirectionFourth, lVDSIOSignalConfiguration, lVDSIONewFeaturesSelection, lVDSIOPatternLatchMode, tRGINControl, tRGINMezzanines, forceTRGOUT, tRGOUTMode, tRGOUTModeSelection, motherboardVirtualProbeSelection, motherboardVirtualProbePropagation, patternConfiguration) {};
        EasyDPPFrontPanelIOControlHelper(uint32_t mask) : EasyFrontPanelIOControlHelper(mask) {};
    }; // class EasyDPPFrontPanelIOControlHelper


    /**
     * @class EasyROCFPGAFirmwareRevisionHelper
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
     * @param EasyROCFPGAFirmwareRevisionHelper::minorRevisionNumber
     * ROC Firmware Minor Revision Number (Y).
     * @param EasyROCFPGAFirmwareRevisionHelper::majorRevisionNumber
     * ROC Firmware Major Revision Number (X).
     * @param EasyROCFPGAFirmwareRevisionHelper::revisionDate
     * ROC Firmware Revision Date (Y/M/DD).
     */
    class EasyROCFPGAFirmwareRevisionHelper : public EasyHelper
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
        virtual const std::string getClassName() const override { return "EasyROCFPGAFirmwareRevisionHelper"; };
        /* Construct using default values from docs */
        /* We allow both clamped and individual revision date format here */
        EasyROCFPGAFirmwareRevisionHelper(const uint8_t minorRevisionNumber, const uint8_t majorRevisionNumber, const uint16_t revisionDate)
        {
            const uint8_t revisionDayLower = (const uint8_t)(revisionDate & 0x7);
            const uint8_t revisionDayUpper = (const uint8_t)((revisionDate >> 4) & 0x7);
            const uint8_t revisionMonth = (const uint8_t)((revisionDate >> 8) & 0x7);
            const uint8_t revisionYear = (const uint8_t)((revisionDate >> 12) & 0x7);
            construct(minorRevisionNumber, majorRevisionNumber, revisionDayLower, revisionDayUpper, revisionMonth, revisionYear);
        }
        EasyROCFPGAFirmwareRevisionHelper(const uint8_t minorRevisionNumber, const uint8_t majorRevisionNumber, const uint8_t revisionDayLower, const uint8_t revisionDayUpper, const uint8_t revisionMonth, const uint8_t revisionYear)
        {
            construct(minorRevisionNumber, majorRevisionNumber, revisionDayLower, revisionDayUpper, revisionMonth, revisionYear);
        }
        /* Construct from low-level bit mask in line with docs */
        EasyROCFPGAFirmwareRevisionHelper(const uint32_t mask)
        {
            constructFromMask(mask);
        }
    }; // class EasyROCFPGAFirmwareRevisionHelper


    /**
     * @class EasyDPPROCFPGAFirmwareRevisionHelper
     * @brief For user-friendly configuration of ROC FPGA Firmware Revision.
     *
     * NOTE: identical to EasyROCFPGAFirmwareRevisionHelper
     */
    class EasyDPPROCFPGAFirmwareRevisionHelper : public EasyHelper
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
        virtual const std::string getClassName() const override { return "EasyDPPROCFPGAFirmwareRevisionHelper"; };
        /* Construct using default values from docs */
        /* We allow both clamped and individual revision date format here */
        EasyDPPROCFPGAFirmwareRevisionHelper(const uint8_t minorRevisionNumber, const uint8_t majorRevisionNumber, const uint16_t revisionDate)
        {
            const uint8_t revisionDayLower = (const uint8_t)(revisionDate & 0x7);
            const uint8_t revisionDayUpper = (const uint8_t)((revisionDate >> 4) & 0x7);
            const uint8_t revisionMonth = (const uint8_t)((revisionDate >> 8) & 0x7);
            const uint8_t revisionYear = (const uint8_t)((revisionDate >> 12) & 0x7);
            construct(minorRevisionNumber, majorRevisionNumber, revisionDayLower, revisionDayUpper, revisionMonth, revisionYear);
        }
        EasyDPPROCFPGAFirmwareRevisionHelper(const uint8_t firmwareRevisionNumber, const uint8_t firmwareDPPCode, const uint8_t buildDayLower, const uint8_t buildDayUpper, const uint8_t buildMonth, const uint8_t buildYear)
        {
            construct(firmwareRevisionNumber, firmwareDPPCode, buildDayLower, buildDayUpper, buildMonth, buildYear);
        }
        /* Construct from low-level bit mask in line with docs */
        EasyDPPROCFPGAFirmwareRevisionHelper(const uint32_t mask)
        {
            constructFromMask(mask);
        }
    }; // class EasyDPPROCFPGAFirmwareRevisionHelper


    /**
     * @class EasyFanSpeedControlHelper
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
     * @param EasyFanSpeedControlHelper::fanSpeedMode
     * Fan Speed Mode. Options are:\n
     * 0 = slow speed or automatic speed tuning\n
     * 1 = high speed.
     */
    class EasyFanSpeedControlHelper : public EasyHelper
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
        virtual const std::string getClassName() const override { return "EasyFanSpeedControlHelper"; };
        /* Construct using default values from docs */
        EasyFanSpeedControlHelper(const uint8_t fanSpeedMode=0)
        {
            construct(fanSpeedMode);
        }
        /* Construct from low-level bit mask in line with docs */
        EasyFanSpeedControlHelper(const uint32_t mask)
        {
            constructFromMask(mask);
        }
    }; // class EasyFanSpeedControlHelper


    /**
     * @class EasyDPPFanSpeedControlHelper
     * @brief For user-friendly configuration of Fan Speed Control mask.
     *
     * NOTE: identical to EasyFanSpeedControlHelper
     */
    class EasyDPPFanSpeedControlHelper : public EasyFanSpeedControlHelper
    {
    /* Just inherit everything else from parent class */
    public:
        virtual const std::string getClassName() const override { return "EasyDPPFanSpeedControlHelper"; };
        EasyDPPFanSpeedControlHelper(uint8_t fanSpeedMode) : EasyFanSpeedControlHelper(fanSpeedMode) {};
        EasyDPPFanSpeedControlHelper(uint32_t mask) : EasyFanSpeedControlHelper(mask) {};
    }; // class EasyDPPFanSpeedControlHelper


    /**
     * @class EasyReadoutControlHelper
     * @brief For user-friendly configuration of Readout Control mask.
     *
     * This register is mainly intended for VME boards, anyway some bits
     * are applicable also for DT and NIM boards.
     *
     * @param EasyReadoutControlHelper::vMEInterruptLevel
     * VME Interrupt Level (VME boards only). Options are:\n
     * 0 = VME interrupts are disabled\n
     * 1,..,7 = sets the VME interrupt level.\n
     * NOTE: these bits are reserved in case of DT and NIM boards.
     * @param EasyReadoutControlHelper::opticalLinkInterruptEnable
     * Optical Link Interrupt Enable. Op ons are:\n
     * 0 = Optical Link interrupts are disabled\n
     * 1 = Optical Link interrupts are enabled.
     * @param EasyReadoutControlHelper::vMEBusErrorEventAlignedEnable
     * VME Bus Error / Event Aligned Readout Enable (VME boards
     * only). Options are:\n
     * 0 = VME Bus Error / Event Aligned Readout disabled (the module
     * sends a DTACK signal until the CPU inquires the module)\n
     * 1 = VME Bus Error / Event Aligned Readout enabled (the module is
     * enabled either to generate a Bus Error to finish a block transfer
     * or during the empty buffer readout in D32).\n
     * NOTE: this bit is reserved (must be 1) in case of DT and NIM boards.
     * @param EasyReadoutControlHelper::vMEAlign64Mode
     * VME Align64 Mode (VME boards only). Options are:\n
     * 0 = 64-bit aligned readout mode disabled\n
     * 1 = 64-bit aligned readout mode enabled.\n
     * NOTE: this bit is reserved (must be 0) in case of DT and NIM boards.
     * @param EasyReadoutControlHelper::vMEBaseAddressRelocation
     * VME Base Address Relocation (VME boards only). Options are:\n
     * 0 = Address Relocation disabled (VME Base Address is set by the on-board
     * rotary switches)\n
     * 1 = Address Relocation enabled (VME Base Address is set by
     * register 0xEF0C).\n
     * NOTE: this bit is reserved (must be 0) in case of DT and NIM boards.
     * @param EasyReadoutControlHelper::interruptReleaseMode
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
     * @param EasyReadoutControlHelper::extendedBlockTransferEnable
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
    class EasyReadoutControlHelper : public EasyHelper
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
        virtual const std::string getClassName() const override { return "EasyReadoutControlHelper"; };
        /* Construct using default values from docs */
        EasyReadoutControlHelper(const uint8_t vMEInterruptLevel, const uint8_t opticalLinkInterruptEnable, const uint8_t vMEBusErrorEventAlignedEnable, const uint8_t vMEAlign64Mode, const uint8_t vMEBaseAddressRelocation, const uint8_t interruptReleaseMode, const uint8_t extendedBlockTransferEnable)
        {
            construct(vMEInterruptLevel, opticalLinkInterruptEnable, vMEBusErrorEventAlignedEnable, vMEAlign64Mode, vMEBaseAddressRelocation, interruptReleaseMode, extendedBlockTransferEnable);
        }
        /* Construct from low-level bit mask in line with docs */
        EasyReadoutControlHelper(const uint32_t mask)
        {
            constructFromMask(mask);
        }
    }; // class EasyReadoutControlHelper


    /**
     * @class EasyDPPReadoutControlHelper
     * @brief For user-friendly configuration of Readout Control mask.
     *
     * NOTE: identical to EasyReadoutControlHelper
     */
    class EasyDPPReadoutControlHelper : public EasyReadoutControlHelper
    {
    /* Just inherit everything else from parent class */
    public:
        virtual const std::string getClassName() const override { return "EasyDPPReadoutControlHelper"; };
        EasyDPPReadoutControlHelper(const uint8_t vMEInterruptLevel, const uint8_t opticalLinkInterruptEnable, const uint8_t vMEBusErrorEventAlignedEnable, const uint8_t vMEAlign64Mode, const uint8_t vMEBaseAddressRelocation, const uint8_t interruptReleaseMode, const uint8_t extendedBlockTransferEnable) : EasyReadoutControlHelper(vMEInterruptLevel, opticalLinkInterruptEnable, vMEBusErrorEventAlignedEnable, vMEAlign64Mode, vMEBaseAddressRelocation, interruptReleaseMode, extendedBlockTransferEnable){};
        EasyDPPReadoutControlHelper(const uint32_t mask) : EasyReadoutControlHelper(mask) {};
    }; // class EasyDPPReadoutControlHelper


    /**
     * @class EasyReadoutStatusHelper
     * @brief For user-friendly configuration of Readout Status mask.
     *
     * This register contains informa on related to the readout.
     *
     * @param EasyReadoutStatusHelper::eventReady
     * Event Ready. Indicates if there are events stored ready for
     * readout. Options are:\n
     * 0 = no data ready\n
     * 1 = event ready.
     * @param EasyReadoutStatusHelper::outputBufferStatus
     * Output Buffer Status. Indicates if the Output Buffer is in Full
     * condition. Options are:\n
     * 0 = the Output Buffer is not FULL\n
     * 1 = the Output Buffer is FULL.
     * @param EasyReadoutStatusHelper::busErrorSlaveTerminated
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
    class EasyReadoutStatusHelper : public EasyHelper
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
                {"busErrorSlaveTerminated", (const uint8_t)(busErrorSlaveTerminated & 0x1)}
            };
        };
    public:
        virtual const std::string getClassName() const override { return "EasyReadoutStatusHelper"; };
        /* Construct using default values from docs */
        EasyReadoutStatusHelper(const uint8_t eventReady, const uint8_t outputBufferStatus, const uint8_t busErrorSlaveTerminated)
        {
            construct(eventReady, outputBufferStatus, busErrorSlaveTerminated);
        }
        /* Construct from low-level bit mask in line with docs */
        EasyReadoutStatusHelper(const uint32_t mask)
        {
            constructFromMask(mask);
        }
    }; // class EasyReadoutStatusHelper


    /**
     * @class EasyDPPReadoutStatusHelper
     * @brief For user-friendly configuration of Readout Status mask.
     *
     * NOTE: identical to EasyReadoutStatusHelper
     */
    class EasyDPPReadoutStatusHelper : public EasyReadoutStatusHelper
    {
    /* Just inherit everything else from parent class */
    public:
        virtual const std::string getClassName() const override { return "EasyDPPReadoutStatusHelper"; };
        /* Construct using default values from docs */
        EasyDPPReadoutStatusHelper(const uint8_t eventReady, const uint8_t outputBufferStatus, const uint8_t busErrorSlaveTerminated) : EasyReadoutStatusHelper(eventReady, outputBufferStatus, busErrorSlaveTerminated) {};
        EasyDPPReadoutStatusHelper(const uint32_t mask) : EasyReadoutStatusHelper(mask) {};
    }; // class EasyDPPReadoutStatusHelper


    /**
     * @class EasyAMCFirmwareRevisionHelper
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
     * @param EasyAMCFirmwareRevisionHelper::minorRevisionNumber
     * AMC Firmware Minor Revision Number (Y).
     * @param EasyAMCFirmwareRevisionHelper::majorRevisionNumber
     * AMC Firmware Major Revision Number (X).
     * @param EasyAMCFirmwareRevisionHelper::revisionDate
     * AMC Firmware Revision Date (Y/M/DD).
     */
    class EasyAMCFirmwareRevisionHelper : public EasyHelper
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
        virtual const std::string getClassName() const override { return "EasyAMCFirmwareRevisionHelper"; };
        /* Construct using default values from docs */
        /* We allow both clamped and individual revision date format here */
        EasyAMCFirmwareRevisionHelper(const uint8_t minorRevisionNumber, const uint8_t majorRevisionNumber, const uint16_t revisionDate)
        {
            const uint8_t revisionDayLower = (const uint8_t)(revisionDate & 0x7);
            const uint8_t revisionDayUpper = (const uint8_t)((revisionDate >> 4) & 0x7);
            const uint8_t revisionMonth = (const uint8_t)((revisionDate >> 8) & 0x7);
            const uint8_t revisionYear = (const uint8_t)((revisionDate >> 12) & 0x7);
            construct(minorRevisionNumber, majorRevisionNumber, revisionDayLower, revisionDayUpper, revisionMonth, revisionYear);
        }
        EasyAMCFirmwareRevisionHelper(const uint8_t minorRevisionNumber, const uint8_t majorRevisionNumber, const uint8_t revisionDayLower, const uint8_t revisionDayUpper, const uint8_t revisionMonth, const uint8_t revisionYear)
        {
            construct(minorRevisionNumber, majorRevisionNumber, revisionDayLower, revisionDayUpper, revisionMonth, revisionYear);
        }
        /* Construct from low-level bit mask in line with docs */
        EasyAMCFirmwareRevisionHelper(const uint32_t mask)
        {
            constructFromMask(mask);
        }
    }; // class EasyAMCFirmwareRevisionHelper


    /**
     * @class EasyDPPAMCFirmwareRevisionHelper
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
     * @param EasyDPPAMCFirmwareRevisionHelper::firmwareRevisionNumber
     * Firmware revision number
     * @param EasyDPPAMCFirmwareRevisionHelper::firmwareDPPCode
     * Firmware DPP code. Each DPP firmware has a unique code.
     * @param EasyDPPAMCFirmwareRevisionHelper::buildDayLower
     * Build Day (lower digit)
     * @param EasyDPPAMCFirmwareRevisionHelper::buildDayUpper
     * Build Day (upper digit)
     * @param EasyDPPAMCFirmwareRevisionHelper::buildMonth
     * Build Month. For example: 3 means March, 12 is December.
     * @param EasyDPPAMCFirmwareRevisionHelper::buildYear
     * Build Year. For example: 0 means 2000, 12 means 2012. NOTE: since
     * 2016 the build year started again from 0.
     */
    class EasyDPPAMCFirmwareRevisionHelper : public EasyHelper
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
        virtual const std::string getClassName() const override { return "EasyDPPAMCFirmwareRevisionHelper"; };
        EasyDPPAMCFirmwareRevisionHelper(const uint8_t firmwareRevisionNumber, const uint8_t firmwareDPPCode, const uint8_t buildDayLower, const uint8_t buildDayUpper, const uint8_t buildMonth, const uint8_t buildYear)
        {
            construct(firmwareRevisionNumber, firmwareDPPCode, buildDayLower, buildDayUpper, buildMonth, buildYear);
        }
        /* Construct from low-level bit mask in line with docs */
        EasyDPPAMCFirmwareRevisionHelper(const uint32_t mask)
        {
            constructFromMask(mask);
        }
    }; // class EasyDPPAMCFirmwareRevisionHelper


    /**
     * @class EasyDPPAlgorithmControlHelper
     * @brief For user-friendly configuration of DPP Algorithm Control mask.
     *
     * Management of the DPP algorithm features.
     *
     * @param EasyDPPAlgorithmControlHelper::chargeSensitivity
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
     * @param EasyDPPAlgorithmControlHelper::internalTestPulse
     * Internal Test Pulse. It is possible to enable an internal test
     * pulse for debugging purposes. The ADC counts are replaced with
     * the built-in pulse emulator. Options are:\n
     * 0: disabled\n
     * 1: enabled.
     * @param EasyDPPAlgorithmControlHelper::testPulseRate
     * Test Pulse Rate. Set the rate of the built-in test pulse
     * emulator. Options are:\n
     * 00: 1 kHz\n
     * 01: 10 kHz\n
     * 10: 100 kHz\n
     * 11: 1 MHz.
     * @param EasyDPPAlgorithmControlHelper::chargePedestal
     * Charge Pedestal: when enabled a fixed value of 1024 is added to
     * the charge. This feature is useful in case of energies close to
     * zero.
     * @param EasyDPPAlgorithmControlHelper::inputSmoothingFactor
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
     * @param EasyDPPAlgorithmControlHelper::pulsePolarity
     * Pulse Polarity. Options are:\n
     * 0: positive pulse\n
     * 1: negative pulse.
     * @param EasyDPPAlgorithmControlHelper::triggerMode
     * Trigger Mode. Options are:\n
     * 00: Normal mode. Each channel can self-trigger independently from
     * the other channels.\n
     * 01: Paired mode. Each channel of a couple ’n’ acquire the event
     * in logic OR between its self-trigger and the self-trigger of the
     * other channel of the couple. Couple n corresponds to channel n
     * and channel n+2\n
     * 10: Reserved\n
     * 11: Reserved.
     * @param EasyDPPAlgorithmControlHelper::baselineMean
     * Baseline Mean. Sets the number of events for the baseline mean
     * calculation. Options are:\n
     * 000: Fixed: the baseline value is fixed to the value set in
     * register 0x1n38\n
     * 001: 4 samples\n
     * 010: 16 samples\n
     * 011: 64 samples.
     * @param EasyDPPAlgorithmControlHelper::disableSelfTrigger
     * Disable Self Trigger. If disabled, the self-trigger can be still
     * propagated to the TRG-OUT front panel connector, though it is not
     * used by the channel to acquire the event. Options are:\n
     * 0: self-trigger enabled\n
     * 1: self-trigger disabled.
     * @param EasyDPPAlgorithmControlHelper::triggerHysteresis
     * Trigger Hysteresis. The trigger can be inhibited during the
     * trailing edge of a pulse, to avoid re-triggering on the pulse
     * itself. Options are:\n
     * 0 (default value): enabled\n
     * 1: disabled.
     */
    class EasyDPPAlgorithmControlHelper : public EasyHelper
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
        virtual const std::string getClassName() const override { return "EasyDPPAlgorithmControlHelper"; };
        EasyDPPAlgorithmControlHelper(const uint8_t chargeSensitivity, const uint8_t internalTestPulse, const uint8_t testPulseRate, const uint8_t chargePedestal, const uint8_t inputSmoothingFactor, const uint8_t pulsePolarity, const uint8_t triggerMode, const uint8_t baselineMean, const uint8_t disableSelfTrigger, const uint8_t triggerHysteresis)
        {
            construct(chargeSensitivity, internalTestPulse, testPulseRate, chargePedestal, inputSmoothingFactor, pulsePolarity, triggerMode, baselineMean, disableSelfTrigger, triggerHysteresis);
        }
        /* Construct from low-level bit mask in line with docs */
        EasyDPPAlgorithmControlHelper(const uint32_t mask)
        {
            constructFromMask(mask);
        }
    }; // class EasyDPPAlgorithmControlHelper


    /* Bit-banging helpers to translate between EasyX struct and bitmask.
     * Please refer to the register docs for the individual mask
     * layouts.
     */

    /**
     * @brief unpack at most 8 bits from 32 bit mask
     * @param mask:
     * bit mask
     * @param bits:
     * number of bits to unpack
     * @param offset:
     * offset to the bits to unpack, i.e. how many bits to right-shift
     */
    static uint8_t unpackBits(uint32_t mask, uint8_t bits, uint8_t offset)
    { return (uint8_t)((mask >> offset) & ((1 << bits) - 1)); }
    /**
     * @brief pack at most 8 bits into 32 bit mask
     * @param value:
     * value to pack
     * @param bits:
     * number of bits to pack
     * @param offset:
     * offset to the packed bits, i.e. how many bits to left-shift
     */
    static uint32_t packBits(uint8_t value, uint8_t bits, uint8_t offset)
    { return (value & ((1 << bits) - 1)) << offset; }


    /**
     * @brief pack EasyAMCFirmwareRevision settings into bit mask
     * @param settings:
     * Settings structure to pack
     * @returns
     * Bit mask ready for low-level set function.
     * @internal
     * EasyAMCFirmwareRevision fields:
     * minor revision number in [0:7], major revision number in [8:15],
     * revision date in [16:31]
     */
    static uint32_t eafr2bits(EasyAMCFirmwareRevision settings)
    {
        uint32_t mask = 0;
        mask |= packBits(settings.minorRevisionNumber, 8, 0);
        mask |= packBits(settings.majorRevisionNumber, 8, 8);
        /* revisionDate is 16-bit - manually pack in two steps */
        mask |= packBits(settings.revisionDate & 0xFF, 8, 16);
        mask |= packBits(settings.revisionDate >> 8 & 0xFF, 8, 24);
        return mask;
    }
    /**
     * @brief unpack bit mask into EasyAMCFirmwareRevision settings
     * @param mask:
     * Bit mask from low-level get function to unpack
     * @returns
     * Settings structure for convenient use.
     */
    static EasyAMCFirmwareRevision bits2eafr(uint32_t mask)
    {
        EasyAMCFirmwareRevision settings;
        /* revisionDate is 16-bit - manually unpack in two steps */
        settings = {unpackBits(mask, 8, 0), unpackBits(mask, 8, 8),
                    (uint16_t)(unpackBits(mask, 8, 24) << 8 |
                               unpackBits(mask, 8, 16))};
        return settings;
    }

    /**
     * @brief pack EasyDPPAMCFirmwareRevision settings into bit mask
     * @param settings:
     * Settings structure to pack
     * @returns
     * Bit mask ready for low-level set function.
     * @internal
     * EasyDPPAMCFirmwareRevision fields:
     * firmware revision number in [0:7], firmware DPP code in [8:15],
     * build day lower in [16:19], build day upper in [20:23],
     * build month in [24:27], build year in [28:31]
     */
    static uint32_t edafr2bits(EasyDPPAMCFirmwareRevision settings)
    {
        uint32_t mask = 0;
        mask |= packBits(settings.firmwareRevisionNumber, 8, 0);
        mask |= packBits(settings.firmwareDPPCode, 8, 8);
        mask |= packBits(settings.buildDayLower, 4, 16);
        mask |= packBits(settings.buildDayUpper, 4, 20);
        mask |= packBits(settings.buildMonth, 4, 24);
        mask |= packBits(settings.buildYear, 4, 28);
        return mask;
    }
    /**
     * @brief unpack bit mask into EasyDPPAMCFirmwareRevision settings
     * @param mask:
     * Bit mask from low-level get function to unpack
     * @returns
     * Settings structure for convenient use.
     */
    static EasyDPPAMCFirmwareRevision bits2edafr(uint32_t mask)
    {
        EasyDPPAMCFirmwareRevision settings;
        settings = {unpackBits(mask, 8, 0), unpackBits(mask, 8, 8),
                    unpackBits(mask, 4, 16), unpackBits(mask, 4, 20),
                    unpackBits(mask, 4, 24), unpackBits(mask, 4, 28)};
        return settings;
    }

    /**
     * @brief pack EasyDPPAlgorithmControl settings into bit mask
     * @param settings:
     * Settings structure to pack
     * @returns
     * Bit mask ready for low-level set function.
     * @internal
     * EasyDPPAlgorithmControl fields:
     * charge sensitivity in [0:2], internal test pulse in [4],
     * test pulse rate in [5:6], charge pedestal in [8],
     * input smoothing factor in [12:14], pulse polarity in [16],
     * trigger mode in [18:19], baseline mean in [20:22],
     * disable self trigger in [24], trigger hysteresis in [30].
     */
    static uint32_t edppac2bits(EasyDPPAlgorithmControl settings)
    {
        uint32_t mask = 0;
        mask |= packBits(settings.chargeSensitivity, 3, 0);
        mask |= packBits(settings.internalTestPulse, 1, 4);
        mask |= packBits(settings.testPulseRate, 2, 5);
        mask |= packBits(settings.chargePedestal, 1, 8);
        mask |= packBits(settings.inputSmoothingFactor, 3, 12);
        mask |= packBits(settings.pulsePolarity, 1, 16);
        mask |= packBits(settings.triggerMode, 2, 18);
        mask |= packBits(settings.baselineMean, 3, 20);
        mask |= packBits(settings.disableSelfTrigger, 1, 24);
        mask |= packBits(settings.triggerHysteresis, 1, 30);
        return mask;
    }
    /**
     * @brief unpack bit mask into EasyDPPAlgorithmControl settings
     * @param mask:
     * Bit mask from low-level get function to unpack
     * @returns
     * Settings structure for convenient use.
     */
    static EasyDPPAlgorithmControl bits2edppac(uint32_t mask)
    {
        EasyDPPAlgorithmControl settings;
        settings = {unpackBits(mask, 3, 0), unpackBits(mask, 1, 4),
                    unpackBits(mask, 2, 5), unpackBits(mask, 1, 8),
                    unpackBits(mask, 3, 12), unpackBits(mask, 1, 16),
                    unpackBits(mask, 2, 18), unpackBits(mask, 3, 20),
                    unpackBits(mask, 1, 24), unpackBits(mask, 1, 30)};
        return settings;
    }

    /**
     * @brief pack EasyBoardConfiguration settings into bit mask
     * @param settings:
     * Settings structure to pack
     * @returns
     * Bit mask ready for low-level set function.
     * @internal
     * EasyBoardConfiguration fields:
     * trigger overlap setting in [1], test pattern enable in [3],
     * self-trigger polarity in [6].
     */
    static uint32_t ebc2bits(EasyBoardConfiguration settings)
    {
        uint32_t mask = 0;
        /* NOTE: board configuration includes reserved forced-1 in [4]
         * but we leave that to the low-lewel get/set ops. */
        mask |= packBits(settings.triggerOverlapSetting, 1, 1);
        mask |= packBits(settings.testPatternEnable, 1, 3);
        mask |= packBits(settings.selfTriggerPolarity, 1, 6);
        return mask;
    }
    /**
     * @brief unpack bit mask into EasyBoardConfiguration settings
     * @param mask:
     * Bit mask from low-level get function to unpack
     * @returns
     * Settings structure for convenient use.
     */
    static EasyBoardConfiguration bits2ebc(uint32_t mask)
    {
        EasyBoardConfiguration settings;
        settings = {unpackBits(mask, 1, 1), unpackBits(mask, 1, 3),
                    unpackBits(mask, 1, 6)};
        return settings;
    }

    /**
     * @brief pack EasyDPPBoardConfiguration settings into bit mask
     * @param settings:
     * Settings structure to pack
     * @returns
     * Bit mask ready for low-level set function.
     * @internal
     * EasyDPPBoardConfiguration fields:
     * individual trigger in [8], analog probe in [12:13],
     * waveform recording in [16], extras recording in [17],
     * time stamp recording in [18], charge recording in [19],
     * external trigger mode in [20:21].
     */
    static uint32_t edbc2bits(EasyDPPBoardConfiguration settings)
    {
        uint32_t mask = 0;
        /* NOTE: board configuration includes reserved forced-1 in [4]
         * but we leave that to the low-lewel get/set ops. */
        mask |= packBits(settings.individualTrigger, 1, 8);
        mask |= packBits(settings.analogProbe, 2, 12);
        mask |= packBits(settings.waveformRecording, 1, 16);
        mask |= packBits(settings.extrasRecording, 1, 17);
        mask |= packBits(settings.timeStampRecording, 1, 18);
        mask |= packBits(settings.chargeRecording, 1, 19);
        mask |= packBits(settings.externalTriggerMode, 2, 20);
        return mask;
    }
    /**
     * @brief unpack bit mask into EasyDPPBoardConfiguration settings
     * @param mask:
     * Bit mask from low-level get function to unpack
     * @returns
     * Settings structure for convenient use.
     */
    static EasyDPPBoardConfiguration bits2edbc(uint32_t mask)
    {
        EasyDPPBoardConfiguration settings;
        settings = {unpackBits(mask, 1, 8), unpackBits(mask, 2, 12),
                    unpackBits(mask, 1, 16), unpackBits(mask, 1, 17),
                    unpackBits(mask, 1, 18), unpackBits(mask, 1, 19),
                    unpackBits(mask, 2, 20)};
        return settings;
    }

    /**
     * @brief pack EasyAcquisitionControl settings into bit mask
     * @param settings:
     * Settings structure to pack
     * @returns
     * Bit mask ready for low-level set function.
     * @internal
     * EasyAcquisitionControl fields:
     * start/stop mode [0:1], acquisition start/arm in [2],
     * trigger counting mode in [3], memory full mode selection in [5],
     * PLL reference clock source in [6], LVDS I/O busy enable in [8],
     * LVDS veto enable in [9], LVDS I/O RunIn enable in [11].
     */
    static uint32_t eac2bits(EasyAcquisitionControl settings)
    {
        uint32_t mask = 0;
        mask |= packBits(settings.startStopMode, 2, 0);
        mask |= packBits(settings.acquisitionStartArm, 1, 2);
        mask |= packBits(settings.triggerCountingMode, 1, 3);
        mask |= packBits(settings.memoryFullModeSelection, 1, 5);
        mask |= packBits(settings.pLLRefererenceClockSource, 1, 6);
        mask |= packBits(settings.lVDSIOBusyEnable, 1, 8);
        mask |= packBits(settings.lVDSVetoEnable, 1, 9);
        mask |= packBits(settings.lVDSIORunInEnable, 1, 11);
        return mask;
    }
    /**
     * @brief unpack bit mask into EasyAcquisitionControl settings
     * @param mask:
     * Bit mask from low-level get function to unpack
     * @returns
     * Settings structure for convenient use.
     */
    static EasyAcquisitionControl bits2eac(uint32_t mask)
    {
        EasyAcquisitionControl settings;
        settings = {unpackBits(mask, 2, 0), unpackBits(mask, 1, 2),
                    unpackBits(mask, 1, 3), unpackBits(mask, 1, 5),
                    unpackBits(mask, 1, 6), unpackBits(mask, 1, 8),
                    unpackBits(mask, 1, 9), unpackBits(mask, 1, 11)};
        return settings;
    }

    /**
     * @brief pack EasyDPPAcquisitionControl settings into bit mask
     * @param settings:
     * Settings structure to pack
     * @returns
     * Bit mask ready for low-level set function.
     * @internal
     * EasyDPPAcquisitionControl fields:
     * start/stop mode [0:1], acquisition start/arm in [2],
     * trigger counting mode in [3], PLL reference clock
     * source in [6], LVDS I/O busy enable in [8],
     * LVDS veto enable in [9], LVDS I/O RunIn enable in [11].
     */
    static uint32_t edac2bits(EasyDPPAcquisitionControl settings)
    {
        uint32_t mask = 0;
        mask |= packBits(settings.startStopMode, 2, 0);
        mask |= packBits(settings.acquisitionStartArm, 1, 2);
        mask |= packBits(settings.triggerCountingMode, 1, 3);
        mask |= packBits(settings.pLLRefererenceClockSource, 1, 6);
        mask |= packBits(settings.lVDSIOBusyEnable, 1, 8);
        mask |= packBits(settings.lVDSVetoEnable, 1, 9);
        mask |= packBits(settings.lVDSIORunInEnable, 1, 11);
        return mask;
    }
    /**
     * @brief unpack bit mask into EasyDPPAcquisitionControl settings
     * @param mask:
     * Bit mask from low-level get function to unpack
     * @returns
     * Settings structure for convenient use.
     */
    static EasyDPPAcquisitionControl bits2edac(uint32_t mask)
    {
        EasyDPPAcquisitionControl settings;
        settings = {unpackBits(mask, 2, 0), unpackBits(mask, 1, 2),
                    unpackBits(mask, 1, 3), unpackBits(mask, 1, 6),
                    unpackBits(mask, 1, 8), unpackBits(mask, 1, 9),
                    unpackBits(mask, 1, 11)};
        return settings;
    }

    /**
     * @brief pack EasyAcquisitionStatus settings into bit mask
     * @param settings:
     * Settings structure to pack
     * @returns
     * Bit mask ready for low-level set function.
     * @internal
     * EasyAcquisitionStatus fields:
     * acquisition status [2], event ready [3], event full in [4],
     * clock source in [5], PLL bypass mode in [6],
     * PLL unlock detect in [7], board ready in [8], S-In in [15],
     * TRG-IN in [16].
     */
    static uint32_t eas2bits(EasyAcquisitionStatus settings)
    {
        uint32_t mask = 0;
        mask |= packBits(settings.acquisitionStatus, 1, 2);
        mask |= packBits(settings.eventReady, 1, 3);
        mask |= packBits(settings.eventFull, 1, 4);
        mask |= packBits(settings.clockSource, 1, 5);
        mask |= packBits(settings.pLLBypassMode, 1, 6);
        mask |= packBits(settings.pLLUnlockDetect, 1, 7);
        mask |= packBits(settings.boardReady, 1, 8);
        mask |= packBits(settings.s_IN, 1, 15);
        mask |= packBits(settings.tRG_IN, 1, 16);
        return mask;
    }
    /**
     * @brief unpack bit mask into EasyAcquisitionStatus settings
     * @param mask:
     * Bit mask from low-level get function to unpack
     * @returns
     * Settings structure for convenient use.
     */
    static EasyAcquisitionStatus bits2eas(uint32_t mask)
    {
        EasyAcquisitionStatus settings;
        settings = {unpackBits(mask, 1, 2), unpackBits(mask, 1, 3),
                    unpackBits(mask, 1, 4), unpackBits(mask, 1, 5),
                    unpackBits(mask, 1, 6), unpackBits(mask, 1, 7),
                    unpackBits(mask, 1, 8), unpackBits(mask, 1, 15),
                    unpackBits(mask, 1, 16)};
        return settings;
    }

    /**
     * @brief pack EasyDPPAcquisitionStatus settings into bit mask
     * @param settings:
     * Settings structure to pack
     * @returns
     * Bit mask ready for low-level set function.
     * @internal
     * EasyDPPAcquisitionStatus fields:
     * acquisition status [2], event ready [3], event full in [4],
     * clock source in [5], PLL unlock detect in [7], board ready in [8],
     * S-In in [15], TRG-IN in [16].
     */
    static uint32_t edas2bits(EasyDPPAcquisitionStatus settings)
    {
        uint32_t mask = 0;
        mask |= packBits(settings.acquisitionStatus, 1, 2);
        mask |= packBits(settings.eventReady, 1, 3);
        mask |= packBits(settings.eventFull, 1, 4);
        mask |= packBits(settings.clockSource, 1, 5);
        mask |= packBits(settings.pLLUnlockDetect, 1, 7);
        mask |= packBits(settings.boardReady, 1, 8);
        mask |= packBits(settings.s_IN, 1, 15);
        mask |= packBits(settings.tRG_IN, 1, 16);
        return mask;
    }
    /**
     * @brief unpack bit mask into EasyDPPAcquisitionStatus settings
     * @param mask:
     * Bit mask from low-level get function to unpack
     * @returns
     * Settings structure for convenient use.
     */
    static EasyDPPAcquisitionStatus bits2edas(uint32_t mask)
    {
        EasyDPPAcquisitionStatus settings;
        settings = {unpackBits(mask, 1, 2), unpackBits(mask, 1, 3),
                    unpackBits(mask, 1, 4), unpackBits(mask, 1, 5),
                    unpackBits(mask, 1, 7), unpackBits(mask, 1, 8),
                    unpackBits(mask, 1, 15), unpackBits(mask, 1, 16)};
        return settings;
    }

    /**
     * @brief pack EasyGlobalTriggerMask settings into bit mask
     * @param settings:
     * Settings structure to pack
     * @returns
     * Bit mask ready for low-level set function.
     * @internal
     * EasyGlobalTriggerMask fields:
     * groupTriggerMask in [0:7], majorityCoincidenceWindow in [20:23],
     * majorityLevel in [24:26], LVDS trigger in [29],
     * external trigger in [30], software trigger in [31].
     */
    static uint32_t egtm2bits(EasyGlobalTriggerMask settings)
    {
        uint32_t mask = 0;
        mask |= packBits(settings.groupTriggerMask, 8, 0);
        mask |= packBits(settings.majorityCoincidenceWindow, 4, 20);
        mask |= packBits(settings.majorityLevel, 3, 24);
        mask |= packBits(settings.lVDSTrigger, 1, 29);
        mask |= packBits(settings.externalTrigger, 1, 30);
        mask |= packBits(settings.softwareTrigger, 1, 31);
        return mask;
    }
    /**
     * @brief unpack bit mask into EasyGlobalTriggerMask settings
     * @param mask:
     * Bit mask from low-level get function to unpack
     * @returns
     * Settings structure for convenient use.
     */
    static EasyGlobalTriggerMask bits2egtm(uint32_t mask)
    {
        EasyGlobalTriggerMask settings;
        settings = {unpackBits(mask, 8, 0), unpackBits(mask, 4, 20),
                    unpackBits(mask, 3, 24), unpackBits(mask, 1, 29),
                    unpackBits(mask, 1, 30), unpackBits(mask, 1, 31)};
        return settings;
    }

    /**
     * @brief pack EasyDPPGlobalTriggerMask settings into bit mask
     * @param settings:
     * Settings structure to pack
     * @returns
     * Bit mask ready for low-level set function.
     * @internal
     * EasyDPPGlobalTriggerMask fields:
     * LVDS trigger in [29], external trigger in [30],
     * software trigger in [31].
     */
    static uint32_t edgtm2bits(EasyDPPGlobalTriggerMask settings)
    {
        uint32_t mask = 0;
        mask |= packBits(settings.lVDSTrigger, 1, 29);
        mask |= packBits(settings.externalTrigger, 1, 30);
        mask |= packBits(settings.softwareTrigger, 1, 31);
        return mask;
    }
    /**
     * @brief unpack bit mask into EasyDPPGlobalTriggerMask settings
     * @param mask:
     * Bit mask from low-level get function to unpack
     * @returns
     * Settings structure for convenient use.
     */
    static EasyDPPGlobalTriggerMask bits2edgtm(uint32_t mask)
    {
        EasyDPPGlobalTriggerMask settings;
        settings = {unpackBits(mask, 1, 29), unpackBits(mask, 1, 30),
                    unpackBits(mask, 1, 31)};
        return settings;
    }

    /**
     * @brief pack EasyFrontPanelTRGOUTEnableMask settings into bit mask
     * @param settings:
     * Settings structure to pack
     * @returns
     * Bit mask ready for low-level set function.
     * @internal
     * EasyFrontPanelTRGOUTEnableMask fields:
     * group Trigger mask in [0:7], TRG-OUT generation logic in [8:9],
     * majority level in [10:12], LVDS trigger enable in [29],
     * external trigger in [30], software trigger in [31].
     */
    static uint32_t efptoem2bits(EasyFrontPanelTRGOUTEnableMask settings)
    {
        uint32_t mask = 0;
        mask |= packBits(settings.groupTriggerMask, 8, 0);
        mask |= packBits(settings.tRGOUTGenerationLogic, 2, 8);
        mask |= packBits(settings.majorityLevel, 3, 10);
        mask |= packBits(settings.lVDSTriggerEnable, 1, 29);
        mask |= packBits(settings.externalTrigger, 1, 30);
        mask |= packBits(settings.softwareTrigger, 1, 31);
        return mask;
    }
    /**
     * @brief unpack bit mask into EasyFrontPanelTRGOUTEnableMask settings
     * @param mask:
     * Bit mask from low-level get function to unpack
     * @returns
     * Settings structure for convenient use.
     */
    static EasyFrontPanelTRGOUTEnableMask bits2efptoem(uint32_t mask)
    {
        EasyFrontPanelTRGOUTEnableMask settings;
        settings = {unpackBits(mask, 8, 0), unpackBits(mask, 2, 8),
                    unpackBits(mask, 3, 10), unpackBits(mask, 1, 29),
                    unpackBits(mask, 1, 30), unpackBits(mask, 1, 31)};
        return settings;
    }

    /**
     * @brief pack EasyDPPFrontPanelTRGOUTEnableMask settings into bit mask
     * @param settings:
     * Settings structure to pack
     * @returns
     * Bit mask ready for low-level set function.
     * @internal
     * EasyDPPFrontPanelTRGOUTEnableMask fields:
     * LVDS trigger enable in [29], external trigger in [30],
     * software trigger in [31].
     */
    static uint32_t edfptoem2bits(EasyDPPFrontPanelTRGOUTEnableMask settings)
    {
        uint32_t mask = 0;
        mask |= packBits(settings.lVDSTriggerEnable, 1, 29);
        mask |= packBits(settings.externalTrigger, 1, 30);
        mask |= packBits(settings.softwareTrigger, 1, 31);
        return mask;
    }
    /**
     * @brief unpack bit mask into EasyDPPFrontPanelTRGOUTEnableMask settings
     * @param mask:
     * Bit mask from low-level get function to unpack
     * @returns
     * Settings structure for convenient use.
     */
    static EasyDPPFrontPanelTRGOUTEnableMask bits2edfptoem(uint32_t mask)
    {
        EasyDPPFrontPanelTRGOUTEnableMask settings;
        settings = {unpackBits(mask, 1, 29), unpackBits(mask, 1, 30),
                    unpackBits(mask, 1, 31)};
        return settings;
    }

    /**
     * @brief pack EasyFrontPanelIOControl settings into bit mask
     * @param settings:
     * Settings structure to pack
     * @returns
     * Bit mask ready for low-level set function.
     * @internal
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
    static uint32_t efpioc2bits(EasyFrontPanelIOControl settings)
    {
        uint32_t mask = 0;
        mask |= packBits(settings.lEMOIOElectricalLevel, 1, 0);
        mask |= packBits(settings.tRGOUTEnable, 1, 1);
        mask |= packBits(settings.lVDSIODirectionFirst, 1, 2);
        mask |= packBits(settings.lVDSIODirectionSecond, 1, 3);
        mask |= packBits(settings.lVDSIODirectionThird, 1, 4);
        mask |= packBits(settings.lVDSIODirectionFourth, 1, 5);
        mask |= packBits(settings.lVDSIOSignalConfiguration, 2, 6);
        mask |= packBits(settings.lVDSIONewFeaturesSelection, 1, 8);
        mask |= packBits(settings.lVDSIOPatternLatchMode, 1, 9);
        mask |= packBits(settings.tRGINControl, 1, 10);
        mask |= packBits(settings.tRGINMezzanines, 1, 11);
        mask |= packBits(settings.forceTRGOUT, 1, 14);
        mask |= packBits(settings.tRGOUTMode, 1, 15);
        mask |= packBits(settings.tRGOUTModeSelection, 2, 16);
        mask |= packBits(settings.motherboardVirtualProbeSelection, 2, 18);
        mask |= packBits(settings.motherboardVirtualProbePropagation, 1, 20);
        mask |= packBits(settings.patternConfiguration, 2, 21);
        return mask;
    }
    /**
     * @brief unpack bit mask into EasyFrontPanelIOControl settings
     * @param mask:
     * Bit mask from low-level get function to unpack
     * @returns
     * Settings structure for convenient use.
     */
    static EasyFrontPanelIOControl bits2efpioc(uint32_t mask)
    {
        EasyFrontPanelIOControl settings;
        settings = {unpackBits(mask, 1, 0), unpackBits(mask, 1, 1),
                    unpackBits(mask, 1, 2), unpackBits(mask, 1, 3),
                    unpackBits(mask, 1, 4), unpackBits(mask, 1, 5),
                    unpackBits(mask, 2, 6), unpackBits(mask, 1, 8),
                    unpackBits(mask, 1, 9), unpackBits(mask, 1, 10),
                    unpackBits(mask, 1, 11), unpackBits(mask, 1, 14),
                    unpackBits(mask, 1, 15), unpackBits(mask, 2, 16),
                    unpackBits(mask, 2, 18), unpackBits(mask, 1, 20),
                    unpackBits(mask, 2, 21)};
        return settings;
    }

    /**
     * @brief pack EasyROCFPGAFirmwareRevision settings into bit mask
     * @param settings:
     * Settings structure to pack
     * @returns
     * Bit mask ready for low-level set function.
     * @internal
     * EasyROCFPGAFirmwareRevision fields:
     * minor revision number in [0:7], major revision number in [8:15],
     * revision date in [16:31]
     */
    static uint32_t erffr2bits(EasyROCFPGAFirmwareRevision settings)
    {
        uint32_t mask = 0;
        mask |= packBits(settings.minorRevisionNumber, 8, 0);
        mask |= packBits(settings.majorRevisionNumber, 8, 8);
        /* revisionDate is 16-bit - manually pack in two steps */
        mask |= packBits(settings.revisionDate & 0xFF, 8, 16);
        mask |= packBits(settings.revisionDate >> 8 & 0xFF, 8, 24);
        return mask;
    }
    /**
     * @brief unpack bit mask into EasyROCFPGAFirmwareRevision settings
     * @param mask:
     * Bit mask from low-level get function to unpack
     * @returns
     * Settings structure for convenient use.
     */
    static EasyROCFPGAFirmwareRevision bits2erffr(uint32_t mask)
    {
        EasyROCFPGAFirmwareRevision settings;
        /* revisionDate is 16-bit - manually unpack in two steps */
        settings = {unpackBits(mask, 8, 0), unpackBits(mask, 8, 8),
                    (uint16_t)(unpackBits(mask, 8, 24) << 8 |
                               unpackBits(mask, 8, 16))};
        return settings;
    }

    /**
     * @brief pack EasyFanSpeedControl settings into bit mask
     * @param settings:
     * Settings structure to pack
     * @returns
     * Bit mask ready for low-level set function.
     * @internal
     * EasyFanSpeedControl fields:
     * fan Speed mode in [3].
     */
    static uint32_t efsc2bits(EasyFanSpeedControl settings)
    {
        uint32_t mask = 0;
        mask |= packBits(settings.fanSpeedMode, 1, 3);
        return mask;
    }
    /**
     * @brief unpack bit mask into EasyFanSpeedControl settings
     * @param mask:
     * Bit mask from low-level get function to unpack
     * @returns
     * Settings structure for convenient use.
     */
    static EasyFanSpeedControl bits2efsc(uint32_t mask)
    {
        EasyFanSpeedControl settings;
        settings = {unpackBits(mask, 1, 3)};
        return settings;
    }


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
        { CAEN_DGTZ_DPPFirmware_t firmware; errorHandler(_CAEN_DGTZ_GetDPPFirmwareType(handle_, &firmware)); return firmware; }

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
        { errorHandler(CAEN_DGTZ_ReadData(handle_, mode, buffer.data, &buffer.dataSize)); return buffer; }

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
        { ReadoutBuffer b; errorHandler(_CAEN_DGTZ_MallocReadoutBuffer(handle_, &b.data, &b.size)); return b; }
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
            switch(getDPPFirmwareType())
            {
                case CAEN_DGTZ_DPPFirmware_PHA:
                    events.ptr = new void*[MAX_DPP_PHA_CHANNEL_SIZE];
                    events.nEvents = new uint32_t[MAX_DPP_PHA_CHANNEL_SIZE];
                    break;
                case CAEN_DGTZ_DPPFirmware_PSD:
                    events.ptr = new void*[MAX_DPP_PSD_CHANNEL_SIZE];
                    events.nEvents = new uint32_t[MAX_DPP_PSD_CHANNEL_SIZE];
                    break;
                case CAEN_DGTZ_DPPFirmware_CI:
                    events.ptr = new void*[MAX_DPP_CI_CHANNEL_SIZE];
                    events.nEvents = new uint32_t[MAX_DPP_CI_CHANNEL_SIZE];
                    break;
                case CAEN_DGTZ_DPPFirmware_QDC:
                    events.ptr = new void*[MAX_DPP_QDC_CHANNEL_SIZE];
                    events.nEvents = new uint32_t[MAX_DPP_QDC_CHANNEL_SIZE];
                    break;
                default:
                    errorHandler(CAEN_DGTZ_FunctionNotAllowed);
            }
            errorHandler(_CAEN_DGTZ_MallocDPPEvents(handle_, events.ptr, &events.allocatedSize));
            return events;
        }
        void freeDPPEvents(DPPEvents events)
        { errorHandler(_CAEN_DGTZ_FreeDPPEvents(handle_, events.ptr)); delete[](events.ptr); delete[](events.nEvents); }

        DPPWaveforms mallocDPPWaveforms()
        { DPPWaveforms waveforms; errorHandler(_CAEN_DGTZ_MallocDPPWaveforms(handle_, &waveforms.ptr, &waveforms.allocatedSize)); return waveforms; }
        /* TODO: is this actually supposed to be a (void **) to FreeDPPWaveforms? 
         *       From docs it looks more like a (void *) but explicitly
         *       uses (void **) in _CAENDigitizer.c for some reason.
         */
        void freeDPPWaveforms(DPPWaveforms waveforms)
        { errorHandler(_CAEN_DGTZ_FreeDPPWaveforms(handle_, &waveforms.ptr)); }

        /* Detector data information and manipulation*/
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

        DPPEvents& getDPPEvents(ReadoutBuffer buffer, DPPEvents& events)
        { errorHandler(CAEN_DGTZ_GetDPPEvents(handle_, buffer.data, buffer.dataSize, events.ptr, events.nEvents)); return events; }

        /* TODO: is this actually supposed to be a (void **) to DecodeDPPWaveforms? 
         *       From docs it looks more like a (void *) but see above.
         */
        DPPWaveforms& decodeDPPWaveforms(void *event, DPPWaveforms& waveforms)
        { errorHandler(CAEN_DGTZ_DecodeDPPWaveforms(handle_, event, waveforms.ptr)); return waveforms; }

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

        /**
         * @bug
         * CAEN_DGTZ_GetDecimationFactor fails with GenericError
         * on DT5740_171 and V1740D_137, apparently a mismatch between
         * DigitizerTable value end value read from register in the
         * V1740 specific case. 
         */
        uint16_t getDecimationFactor()
        { uint16_t factor;  errorHandler(CAEN_DGTZ_GetDecimationFactor(handle_, &factor)); return factor; }
        void setDecimationFactor(uint16_t factor)
        { errorHandler(CAEN_DGTZ_SetDecimationFactor(handle_, factor)); }

        uint32_t getPostTriggerSize()
        { uint32_t percent; errorHandler(CAEN_DGTZ_GetPostTriggerSize(handle_, &percent)); return percent;}
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

        uint32_t getChannelDCOffset(uint32_t channel)
        { uint32_t offset; errorHandler(CAEN_DGTZ_GetChannelDCOffset(handle_, channel, &offset)); return offset; }
        void setChannelDCOffset(uint32_t channel, uint32_t offset)
        { errorHandler(CAEN_DGTZ_SetChannelDCOffset(handle_, channel, offset)); }

        /**
         * @bug
         * CAEN_DGTZ_GetGroupDCOffset fails with GenericError on
         * V1740D_137, something fails in the V1740 specific case. 
         */
        uint32_t getGroupDCOffset(uint32_t group)
        {
            uint32_t offset;
            if (group >= groups())  // Needed because of bug in CAEN_DGTZ_GetGroupDCOffset - patch sent
                errorHandler(CAEN_DGTZ_InvalidChannelNumber);
            errorHandler(CAEN_DGTZ_GetGroupDCOffset(handle_, group, &offset)); return offset;
        }
        void setGroupDCOffset(uint32_t group, uint32_t offset)
        {
            if (group >= groups())  // Needed because of bug in CAEN_DGTZ_SetGroupDCOffset - patch sent
                errorHandler(CAEN_DGTZ_InvalidChannelNumber);
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
        { CAEN_DGTZ_TriggerMode_t mode; errorHandler(CAEN_DGTZ_GetFastTriggerMode(handle_, &mode)); return mode; }
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
        /* NOTE: we explicitly initialize params here since some of them
         * may remain untouched garbage otherwise */
        AIMParams getAnalogInspectionMonParams()
        { AIMParams params; params.channelmask = 0; params.offset = 0; params.mf = (CAEN_DGTZ_AnalogMonitorMagnify_t)0; params.ami = (CAEN_DGTZ_AnalogMonitorInspectorInverter_t)0;
            errorHandler(CAEN_DGTZ_GetAnalogInspectionMonParams(handle_, &params.channelmask, &params.offset, &params.mf, &params.ami)); return params; }
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

        /**
         * @bug
         * CAEN_DGTZ_GetChannelPulsePolarity fails with InvalidParam om
         * DT5740_171 and V1740D_137. Seems to fail deep in readout when
         * the digtizer library calls ReadRegister 0x1n80 
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

        uint32_t getNumEventsPerAggregate()
        { uint32_t numEvents; errorHandler(CAEN_DGTZ_GetNumEventsPerAggregate(handle_, &numEvents)); return numEvents; }
        uint32_t getNumEventsPerAggregate(uint32_t channel)
        { uint32_t numEvents; errorHandler(CAEN_DGTZ_GetNumEventsPerAggregate(handle_, &numEvents, channel)); return numEvents; }
        void setNumEventsPerAggregate(uint32_t numEvents)
        { errorHandler(CAEN_DGTZ_SetNumEventsPerAggregate(handle_, numEvents)); }
        void setNumEventsPerAggregate(uint32_t channel, uint32_t numEvents)
        { errorHandler(CAEN_DGTZ_SetNumEventsPerAggregate(handle_, numEvents, channel)); }

        uint32_t getMaxNumAggregatesBLT()
        { uint32_t numAggr; errorHandler(CAEN_DGTZ_GetMaxNumAggregatesBLT(handle_, &numAggr)); return numAggr; }
        void setMaxNumAggregatesBLT(uint32_t numAggr)
        { errorHandler(CAEN_DGTZ_SetMaxNumAggregatesBLT(handle_, numAggr)); }

        void setDPPParameters(uint32_t channelmask, void *params)
        { errorHandler(CAEN_DGTZ_SetDPPParameters(handle_, channelmask, params)); }

        virtual uint32_t getAMCFirmwareRevision(uint32_t group) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyAMCFirmwareRevision getEasyAMCFirmwareRevision(uint32_t group) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyDPPAMCFirmwareRevision getEasyDPPAMCFirmwareRevision(uint32_t group) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyAMCFirmwareRevisionHelper getEasyAMCFirmwareRevisionHelper(uint32_t group) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyDPPAMCFirmwareRevisionHelper getEasyDPPAMCFirmwareRevisionHelper(uint32_t group) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

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
        virtual void setDPPTriggerHoldOffWidth(uint32_t group, uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setDPPTriggerHoldOffWidth(uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getDPPAlgorithmControl(uint32_t group) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setDPPAlgorithmControl(uint32_t group, uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setDPPAlgorithmControl(uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyDPPAlgorithmControl getEasyDPPAlgorithmControl(uint32_t group) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setEasyDPPAlgorithmControl(uint32_t group, EasyDPPAlgorithmControl settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setEasyDPPAlgorithmControl(EasyDPPAlgorithmControl settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyDPPAlgorithmControlHelper getEasyDPPAlgorithmControlHelper(uint32_t group) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setEasyDPPAlgorithmControlHelper(uint32_t group, EasyDPPAlgorithmControlHelper settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setEasyDPPAlgorithmControlHelper(EasyDPPAlgorithmControlHelper settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getDPPShapedTriggerWidth(uint32_t group) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setDPPShapedTriggerWidth(uint32_t group, uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setDPPShapedTriggerWidth(uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getBoardConfiguration() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setBoardConfiguration(uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void unsetBoardConfiguration(uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyBoardConfiguration getEasyBoardConfiguration() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setEasyBoardConfiguration(EasyBoardConfiguration settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void unsetEasyBoardConfiguration(EasyBoardConfiguration settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyDPPBoardConfiguration getEasyDPPBoardConfiguration() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setEasyDPPBoardConfiguration(EasyDPPBoardConfiguration settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void unsetEasyDPPBoardConfiguration(EasyDPPBoardConfiguration settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyBoardConfigurationHelper getEasyBoardConfigurationHelper() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setEasyBoardConfigurationHelper(EasyBoardConfigurationHelper settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void unsetEasyBoardConfigurationHelper(EasyBoardConfigurationHelper settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyDPPBoardConfigurationHelper getEasyDPPBoardConfigurationHelper() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setEasyDPPBoardConfigurationHelper(EasyDPPBoardConfigurationHelper settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void unsetEasyDPPBoardConfigurationHelper(EasyDPPBoardConfigurationHelper settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getDPPAggregateOrganization() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setDPPAggregateOrganization(uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getAcquisitionControl() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setAcquisitionControl(uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyAcquisitionControl getEasyAcquisitionControl() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setEasyAcquisitionControl(EasyAcquisitionControl settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyDPPAcquisitionControl getEasyDPPAcquisitionControl() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setEasyDPPAcquisitionControl(EasyDPPAcquisitionControl settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyAcquisitionControlHelper getEasyAcquisitionControlHelper() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setEasyAcquisitionControlHelper(EasyAcquisitionControlHelper settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyDPPAcquisitionControlHelper getEasyDPPAcquisitionControlHelper() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setEasyDPPAcquisitionControlHelper(EasyDPPAcquisitionControlHelper settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getAcquisitionStatus() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyAcquisitionStatus getEasyAcquisitionStatus() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual uint32_t getDPPAcquisitionStatus() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyDPPAcquisitionStatus getEasyDPPAcquisitionStatus() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyAcquisitionStatusHelper getEasyAcquisitionStatusHelper() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyDPPAcquisitionStatusHelper getEasyDPPAcquisitionStatusHelper() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getGlobalTriggerMask() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setGlobalTriggerMask(uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyGlobalTriggerMask getEasyGlobalTriggerMask() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setEasyGlobalTriggerMask(EasyGlobalTriggerMask settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyDPPGlobalTriggerMask getEasyDPPGlobalTriggerMask() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setEasyDPPGlobalTriggerMask(EasyDPPGlobalTriggerMask settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyGlobalTriggerMaskHelper getEasyGlobalTriggerMaskHelper() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setEasyGlobalTriggerMaskHelper(EasyGlobalTriggerMaskHelper settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyDPPGlobalTriggerMaskHelper getEasyDPPGlobalTriggerMaskHelper() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setEasyDPPGlobalTriggerMaskHelper(EasyDPPGlobalTriggerMaskHelper settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getFrontPanelTRGOUTEnableMask() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setFrontPanelTRGOUTEnableMask(uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyFrontPanelTRGOUTEnableMask getEasyFrontPanelTRGOUTEnableMask() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setEasyFrontPanelTRGOUTEnableMask(EasyFrontPanelTRGOUTEnableMask settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyDPPFrontPanelTRGOUTEnableMask getEasyDPPFrontPanelTRGOUTEnableMask() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setEasyDPPFrontPanelTRGOUTEnableMask(EasyDPPFrontPanelTRGOUTEnableMask settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyFrontPanelTRGOUTEnableMaskHelper getEasyFrontPanelTRGOUTEnableMaskHelper() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setEasyFrontPanelTRGOUTEnableMaskHelper(EasyFrontPanelTRGOUTEnableMaskHelper settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyDPPFrontPanelTRGOUTEnableMaskHelper getEasyDPPFrontPanelTRGOUTEnableMaskHelper() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setEasyDPPFrontPanelTRGOUTEnableMaskHelper(EasyDPPFrontPanelTRGOUTEnableMaskHelper settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getFrontPanelIOControl() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setFrontPanelIOControl(uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyFrontPanelIOControl getEasyFrontPanelIOControl() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setEasyFrontPanelIOControl(EasyFrontPanelIOControl settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyDPPFrontPanelIOControl getEasyDPPFrontPanelIOControl() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setEasyDPPFrontPanelIOControl(EasyDPPFrontPanelIOControl settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyFrontPanelIOControlHelper getEasyFrontPanelIOControlHelper() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setEasyFrontPanelIOControlHelper(EasyFrontPanelIOControlHelper settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyDPPFrontPanelIOControlHelper getEasyDPPFrontPanelIOControlHelper() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setEasyDPPFrontPanelIOControlHelper(EasyDPPFrontPanelIOControlHelper settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getROCFPGAFirmwareRevision() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyROCFPGAFirmwareRevision getEasyROCFPGAFirmwareRevision() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyDPPROCFPGAFirmwareRevision getEasyDPPROCFPGAFirmwareRevision() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyROCFPGAFirmwareRevisionHelper getEasyROCFPGAFirmwareRevisionHelper() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyDPPROCFPGAFirmwareRevisionHelper getEasyDPPROCFPGAFirmwareRevisionHelper() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getEventSize() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getFanSpeedControl() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setFanSpeedControl(uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyFanSpeedControl getEasyFanSpeedControl() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setEasyFanSpeedControl(EasyFanSpeedControl settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyDPPFanSpeedControl getEasyDPPFanSpeedControl() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setEasyDPPFanSpeedControl(EasyDPPFanSpeedControl settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyFanSpeedControlHelper getEasyFanSpeedControlHelper() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setEasyFanSpeedControlHelper(EasyFanSpeedControlHelper settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyDPPFanSpeedControlHelper getEasyDPPFanSpeedControlHelper() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setEasyDPPFanSpeedControlHelper(EasyDPPFanSpeedControlHelper settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getDPPDisableExternalTrigger() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setDPPDisableExternalTrigger(uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getRunStartStopDelay() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setRunStartStopDelay(uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getReadoutControl() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setReadoutControl(uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyReadoutControlHelper getEasyReadoutControlHelper() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setEasyReadoutControlHelper(EasyReadoutControlHelper settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyDPPReadoutControlHelper getEasyDPPReadoutControlHelper() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setEasyDPPReadoutControlHelper(EasyDPPReadoutControlHelper settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getReadoutStatus() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyReadoutStatusHelper getEasyReadoutStatusHelper() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyDPPReadoutStatusHelper getEasyDPPReadoutStatusHelper() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

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
         * EasyAMCFirmwareRevision structure
         */
        EasyAMCFirmwareRevision getEasyAMCFirmwareRevision(uint32_t group) override
        {
            uint32_t mask;
            mask = getAMCFirmwareRevision(group);
            return bits2eafr(mask);
        }
        /**
         * @brief Easy Get AMCFirmwareRevisionHelper
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
         * EasyAMCFirmwareRevisionHelper object
         */
        EasyAMCFirmwareRevisionHelper getEasyAMCFirmwareRevisionHelper(uint32_t group) override
        {
            uint32_t mask;
            mask = getAMCFirmwareRevision(group);
            return EasyAMCFirmwareRevisionHelper(mask);
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
        { errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x8004, filterBoardConfigurationSetMask(mask))); }
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
        { errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x8008, filterBoardConfigurationUnsetMask(mask))); }

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
         * EasyBoardConfiguration structure
         */
        EasyBoardConfiguration getEasyBoardConfiguration() override
        {
            uint32_t mask;
            mask = getBoardConfiguration();
            return bits2ebc(mask);
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
         * @param settings:
         * EasyBoardConfiguration structure
         */
        void setEasyBoardConfiguration(EasyBoardConfiguration settings) override
        {
            uint32_t mask = ebc2bits(settings);
            setBoardConfiguration(mask);
        }
        /**
         * @brief Easy Unset BoardConfiguration
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating to the bit mask needed by the
         * the underlying low-level unset funtion.
         *
         * @param settings:
         * EasyBoardConfiguration structure
         */
        void unsetEasyBoardConfiguration(EasyBoardConfiguration settings) override
        {
            uint32_t mask = ebc2bits(settings);
            unsetBoardConfiguration(mask);
        }
        /**
         * @brief Easy Get BoardConfigurationHelper
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
        EasyBoardConfigurationHelper getEasyBoardConfigurationHelper() override
        {
            uint32_t mask;
            mask = getBoardConfiguration();
            return EasyBoardConfigurationHelper(mask);
        }
        /**
         * @brief Easy Set BoardConfigurationHelper
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating to the bit mask needed by the
         * the underlying low-level set funtion.
         *
         * @param settings:
         * EasyBoardConfiguration object
         */
        void setEasyBoardConfigurationHelper(EasyBoardConfigurationHelper settings) override
        {
            uint32_t mask = settings.toBits();
            setBoardConfiguration(mask);
        }
        /**
         * @brief Easy Unset BoardConfigurationHelper
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating to the bit mask needed by the
         * the underlying low-level unset funtion.
         *
         * @param settings:
         * EasyBoardConfiguration object
         */
        void unsetEasyBoardConfigurationHelper(EasyBoardConfigurationHelper settings) override
        {
            uint32_t mask = settings.toBits();
            unsetBoardConfiguration(mask);
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
         * EasyAcquisitionControl structure
         */
        EasyAcquisitionControl getEasyAcquisitionControl() override
        {
            uint32_t mask;
            mask = getAcquisitionControl();
            return bits2eac(mask);
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
         * EasyAcquisitionControl structure
         */
        void setEasyAcquisitionControl(EasyAcquisitionControl settings) override
        {
            uint32_t mask = eac2bits(settings);
            setAcquisitionControl(mask);
        }
        /**
         * @brief Easy Get AcquisitionControlHelper
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
        EasyAcquisitionControlHelper getEasyAcquisitionControlHelper() override
        {
            uint32_t mask;
            mask = getAcquisitionControl();
            return EasyAcquisitionControlHelper(mask);
        }
        /**
         * @brief Easy Set AcquisitionControlHelper
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
        void setEasyAcquisitionControlHelper(EasyAcquisitionControlHelper settings) override
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
         * EasyAcquisitionStatus structure
         */
        EasyAcquisitionStatus getEasyAcquisitionStatus() override
        {
            uint32_t mask;
            mask = getAcquisitionStatus();
            return bits2eas(mask);
        }
        /**
         * @brief Easy Get AcquisitionStatusHelper
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
        EasyAcquisitionStatusHelper getEasyAcquisitionStatusHelper() override
        {
            uint32_t mask;
            mask = getAcquisitionStatus();
            return EasyAcquisitionStatusHelper(mask);
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
         * EasyGlobalTriggerMask structure
         */
        EasyGlobalTriggerMask getEasyGlobalTriggerMask() override
        {
            uint32_t mask;
            mask = getGlobalTriggerMask();
            return bits2egtm(mask);
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
         * EasyGlobalTriggerMask structure
         */
        void setEasyGlobalTriggerMask(EasyGlobalTriggerMask settings) override
        {
            uint32_t mask = egtm2bits(settings);
            setGlobalTriggerMask(mask);
        }
        /**
         * @brief Easy Get GlobalTriggerMaskHelper
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
        EasyGlobalTriggerMaskHelper getEasyGlobalTriggerMaskHelper() override
        {
            uint32_t mask;
            mask = getGlobalTriggerMask();
            return EasyGlobalTriggerMaskHelper(mask);
        }
        /**
         * @brief Easy Set GlobalTriggerMaskHelper
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
        void setEasyGlobalTriggerMaskHelper(EasyGlobalTriggerMaskHelper settings) override
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
         * EasyFrontPanelTRGOUTEnableMask structure
         */
        EasyFrontPanelTRGOUTEnableMask getEasyFrontPanelTRGOUTEnableMask() override
        {
            uint32_t mask;
            mask = getFrontPanelTRGOUTEnableMask();
            return bits2efptoem(mask);
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
         * EasyFrontPanelTRGOUTEnableMask structure
         */
        void setEasyFrontPanelTRGOUTEnableMask(EasyFrontPanelTRGOUTEnableMask settings) override
        {
            uint32_t mask = efptoem2bits(settings);
            setFrontPanelTRGOUTEnableMask(mask);
        }
        /**
         * @brief Easy Get FrontPanelTRGOUTEnableMaskHelper
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
        EasyFrontPanelTRGOUTEnableMaskHelper getEasyFrontPanelTRGOUTEnableMaskHelper() override
        {
            uint32_t mask;
            mask = getFrontPanelTRGOUTEnableMask();
            return EasyFrontPanelTRGOUTEnableMaskHelper(mask);
        }
        /**
         * @brief Easy Set FrontPanelTRGOUTEnableMaskHelper
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
        void setEasyFrontPanelTRGOUTEnableMaskHelper(EasyFrontPanelTRGOUTEnableMaskHelper settings) override
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
         * EasyFrontPanelIOControl structure
         */
        EasyFrontPanelIOControl getEasyFrontPanelIOControl() override
        {
            uint32_t mask;
            mask = getFrontPanelIOControl();
            return bits2efpioc(mask);
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
         * EasyFrontPanelIOControl structure
         */
        void setEasyFrontPanelIOControl(EasyFrontPanelIOControl settings) override
        {
            uint32_t mask = efpioc2bits(settings);
            setFrontPanelIOControl(mask);
        }
        /**
         * @brief Easy Get FrontPanelIOControlHelper
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
        EasyFrontPanelIOControlHelper getEasyFrontPanelIOControlHelper() override
        {
            uint32_t mask;
            mask = getFrontPanelIOControl();
            return EasyFrontPanelIOControlHelper(mask);
        }
        /**
         * @brief Easy Set FrontPanelIOControlHelper
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
        void setEasyFrontPanelIOControlHelper(EasyFrontPanelIOControlHelper settings) override
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
            return bits2erffr(mask);
        }
        /**
         * @brief Easy Get ROCFPGAFirmwareRevisionHelper
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
        EasyROCFPGAFirmwareRevisionHelper getEasyROCFPGAFirmwareRevisionHelper() override
        {
            uint32_t mask;
            mask = getROCFPGAFirmwareRevision();
            return EasyROCFPGAFirmwareRevisionHelper(mask);
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
         * EasyFanSpeedControl structure
         */
        EasyFanSpeedControl getEasyFanSpeedControl() override
        {
            uint32_t mask;
            mask = getFanSpeedControl();
            return bits2efsc(mask);
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
         * EasyFanSpeedControl structure
         */
        void setEasyFanSpeedControl(EasyFanSpeedControl settings) override
        {
            uint32_t mask = efsc2bits(settings);
            setFanSpeedControl(mask);
        }
        /**
         * @brief Easy Get FanSpeedControlHelper
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating from the bit mask returned by the
         * the underlying low-level get funtion.
         *
         * @returns
         * EasyFanSpeedControlHelper object
         */
        EasyFanSpeedControlHelper getEasyFanSpeedControlHelper() override
        {
            uint32_t mask;
            mask = getFanSpeedControl();
            return EasyFanSpeedControlHelper(mask);
        }
        /**
         * @brief Easy Set FanSpeedControlHelper
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
        void setEasyFanSpeedControlHelper(EasyFanSpeedControlHelper settings) override
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
         * @brief Easy Get ReadoutControlHelper
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating from the bit mask returned by the
         * the underlying low-level get funtion.
         *
         * @returns
         * EasyReadoutControlHelper object
         */
        EasyReadoutControlHelper getEasyReadoutControlHelper() override
        {
            uint32_t mask;
            mask = getReadoutControl();
            return EasyReadoutControlHelper(mask);
        }
        /**
         * @brief Easy Set ReadoutControlHelper
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
        void setEasyReadoutControlHelper(EasyReadoutControlHelper settings) override
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
         * @brief Easy Get ReadoutStatusHelper
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating from the bit mask returned by the
         * the underlying low-level get funtion.
         *
         * @returns
         * EasyReadoutStatusHelper object
         */
        virtual EasyReadoutStatusHelper getEasyReadoutStatusHelper() override
        {
            uint32_t mask;
            mask = getReadoutStatus();
            return EasyReadoutStatusHelper(mask);
        }

        /* TODO: wrap Board ID from register docs? */
        /* TODO: wrap MCST Base Address and Control from register docs? */
        /* TODO: wrap Relocation Address from register docs? */
        /* TODO: wrap Interrupt Status/ID from register docs? */
        /* TODO: wrap Interrupt Event Number from register docs? */

        /* TODO: wrap Scratch from register docs? */
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
         * EasyDPPAlgorithmControl structure
         */
        EasyDPPAlgorithmControl getEasyDPPAlgorithmControl(uint32_t group) override
        {
            uint32_t mask = getDPPAlgorithmControl(group);
            return bits2edppac(mask);
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
         * EasyDPPAlgorithmControl structure
         */
        void setEasyDPPAlgorithmControl(uint32_t group, EasyDPPAlgorithmControl settings) override
        {
            uint32_t mask = edppac2bits(settings);
            setDPPAlgorithmControl(group, mask);
        }
        /**
         * @brief Broadcast version: please refer to details in
         * single-group version.
         */
        void setEasyDPPAlgorithmControl(EasyDPPAlgorithmControl settings) override
        {
            uint32_t mask = edppac2bits(settings);
            setDPPAlgorithmControl(mask);
        }
        /**
         * @brief Easy Get DPPAlgorithmControlHelper
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
        EasyDPPAlgorithmControlHelper getEasyDPPAlgorithmControlHelper(uint32_t group) override
        {
            uint32_t mask = getDPPAlgorithmControl(group);
            return EasyDPPAlgorithmControlHelper(mask);
        }
        /**
         * @brief Easy Set DPPAlgorithmControlHelper
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
        void setEasyDPPAlgorithmControlHelper(uint32_t group, EasyDPPAlgorithmControlHelper settings) override
        {
            uint32_t mask = settings.toBits();
            setDPPAlgorithmControl(group, mask);
        }
        /**
         * @brief Broadcast version: please refer to details in
         * single-group version.
         */
        void setEasyDPPAlgorithmControlHelper(EasyDPPAlgorithmControlHelper settings) override
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
        uint32_t getDPPShapedTriggerWidth(uint32_t group) override
        {
            if (group >= groups())
                errorHandler(CAEN_DGTZ_InvalidChannelNumber);
            uint32_t value;
            errorHandler(CAEN_DGTZ_ReadRegister(handle_, 0x1078 | group<<8 , &value));
            return value;
        }
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
        void setDPPShapedTriggerWidth(uint32_t group, uint32_t value) override
        {
            if (group >= groups())
                errorHandler(CAEN_DGTZ_InvalidChannelNumber);
            errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x1078 | group<<8, value & 0xFFFF));
        }
        /**
         * @brief Broadcast version: please refer to details in
         * single-group version.
         */
        void setDPPShapedTriggerWidth(uint32_t value) override
        { errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x8078, value & 0xFFFF)); }

        /* NOTE: reuse get AMCFirmwareRevision from parent */
        /* NOTE: disable inherited 740 EasyAMCFirmwareRevision since we only
         * support EasyDPPAMCFirmwareRevision here. */
        /**
         * @brief use getEasyDPPX version instead
         */
        virtual EasyAMCFirmwareRevision getEasyAMCFirmwareRevision(uint32_t group) override { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        /**
         * @brief use getEasyDPPX version instead
         */
        virtual EasyAMCFirmwareRevisionHelper getEasyAMCFirmwareRevisionHelper(uint32_t group) override { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
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
         * EasyDPPAMCFirmwareRevision structure
         */
        EasyDPPAMCFirmwareRevision getEasyDPPAMCFirmwareRevision(uint32_t group) override
        {
            uint32_t mask;
            mask = getAMCFirmwareRevision(group);
            return bits2edafr(mask);
        }
        /**
         * @brief Easy Get DPP AMCFirmwareRevisionHelper
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
         * EasyAMCFirmwareRevisionHelper object
         */
        EasyDPPAMCFirmwareRevisionHelper getEasyDPPAMCFirmwareRevisionHelper(uint32_t group) override
        {
            uint32_t mask;
            mask = getAMCFirmwareRevision(group);
            return EasyDPPAMCFirmwareRevisionHelper(mask);
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
         * @brief use unsetEasyDPPX version instead
         */
        virtual void unsetEasyBoardConfiguration(EasyBoardConfiguration settings) override { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

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
         * EasyDPPBoardConfiguration structure
         */
        EasyDPPBoardConfiguration getEasyDPPBoardConfiguration() override
        {
            uint32_t mask;
            mask = getBoardConfiguration();
            return bits2edbc(mask);
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
         * @param settings:
         * EasyDPPBoardConfiguration structure
         */
        void setEasyDPPBoardConfiguration(EasyDPPBoardConfiguration settings) override
        {
            /* NOTE: according to DPP register docs individualTrigger,
             * timeStampRecording and chargeRecording MUST all be 1 */
            assert(settings.individualTrigger == 1);
            assert(settings.timeStampRecording == 1);
            assert(settings.chargeRecording == 1);
            uint32_t mask = edbc2bits(settings);
            setBoardConfiguration(mask);
        }
        /**
         * @brief Easy Unset DPP BoardConfiguration
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating to the bit mask needed by the
         * the underlying low-level unset funtion.
         *
         * @param settings:
         * EasyDPPBoardConfiguration structure
         */
        void unsetEasyDPPBoardConfiguration(EasyDPPBoardConfiguration settings) override
        {
            /* NOTE: according to docs individualTrigger, timeStampRecording
             * and chargeRecording MUST all be 1. Thus we do NOT allow
             * unset.
             */
            assert(settings.individualTrigger == 0);
            assert(settings.timeStampRecording == 0);
            assert(settings.chargeRecording == 0);
            uint32_t mask = edbc2bits(settings);
            unsetBoardConfiguration(mask);
        }
        /**
         * @brief use getEasyDPPX version instead
         */
        virtual EasyBoardConfigurationHelper getEasyBoardConfigurationHelper() override { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        /**
         * @brief use setEasyDPPX version instead
         */
        virtual void setEasyBoardConfigurationHelper(EasyBoardConfigurationHelper settings) override { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        /**
         * @brief use unsetEasyDPPX version instead
         */
        virtual void unsetEasyBoardConfigurationHelper(EasyBoardConfigurationHelper settings) override { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        /**
         * @brief Easy Get DPP BoardConfigurationHelper
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
        EasyDPPBoardConfigurationHelper getEasyDPPBoardConfigurationHelper() override
        {
            uint32_t mask;
            mask = getBoardConfiguration();
            return EasyDPPBoardConfigurationHelper(mask);
        }
        /**
         * @brief Easy Set DPP BoardConfigurationHelper
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating to the bit mask needed by the
         * the underlying low-level set funtion.
         *
         * @param settings:
         * EasyDPPBoardConfiguration object
         */
        void setEasyDPPBoardConfigurationHelper(EasyDPPBoardConfigurationHelper settings) override
        {
            /* TODO: add check for forced ones here or in EasyHelper? */
            uint32_t mask = settings.toBits();
            setBoardConfiguration(mask);
        }
        /**
         * @brief Easy Unset DPP BoardConfigurationHelper
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating to the bit mask needed by the
         * the underlying low-level unset funtion.
         *
         * @param settings:
         * EasyDPPBoardConfiguration object
         */
        void unsetEasyDPPBoardConfigurationHelper(EasyDPPBoardConfigurationHelper settings) override
        {
            /* TODO: add check for forced ones here or in EasyHelper? */
            uint32_t mask = settings.toBits();
            unsetBoardConfiguration(mask);
        }

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
        uint32_t getDPPAggregateOrganization() override
        {
            uint32_t value;
            errorHandler(CAEN_DGTZ_ReadRegister(handle_, 0x800C, &value));
            return value;
        }
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
        void setDPPAggregateOrganization(uint32_t value) override
        { errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x800C, value & 0x0F)); }

        /* TODO: check that this is already covered by NumEventsPerAggregate

        uint32_t getEventsPerAggregate() override
        {
            uint32_t value;
            errorHandler(CAEN_DGTZ_ReadRegister(handle_, 0x8020, &value));
            return value;
        }
        void setEventsPerAggregate(uint32_t value) override
        { errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x8020, value & 0x07FF)); }
        */

        /* TODO: make sure Record Length is already covered by RecordLength */

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
         * @brief use getEasyDPPX version instead
         */
        virtual EasyAcquisitionControlHelper getEasyAcquisitionControlHelper() override { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        /**
         * @brief use setEasyDPPX version instead
         */
        virtual void setEasyAcquisitionControlHelper(EasyAcquisitionControlHelper settings) override { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
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
         * EasyDPPAcquisitionControl structure
         */
        EasyDPPAcquisitionControl getEasyDPPAcquisitionControl() override
        {
            uint32_t mask;
            mask = getAcquisitionControl();
            return bits2edac(mask);
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
         * EasyDPPAcquisitionControl structure
         */
        void setEasyDPPAcquisitionControl(EasyDPPAcquisitionControl settings) override
        {
            uint32_t mask = edac2bits(settings);
            setAcquisitionControl(mask);
        }
        /**
         * @brief Easy Get DPP AcquisitionControlHelper
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
        EasyDPPAcquisitionControlHelper getEasyDPPAcquisitionControlHelper() override
        {
            uint32_t mask;
            mask = getAcquisitionControl();
            return EasyDPPAcquisitionControlHelper(mask);
        }
        /**
         * @brief Easy Set DPP AcquisitionControlHelper
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
        void setEasyDPPAcquisitionControlHelper(EasyDPPAcquisitionControlHelper settings) override
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
         * @brief use getEasyDPPX version instead
         */
        virtual EasyAcquisitionStatusHelper getEasyAcquisitionStatusHelper() override { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        /**
         * @brief Easy Get DPPAcquisitionStatus
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating from the bit mask returned by the
         * the underlying low-level get funtion.
         *
         * @returns
         * EasyDPPAcquisitionStatus structure
         */
        EasyDPPAcquisitionStatus getEasyDPPAcquisitionStatus() override
        {
            uint32_t mask;
            mask = getAcquisitionStatus();
            return bits2edas(mask);
        }
        /**
         * @brief Easy Get DPP AcquisitionStatusHelper
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
        EasyDPPAcquisitionStatusHelper getEasyDPPAcquisitionStatusHelper() override
        {
            uint32_t mask;
            mask = getAcquisitionStatus();
            return EasyDPPAcquisitionStatusHelper(mask);
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
         * @brief use getEasyDPPX version instead
         */
        virtual EasyGlobalTriggerMaskHelper getEasyGlobalTriggerMaskHelper() override { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        /**
         * @brief use setEasyDPPX version instead
         */
        virtual void setEasyGlobalTriggerMaskHelper(EasyGlobalTriggerMaskHelper settings) override { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
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
         * EasyDPPGlobalTriggerMask structure
         */
        EasyDPPGlobalTriggerMask getEasyDPPGlobalTriggerMask() override
        {
            uint32_t mask;
            mask = getGlobalTriggerMask();
            return bits2edgtm(mask);
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
         * EasyDPPGlobalTriggerMask structure
         */
        void setEasyDPPGlobalTriggerMask(EasyDPPGlobalTriggerMask settings) override
        {
            uint32_t mask = edgtm2bits(settings);
            setGlobalTriggerMask(mask);
        }
        /**
         * @brief Easy Get DPP GlobalTriggerMaskHelper
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
        EasyDPPGlobalTriggerMaskHelper getEasyDPPGlobalTriggerMaskHelper() override
        {
            uint32_t mask;
            mask = getGlobalTriggerMask();
            return EasyDPPGlobalTriggerMaskHelper(mask);
        }
        /**
         * @brief Easy Set DPP GlobalTriggerMaskHelper
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
        void setEasyDPPGlobalTriggerMaskHelper(EasyDPPGlobalTriggerMaskHelper settings) override
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
         * @brief use getEasyDPPX version instead
         */
        virtual EasyFrontPanelTRGOUTEnableMaskHelper getEasyFrontPanelTRGOUTEnableMaskHelper() override { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        /**
         * @brief use setEasyDPPX version instead
         */
        virtual void setEasyFrontPanelTRGOUTEnableMaskHelper(EasyFrontPanelTRGOUTEnableMaskHelper settings) override { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
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
         * EasyDPPFrontPanelTRGOUTEnableMask structure
         */
        EasyDPPFrontPanelTRGOUTEnableMask getEasyDPPFrontPanelTRGOUTEnableMask() override
        {
            uint32_t mask;
            mask = getFrontPanelTRGOUTEnableMask();
            return bits2edfptoem(mask);
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
         * EasyDPPFrontPanelTRGOUTEnableMask structure
         */
        void setEasyDPPFrontPanelTRGOUTEnableMask(EasyDPPFrontPanelTRGOUTEnableMask settings) override
        {
            uint32_t mask = edfptoem2bits(settings);
            setFrontPanelTRGOUTEnableMask(mask);
        }
        /**
         * @brief Easy Get DPP FrontPanelTRGOUTEnableMaskHelper
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
        EasyDPPFrontPanelTRGOUTEnableMaskHelper getEasyDPPFrontPanelTRGOUTEnableMaskHelper() override
        {
            uint32_t mask;
            mask = getFrontPanelTRGOUTEnableMask();
            return EasyDPPFrontPanelTRGOUTEnableMaskHelper(mask);
        }
        /**
         * @brief Easy Set DPP FrontPanelTRGOUTEnableMaskHelper
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
        void setEasyDPPFrontPanelTRGOUTEnableMaskHelper(EasyDPPFrontPanelTRGOUTEnableMaskHelper settings) override
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
         * @brief use getEasyDPPX version instead
         */
        virtual EasyFrontPanelIOControlHelper getEasyFrontPanelIOControlHelper() override { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        /**
         * @brief use setEasyDPPX version instead
         */
        virtual void setEasyFrontPanelIOControlHelper(EasyFrontPanelIOControlHelper settings) override { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        /* NOTE: 740 and 740DPP share this structure - just re-expose it */
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
         * EasyDPPFrontPanelIOControl structure
         */
        EasyDPPFrontPanelIOControl getEasyDPPFrontPanelIOControl() override
        { return (EasyDPPFrontPanelIOControl) Digitizer740::getEasyFrontPanelIOControl(); }
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
         * EasyDPPFrontPanelIOControl structure
         */
        void setEasyDPPFrontPanelIOControl(EasyDPPFrontPanelIOControl settings) override
        { return setEasyFrontPanelIOControl((EasyFrontPanelIOControl)settings); }
        /**
         * @brief Easy Get DPP FrontPanelIOControlHelper
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
        EasyDPPFrontPanelIOControlHelper getEasyDPPFrontPanelIOControlHelper() override
        {
            uint32_t mask;
            mask = getFrontPanelIOControl();
            return EasyDPPFrontPanelIOControlHelper(mask);
        }
        /**
         * @brief Easy Set DPP FrontPanelIOControlHelper
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
        void setEasyDPPFrontPanelIOControlHelper(EasyDPPFrontPanelIOControlHelper settings) override
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
         * @brief use getEasyDPPX version instead
         */
        virtual EasyROCFPGAFirmwareRevisionHelper getEasyROCFPGAFirmwareRevisionHelper() override { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        /* NOTE: 740 and 740DPP share this structure - just re-expose it */
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
         * EasyDPPROCFPGAFirmwareRevision structure
         */
        EasyDPPROCFPGAFirmwareRevision getEasyDPPROCFPGAFirmwareRevision() override
        { return (EasyDPPROCFPGAFirmwareRevision)Digitizer740::getEasyROCFPGAFirmwareRevision(); }
        /**
         * @brief Easy Get DPP ROCFPGAFirmwareRevisionHelper
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
        EasyDPPROCFPGAFirmwareRevisionHelper getEasyDPPROCFPGAFirmwareRevisionHelper() override
        {
            uint32_t mask;
            mask = getROCFPGAFirmwareRevision();
            return EasyDPPROCFPGAFirmwareRevisionHelper(mask);
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
         * @brief use getEasyDPPX version instead
         */
        virtual EasyFanSpeedControlHelper getEasyFanSpeedControlHelper() override { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        /**
         * @brief use setEasyDPPX version instead
         */
        virtual void setEasyFanSpeedControlHelper(EasyFanSpeedControlHelper settings) override { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        /* NOTE: 740 and 740DPP share this structure - just re-expose it */
        /**
         * @brief Easy Get DPP FanSpeedControl
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating from the bit mask returned by the
         * the underlying low-level get funtion.
         *
         * @returns
         * EasyDPPFanSpeedControl structure
         */
        EasyDPPFanSpeedControl getEasyDPPFanSpeedControl() override
        { return (EasyDPPFanSpeedControl) Digitizer740::getEasyFanSpeedControl(); }
        /**
         * @brief Easy Set DPP FanSpeedControl
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating to the bit mask needed by the
         * the underlying low-level set funtion.
         *
         * @param settings:
         * EasyDPPFanSpeedControl structure
         */
        void setEasyDPPFanSpeedControl(EasyDPPFanSpeedControl settings) override
        { return setEasyFanSpeedControl((EasyFanSpeedControl)settings); }
        /**
         * @brief Easy Get FanSpeedControlHelper
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating from the bit mask returned by the
         * the underlying low-level get funtion.
         *
         * @returns
         * EasyFanSpeedControlHelper object
         */
        EasyDPPFanSpeedControlHelper getEasyDPPFanSpeedControlHelper() override
        {
            uint32_t mask;
            mask = getFanSpeedControl();
            return EasyDPPFanSpeedControlHelper(mask);
        }
        /**
         * @brief Easy Set FanSpeedControlHelper
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
        void setEasyDPPFanSpeedControlHelper(EasyDPPFanSpeedControlHelper settings) override
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
        virtual EasyReadoutControlHelper getEasyReadoutControlHelper() override { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        /**
         * @brief use setEasyDPPX version instead
         */
        virtual void setEasyReadoutControlHelper(EasyReadoutControlHelper settings) override { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        /**
         * @brief Easy Get DPP ReadoutControlHelper
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating from the bit mask returned by the
         * the underlying low-level get funtion.
         *
         * @returns
         * EasyDPPReadoutControlHelper object
         */
        EasyDPPReadoutControlHelper getEasyDPPReadoutControlHelper() override
        {
            uint32_t mask;
            mask = getReadoutControl();
            return EasyDPPReadoutControlHelper(mask);
        }
        /**
         * @brief Easy Set DPP ReadoutControlHelper
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
        void setEasyDPPReadoutControlHelper(EasyDPPReadoutControlHelper settings) override
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
        virtual EasyReadoutStatusHelper getEasyReadoutStatusHelper() override { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        /**
         * @brief Easy Get DPP ReadoutStatusHelper
         *
         * A convenience wrapper for the low-level function of the same
         * name. Works on a struct with named variables rather than
         * directly manipulating obscure bit patterns. Automatically
         * takes care of translating from the bit mask returned by the
         * the underlying low-level get funtion.
         *
         * @returns
         * EasyDPPReadoutStatusHelper object
         */
        virtual EasyDPPReadoutStatusHelper getEasyDPPReadoutStatusHelper() override
        {
            uint32_t mask;
            mask = getReadoutStatus();
            return EasyDPPReadoutStatusHelper(mask);
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
