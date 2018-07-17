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
    struct ReadoutBuffer
    {
        char* data = nullptr;
        uint32_t size = 0;
        uint32_t dataSize = 0;
        char* begin() const { return data;}
        char* end() const { return data+dataSize;}
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

    struct DPPEvents_t
    {
        virtual void** data() = 0;
        uint32_t nEvents[MAX_DPP_CHANNEL_SIZE];// Number of events pr channel
        uint32_t allocatedSize = 0;
    };

    template <typename T>
    struct DPPEvents: public DPPEvents_t
    {
        T* ptr[MAX_DPP_CHANNEL_SIZE];
        void** data() override { return (void**)ptr;}
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
        void* ptr = nullptr;
        uint32_t allocatedSize = 0;
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
        Digitizer()
        {}

        Digitizer(int handle) : handle_(handle)
        { boardInfo_ = getRawDigitizerBoardInfo(handle_); }

    protected:
        int handle_;
        CAEN_DGTZ_BoardInfo_t boardInfo_;

        Digitizer(int handle, CAEN_DGTZ_BoardInfo_t boardInfo) : handle_(handle)
        { boardInfo_ = boardInfo; }

        virtual uint32_t filterBoardConfigurationSetMask(uint32_t mask)
        { return mask; }

        virtual uint32_t filterBoardConfigurationUnsetMask(uint32_t mask)
        { return mask; }

    public:
    public:

        /* Digitizer creation */
        static Digitizer *open(CAEN_DGTZ_ConnectionType linkType, int linkNum, int conetNode, uint32_t VMEBaseAddress);

        /**
         * @brief Instantiate Digitizer from USB device.
         * @param linkNum: device index on the bus
         * @returns
         * Abstracted digitizer instance for the device.
         */
        static Digitizer *USB(int linkNum)
        { return open(CAEN_DGTZ_USB, linkNum, 0, 0); }

        /**
         * @brief Instantiate Digitizer from USB device.
         * @param linkNum: device index on the bus
         * @param VMEBaseAddress: device address if using VME connection
         * @returns
         * Abstracted digitizer instance for the device.
         */
        static Digitizer *USB(int linkNum, uint32_t VMEBaseAddress)
        { return open(CAEN_DGTZ_USB, linkNum, 0, VMEBaseAddress); }

        /**
         * @brief Instantiate Digitizer from Optical connection.
         * @param linkNum: device index on the bus
         * @param conetNode: node id if using optical connection
         * @returns
         * Abstracted digitizer instance for the device.
         */
        static Digitizer *Optical(int linkNum, int conetNode)
        { return open(CAEN_DGTZ_OpticalLink, linkNum, conetNode, 0); }

        /* Destruction */
        /**
         * @brief close Digitizer instance.
         * @param handle: low-level device handle
         */
        static void close(int handle)
        { closeRawDigitizer(handle); }

        /**
         * @brief Destroy Digitizer instance.
         */
        virtual ~Digitizer()
        { close(handle_); }

        /* Information functions */
        const std::string modelName() const
        { return std::string(boardInfo_.ModelName); }

        uint32_t modelNo() const
        { return boardInfo_.Model; }

        virtual uint32_t channels() const
        { return boardInfo_.Channels; }

        // by default groups do not exists. I.e. one channel pr. group
        virtual uint32_t groups() const
        { return boardInfo_.Channels; }

        virtual uint32_t channelsPerGroup() const
        { return 1; }

        uint32_t formFactor() const
        { return boardInfo_.FormFactor; }

        uint32_t familyCode() const
        { return boardInfo_.FamilyCode; }

        const std::string ROCfirmwareRel() const
        { return std::string(boardInfo_.ROC_FirmwareRel); }

        const std::string AMCfirmwareRel() const
        { return std::string(boardInfo_.AMC_FirmwareRel); }

        uint32_t serialNumber() const
        { return boardInfo_.SerialNumber; }

        uint32_t PCBrevision() const
        { return boardInfo_.PCB_Revision; }

        uint32_t ADCbits() const
        { return boardInfo_.ADC_NBits; }

        int commHandle() const
        { return boardInfo_.CommHandle; }

        int VMEhandle() const
        { return boardInfo_.VMEHandle; }

        const std::string license() const
        { return std::string(boardInfo_.License); }

        int handle()
        { return handle_; }

        CAEN_DGTZ_DPPFirmware_t getDPPFirmwareType()
        {
            CAEN_DGTZ_DPPFirmware_t firmware = CAEN_DGTZ_NotDPPFirmware;
            errorHandler(_CAEN_DGTZ_GetDPPFirmwareType(handle_, &firmware));
            return firmware;
        }

        /* Raw register read/write functions */
        void writeRegister(uint32_t address, uint32_t value)
        { errorHandler(CAEN_DGTZ_WriteRegister(handle_, address, value)); }

        uint32_t readRegister(uint32_t address)
        {
            uint32_t value;
            errorHandler(CAEN_DGTZ_ReadRegister(handle_, address, &value));
            return value;
        }

        /* Utility functions */
        void reset()
        { errorHandler(CAEN_DGTZ_Reset(handle_)); }

        void calibrate()
        { errorHandler(CAEN_DGTZ_Calibrate(handle_)); }

        uint32_t readTemperature(int32_t ch)
        {
            uint32_t temp;
            errorHandler(CAEN_DGTZ_ReadTemperature(handle_, ch, &temp));
            return temp;
        }

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
        {
            CAEN_DGTZ_DRS4Correction_t ctable;
            errorHandler(CAEN_DGTZ_GetCorrectionTables(handle_, frequency, &ctable));
            return ctable;
        }

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

        ReadoutBuffer &readData(ReadoutBuffer &buffer, CAEN_DGTZ_ReadMode_t mode)
        {
            errorHandler(CAEN_DGTZ_ReadData(handle_, mode, buffer.data, &buffer.dataSize));
            return buffer;
        }

        /* Interrupt control */

        /* NOTE: interrupts cannot be used in case of communication via
         * USB (either directly or through V1718 and VME)
         */

        InterruptConfig getInterruptConfig()
        {
            InterruptConfig conf;
            errorHandler(
                    CAEN_DGTZ_GetInterruptConfig(handle_, &conf.state, &conf.level, &conf.status_id, &conf.event_number,
                                                 &conf.mode));
            return conf;
        }

        void setInterruptConfig(InterruptConfig conf)
        {
            errorHandler(
                    CAEN_DGTZ_SetInterruptConfig(handle_, conf.state, conf.level, conf.status_id, conf.event_number,
                                                 conf.mode));
        }

        void doIRQWait(uint32_t timeout)
        { errorHandler(CAEN_DGTZ_IRQWait(handle_, timeout)); }

        /* NOTE: VME* calls are for VME bus interrupts and work on a
         * separate VME handle. Not sure if they are needed. 
         */
        int
        doVMEIRQWait(CAEN_DGTZ_ConnectionType LinkType, int LinkNum, int ConetNode, uint8_t IRQMask, uint32_t timeout)
        {
            int vmehandle;
            errorHandler(CAEN_DGTZ_VMEIRQWait(LinkType, LinkNum, ConetNode, IRQMask, timeout, &vmehandle));
            return vmehandle;
        }

        uint8_t doVMEIRQCheck(int vmehandle)
        {
            uint8_t mask;
            errorHandler(CAEN_DGTZ_VMEIRQCheck(vmehandle, &mask));
            return mask;
        }

        int32_t doVMEIACKCycle(int vmehandle, uint8_t level)
        {
            int32_t board_id;
            errorHandler(CAEN_DGTZ_VMEIACKCycle(vmehandle, level, &board_id));
            return board_id;
        }

        void rearmInterrupt()
        { errorHandler(CAEN_DGTZ_RearmInterrupt(handle_)); }

        /* Memory management */
        ReadoutBuffer mallocReadoutBuffer()
        {
            ReadoutBuffer buffer;
            errorHandler(_CAEN_DGTZ_MallocReadoutBuffer(handle_, &buffer.data, &buffer.size));
            return buffer;
        }

        void freeReadoutBuffer(ReadoutBuffer buffer)
        {
            if (buffer.data != nullptr)
            {
                errorHandler(CAEN_DGTZ_FreeReadoutBuffer(&buffer.data));
                buffer.size = 0;
            }
        }

        // TODO Think of an intelligent way to handle events not using void pointers
        void *mallocEvent()
        {
            void *event;
            errorHandler(CAEN_DGTZ_AllocateEvent(handle_, &event));
            return event;
        }

        void freeEvent(void *event)
        { errorHandler(CAEN_DGTZ_FreeEvent(handle_, (void **) &event)); }

        DPPEvents_t *mallocDPPEvents(CAEN_DGTZ_DPPFirmware_t firmware)
        {
            DPPEvents_t *events;
            switch ((int)firmware) //Cast to int as long as CAEN_DGTZ_DPPFirmware_QDC is not part of the enumeration
            {
                case CAEN_DGTZ_DPPFirmware_PHA:
                    events = new DPPEvents <CAEN_DGTZ_DPP_PHA_Event_t>{};
                    break;
                case CAEN_DGTZ_DPPFirmware_PSD:
                    events = new DPPEvents <CAEN_DGTZ_DPP_PSD_Event_t>{};
                    break;
                case CAEN_DGTZ_DPPFirmware_CI:
                    events = new DPPEvents <CAEN_DGTZ_DPP_CI_Event_t>{};
                    break;
                case CAEN_DGTZ_DPPFirmware_ZLE:
                    throw std::runtime_error("ZLE Firmware not supported by DPPEvents_t.");
                    break;
                case CAEN_DGTZ_DPPFirmware_QDC:
                    events = new DPPEvents <_CAEN_DGTZ_DPP_QDC_Event_t>{};
                    break;
                case CAEN_DGTZ_NotDPPFirmware:
                    throw std::runtime_error("Non DPP firmware not supported by DPPEvents_t.");
                    break;
                default:
                    throw std::runtime_error("Unknown firmware type. Not supported by Digitizer.");
            }
            errorHandler(_CAEN_DGTZ_MallocDPPEvents(handle_, events->data(), &(events->allocatedSize)));
            return events;
        }
        virtual DPPEvents_t *mallocDPPEvents() { return mallocDPPEvents(getDPPFirmwareType()); }

        void freeDPPEvents(DPPEvents_t* events)
        { if (events->allocatedSize < 0) errorHandler(_CAEN_DGTZ_FreeDPPEvents(handle_, events->data())); events->allocatedSize = 0; }

        DPPWaveforms mallocDPPWaveforms()
        { DPPWaveforms waveforms; errorHandler(_CAEN_DGTZ_MallocDPPWaveforms(handle_, &waveforms.ptr, &waveforms.allocatedSize)); return waveforms; }
        void freeDPPWaveforms(DPPWaveforms waveforms) {
            if (waveforms.ptr != nullptr) {
                errorHandler(_CAEN_DGTZ_FreeDPPWaveforms(handle_, waveforms.ptr));
                waveforms.ptr = nullptr;
                waveforms.allocatedSize = 0;
            }
        }

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

        void decodeEvent(EventInfo info, void* event)
        { errorHandler(CAEN_DGTZ_DecodeEvent(handle_, info.data, &event)); }

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

        void getDPPEvents(ReadoutBuffer buffer, DPPEvents_t* events)
        { errorHandler(_CAEN_DGTZ_GetDPPEvents(handle_, buffer.data, buffer.dataSize, events->data(), events->nEvents)); }

        DPPWaveforms& decodeDPPWaveforms(void *event, DPPWaveforms& waveforms)
        {
            errorHandler(_CAEN_DGTZ_DecodeDPPWaveforms(handle_, event, waveforms.ptr));
            return waveforms;
        }
        /*
        DPPWaveforms& decodeDPPWaveforms(DPPEvents& events, uint32_t channel, uint32_t eventNo, DPPWaveforms& waveforms)
        {
            void *event = extractDPPEvent(events, channel, eventNo);
            return decodeDPPWaveforms(event, waveforms);
        }*/



        /* Device configuration - i.e. getter and setters */
        virtual uint32_t getRecordLength(uint32_t channel)
        { uint32_t size; errorHandler(CAEN_DGTZ_GetRecordLength(handle_,&size,channel)); return size; }
        virtual uint32_t getRecordLength()
        { uint32_t size; errorHandler(CAEN_DGTZ_GetRecordLength(handle_,&size)); return size; }
        virtual void setRecordLength(uint32_t size)
        { errorHandler(CAEN_DGTZ_SetRecordLength(handle_,size)); }
        virtual void setRecordLength(uint32_t channel, uint32_t size)
        { errorHandler(CAEN_DGTZ_SetRecordLength(handle_,size,channel)); }

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

        uint32_t getChannelDCOffset(uint32_t channel)
        { uint32_t offset; errorHandler(CAEN_DGTZ_GetChannelDCOffset(handle_, channel, &offset)); return offset; }
        void setChannelDCOffset(uint32_t channel, uint32_t offset)
        { errorHandler(CAEN_DGTZ_SetChannelDCOffset(handle_, channel, offset)); }

        uint32_t getGroupDCOffset(uint32_t channel)
        { uint32_t offset; errorHandler(CAEN_DGTZ_GetGroupDCOffset(handle_, channel, &offset)); return offset; }
        void setGroupDCOffset(uint32_t channel, uint32_t offset)
        { errorHandler(CAEN_DGTZ_SetGroupDCOffset(handle_, channel, offset)); }

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

        virtual uint32_t getDPPPreTriggerSize(uint32_t channel=-1) // Default channel -1 == all
        { uint32_t samples; errorHandler(CAEN_DGTZ_GetDPPPreTriggerSize(handle_, channel, &samples)); return samples; }
        virtual void setDPPPreTriggerSize(uint32_t channel, uint32_t samples)
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
        {
            ChannelPairTriggerLogicParams params;
            errorHandler(CAEN_DGTZ_GetChannelPairTriggerLogic(handle_, channelA, channelB, &params.logic, &params.coincidenceWindow));
            return params;
        }
        void setChannelPairTriggerLogic(uint32_t channelA, uint32_t channelB, ChannelPairTriggerLogicParams params)
        { errorHandler(CAEN_DGTZ_SetChannelPairTriggerLogic(handle_, channelA, channelB, params.logic, params.coincidenceWindow)); }

        TriggerLogicParams getTriggerLogic()
        {
            TriggerLogicParams params;
            errorHandler(CAEN_DGTZ_GetTriggerLogic(handle_, &params.logic, &params.majorityLevel));
            return params;
        }
        void setTriggerLogic(TriggerLogicParams params)
        { errorHandler(CAEN_DGTZ_SetTriggerLogic(handle_, params.logic, params.majorityLevel)); }

        SAMTriggerCountVetoParams getSAMTriggerCountVetoParam(int channel)
        {
            SAMTriggerCountVetoParams params;
            errorHandler(CAEN_DGTZ_GetSAMTriggerCountVetoParam(handle_, channel, &params.enable, &params.vetoWindow));
            return params;
        }
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

        virtual uint32_t getAMCFirmwareRevision(uint32_t group) { throw Error(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getRunDelay() { throw Error(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setRunDelay(uint32_t delay) { throw Error(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getDPPGateWidth(uint32_t group) { throw Error(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setDPPGateWidth(uint32_t group, uint32_t value) { throw Error(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setDPPGateWidth(uint32_t value) { throw Error(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getDPPGateOffset(uint32_t group) { throw Error(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setDPPGateOffset(uint32_t group, uint32_t value) { throw Error(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setDPPGateOffset(uint32_t value) { throw Error(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getDPPFixedBaseline(uint32_t group) { throw Error(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setDPPFixedBaseline(uint32_t group, uint32_t value) { throw Error(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setDPPFixedBaseline(uint32_t value) { throw Error(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getDPPTriggerHoldOffWidth(uint32_t group) { throw Error(CAEN_DGTZ_FunctionNotAllowed); }
        virtual uint32_t getDPPTriggerHoldOffWidth() { throw Error(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setDPPTriggerHoldOffWidth(uint32_t group, uint32_t value) { throw Error(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setDPPTriggerHoldOffWidth(uint32_t value) { throw Error(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getDPPAlgorithmControl(uint32_t group) { throw Error(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setDPPAlgorithmControl(uint32_t group, uint32_t value) { throw Error(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setDPPAlgorithmControl(uint32_t value) { throw Error(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getDPPShapedTriggerWidth(uint32_t group) { throw Error(CAEN_DGTZ_FunctionNotAllowed); }
        virtual uint32_t getDPPShapedTriggerWidth() { throw Error(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setDPPShapedTriggerWidth(uint32_t group, uint32_t value) { throw Error(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setDPPShapedTriggerWidth(uint32_t value) { throw Error(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getBoardConfiguration() { throw Error(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setBoardConfiguration(uint32_t value) { throw Error(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void unsetBoardConfiguration(uint32_t value) { throw Error(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getDPPAggregateOrganization() { throw Error(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setDPPAggregateOrganization(uint32_t value) { throw Error(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getAcquisitionControl() { throw Error(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setAcquisitionControl(uint32_t value) { throw Error(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getAcquisitionStatus() { throw Error(CAEN_DGTZ_FunctionNotAllowed); }
        virtual uint32_t getDPPAcquisitionStatus() { throw Error(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getGlobalTriggerMask() { throw Error(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setGlobalTriggerMask(uint32_t value) { throw Error(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getFrontPanelTRGOUTEnableMask() { throw Error(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setFrontPanelTRGOUTEnableMask(uint32_t value) { throw Error(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getFrontPanelIOControl() { throw Error(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setFrontPanelIOControl(uint32_t value) { throw Error(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getROCFPGAFirmwareRevision() { throw Error(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getEventSize() { throw Error(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getFanSpeedControl() { throw Error(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setFanSpeedControl(uint32_t value) { throw Error(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getDPPDisableExternalTrigger() { throw Error(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setDPPDisableExternalTrigger(uint32_t value) { throw Error(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getRunStartStopDelay() { throw Error(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setRunStartStopDelay(uint32_t value) { throw Error(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getReadoutControl() { throw Error(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setReadoutControl(uint32_t value) { throw Error(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getReadoutStatus() { throw Error(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getScratch() { throw Error(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setScratch(uint32_t value) { throw Error(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getDPPAggregateNumberPerBLT() { throw Error(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setDPPAggregateNumberPerBLT(uint32_t value) { throw Error(CAEN_DGTZ_FunctionNotAllowed); }

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
        class BoardConfiguration
        {
        private:
            uint32_t v;
        public:
            BoardConfiguration(uint32_t value): v(value) {}
            uint32_t value() {return v;}
            bool triggerOverlap() {return (v&(1<<1)) == (1<<1);}
            bool testPattern() {return (v&(1<<3)) == (1<<3);}
            bool polarity() {return (v&(1<<6)) == (1<<6);}
        };
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
        class BoardConfiguration
        {
        private:
            uint32_t v;
        public:
            BoardConfiguration(uint32_t value): v(value) {}
            uint32_t value() {return v;}
            bool waveform() {return (v&(1<<16)) == (1<<16);}
            bool extras() {return (v&(1<<17)) == (1<<17);}
            bool timestamp() {return (v&(1<<18)) == (1<<18);}
            bool charge() {return (v&(1<<19)) == (1<<19);}
        };

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
        uint32_t getDPPPreTriggerSize(uint32_t group) override
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
        void setDPPPreTriggerSize(uint32_t group, uint32_t samples) override
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


        /* TODO: wrap Individual Trigger Threshold of Group n Sub Channel m from register docs? */

        /* NOTE: reuse get / set BoardConfiguration from parent */
        /* NOTE: disable inherited 740 EasyBoardConfiguration since we only
         * support EasyDPPBoardConfiguration here. */

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


        /* NOTE: Reuse get / set SoftwareTrigger from parent? */

        /* NOTE: Reuse get / set GlobalTriggerMask from parent */

        /* NOTE: reuse get / set FrontPanelTRGOUTEnableMask from parent */

        /* TODO: disable Post Trigger here if implemented in parent */

        /* NOTE: reuse get / set FrontPanelIOControl from parent */

        /* NOTE: ROCFPGAFirmwareRevision is inherited from Digitizer740  */

        /* TODO: wrap Voltage Level Mode Configuration from register docs? */

        /* NOTE: Analog Monitor Mode from DPP register docs looks equal
         * to Monitor DAC Mode from generic model */

        /* TODO: wrap Time Bomb Downcounter from register docs? */

        /* NOTE: reuse get / set FanSpeedControl from parent */

        /* NOTE: reuse get ReadoutControl from parent */

        /* NOTE: reuse get ReadoutStatus from parent */

        /* NOTE: reuse get Scratch from parent */

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

        uint32_t getRecordLength() override
        {
            uint32_t size; errorHandler(CAEN_DGTZ_ReadRegister(handle_, 0x8024, &size)); return size<<3;
        }
        uint32_t getRecordLength(uint32_t group) override
        {
            if (group > groups())
                throw Error(CAEN_DGTZ_InvalidChannelNumber);
            uint32_t size;
            errorHandler(CAEN_DGTZ_ReadRegister(handle_, 0x1024 | group<<8, &size));
            return size<<3;
        }
        void setRecordLength(uint32_t size) override
        {
            errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x8024, size>>3));
        }
        void setRecordLength(uint32_t group, uint32_t size) override
        {
            if (group > groups())
                throw Error(CAEN_DGTZ_InvalidChannelNumber);
            errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x1024 | group<<8, size));
        }


    };


    class Digitizer751 : public Digitizer{
    private:
        Digitizer751();
        friend Digitizer* Digitizer::open(CAEN_DGTZ_ConnectionType linkType, int linkNum, int conetNode, uint32_t VMEBaseAddress);

    protected:
        Digitizer751(int handle, CAEN_DGTZ_BoardInfo_t boardInfo) : Digitizer(handle,boardInfo) {}

    public:
        class BoardConfiguration
        {
        private:
            uint32_t v;
        public:
            BoardConfiguration(uint32_t value): v(value) {}
            uint32_t value() {return v;}
            bool triggerOverlap() {return (v&(1<<1)) == (1<<1);}
            bool testPattern() {return (v&(1<<3)) == (1<<3);}
            bool polarity() {return (v&(1<<6)) == (1<<6);}
            bool enableDes() {return (v&(1<<12)) == (1<<12);}
        };

        /* NOTE: BoardConfiguration differs in forced ones and zeros
         * between generic and DPP version. Use a class-specific mask.
         */
        /* According to register docs the bits [0,2,5] must be 0 and the bits
         * [4] must be 1 while [7:11,13:31] are reserved so we always force
         * compliance by a bitwise-or with 0x00000010 followed by a bitwise-and
         * with 0x0000105A for the set operation. Similarly we prevent mangling
         * of the force ones by a bitwise-and with the xor inverted version of
         * 0x00000010 for the unset operation.
         */
        virtual uint32_t filterBoardConfigurationSetMask(uint32_t mask) override
        { return ((mask | 0x00000010) & 0x0000105A); }
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

        /* NOTE: Get / Set DC Offset is handled in GroupDCOffset */

        /* NOTE: Get / Set Channel Enable Mask of Group is handled in
         * ChannelGroupMask */

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

      // TODO: many register-level functions are missing in the 751 implementation; they can likely be easily transferred from the 740 class if needed
    };

} // namespace caen
#endif //_CAEN_HPP
