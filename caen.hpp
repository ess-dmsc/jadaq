/*
 * jadaq (Just Another DAQ)
 * Copyright (C) 2017  Troels Blum <troels@blum.dk>
 *
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
 */

#ifndef _CAEN_HPP
#define _CAEN_HPP

#include "_CAENDigitizer.h"
#include <CAENDigitizerType.h>
#include <string>
#include <cstdint>
#include <cassert>

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

    struct ReadoutBuffer {
        char* data;
        uint32_t size;
        uint32_t dataSize;
    };

    struct InterruptConfig
    {
        CAEN_DGTZ_EnaDis_t state;
        uint8_t level;
        uint32_t status_id;
        uint16_t event_number;
        CAEN_DGTZ_IRQMode_t mode;
    };

    struct ZSParams {
        CAEN_DGTZ_ThresholdWeight_t weight; //enum
        int32_t threshold;
        int32_t nsamp;
    };

    struct AIMParams {
        uint32_t channelmask;
        uint32_t offset;
        CAEN_DGTZ_AnalogMonitorMagnify_t mf; //enum
        CAEN_DGTZ_AnalogMonitorInspectorInverter_t ami; //enum
    };

    struct DPPAcquisitionMode {
        CAEN_DGTZ_DPP_AcqMode_t mode;    // enum
        CAEN_DGTZ_DPP_SaveParam_t param; // enum
    };

    /* TODO: should this be changed to pointers like in DPPEvents?
     * if so we should use DPP_SupportedVirtualProbes& supported as
     * argument in getDPP_SupportedVirtualProbes rather than explicit
     * init there.
     */
    struct DPP_SupportedVirtualProbes {
        int probes[MAX_SUPPORTED_PROBES];
        int numProbes;
    };

    struct EventInfo : CAEN_DGTZ_EventInfo_t {char* data;};

    struct DPPEvents
    {
        void** ptr;             // CAEN_DGTZ_DPP_XXX_Event_t* ptr[MAX_DPP_XXX_CHANNEL_SIZE]
        uint32_t* nEvents;      // Number of events pr channel
        uint32_t allocatedSize;
    };

    struct DPPWaveforms
    {
        void* ptr;
        uint32_t allocatedSize;
    };

    struct ChannelPairTriggerLogicParams {
        CAEN_DGTZ_TrigerLogic_t logic;
        uint16_t coincidenceWindow;
    };

    struct TriggerLogicParams {
        CAEN_DGTZ_TrigerLogic_t logic;
        uint32_t majorityLevel;
    };

    struct SAMTriggerCountVetoParams {
        CAEN_DGTZ_EnaDis_t enable;
        uint32_t vetoWindow;
    };

    /* For user-friendly configuration of DPP Algorithm Control mask */
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

    /* For user-friendly configuration of Board Configuration mask */
    struct EasyBoardConfiguration {
        /* Only allow the expected number of bits per field */
        uint8_t individualTrigger : 1;
        uint8_t analogProbe : 2;
        uint8_t waveformRecording : 1;
        uint8_t extrasRecording : 1;
        uint8_t timeStampRecording : 1;
        uint8_t chargeRecording : 1;
        uint8_t externalTriggerMode : 2;
    };

    /* For user-friendly configuration of Acquisition Control mask */
    struct EasyAcquisitionControl {
        /* Only allow the expected number of bits per field */
        uint8_t startStopMode : 2;
        uint8_t acquisitionStartArm : 1;
        uint8_t triggerCountingMode : 1;
        uint8_t pLLRefererenceClock : 1;
        uint8_t lVDSIOBusyEnable : 1;
        uint8_t lVDSVetoEnable : 1;
        uint8_t lVDSIORunInEnable : 1;
    };

    /* For user-friendly configuration of Acquisition Control mask */
    struct EasyAcquisitionStatus {
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


    /* Bit-banging helpers to translate between EasyX struct and bitmask.
     * Please refer to the register docs for the individual mask
     * layouts.
     */

    static uint8_t unpackBits(uint32_t mask, uint8_t size, uint8_t offset)
    { return (uint8_t)((mask >> offset) & size); }
    static uint32_t packBits(uint8_t value, uint8_t size, uint8_t offset)
    { return (value & size) << offset; }

    /* EasyDPPAlgorithmControl fields:
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

    /* EasyBoardConfiguration fields:
     * individual trigger in [8], analog probe in [12:13],
     * waveform recording in [16], extras recording in [17],
     * time stamp recording in [18], charge recording in [19],
     * external trigger mode in [20:21].
     */
    static uint32_t ebc2bits(EasyBoardConfiguration settings)
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
    static EasyBoardConfiguration bits2ebc(uint32_t mask)
    {
        EasyBoardConfiguration settings;
        settings = {unpackBits(mask, 1, 8), unpackBits(mask, 2, 12),
                    unpackBits(mask, 1, 16), unpackBits(mask, 1, 17),
                    unpackBits(mask, 1, 18), unpackBits(mask, 1, 19),
                    unpackBits(mask, 2, 20)};
        return settings;
    }

    /* EasyAcquisitionControl fields:
     * start/stop mode [0:1], acquisition start/arm in [2],
     * trigger counting mode in [3], PLL reference clock
     * source in [6], LVDS I/O busy enable in [8], 
     * LVDS veto enable in [9], LVDS I/O RunIn enable in [11].
     */
    static uint32_t eac2bits(EasyAcquisitionControl settings)
    {
        uint32_t mask = 0;
        mask |= packBits(settings.startStopMode, 2, 0);
        mask |= packBits(settings.acquisitionStartArm, 1, 2);
        mask |= packBits(settings.triggerCountingMode, 1, 3);
        mask |= packBits(settings.pLLRefererenceClock, 1, 6);
        mask |= packBits(settings.lVDSIOBusyEnable, 1, 8);
        mask |= packBits(settings.lVDSVetoEnable, 1, 9);
        mask |= packBits(settings.lVDSIORunInEnable, 1, 11);
        return mask;
    }
    static EasyAcquisitionControl bits2eac(uint32_t mask)
    {
        EasyAcquisitionControl settings;
        settings = {unpackBits(mask, 2, 0), unpackBits(mask, 1, 2),
                    unpackBits(mask, 1, 3), unpackBits(mask, 1, 6),
                    unpackBits(mask, 1, 8), unpackBits(mask, 1, 9),
                    unpackBits(mask, 1, 11)};
        return settings;
    }

    /* EasyAcquisitionStatus fields:
     * acquisition status [2], event ready [3], event full in [4],
     * clock source in [5], PLL unlock detect in [7], board ready in [8],
     * S-In in [15], TRG-IN in [16].
     */
    static uint32_t eas2bits(EasyAcquisitionStatus settings)
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
    static EasyAcquisitionStatus bits2eas(uint32_t mask)
    {
        EasyAcquisitionStatus settings;
        settings = {unpackBits(mask, 1, 2), unpackBits(mask, 1, 3),
                    unpackBits(mask, 1, 4), unpackBits(mask, 1, 5),
                    unpackBits(mask, 1, 7), unpackBits(mask, 1, 8),
                    unpackBits(mask, 1, 15), unpackBits(mask, 1, 16)};
        return settings;
    }


    /* Some low-level digitizer helpers */
    static int openRawDigitizer(CAEN_DGTZ_ConnectionType linkType, int linkNum, int conetNode, uint32_t VMEBaseAddress) 
    {
        int handle;
        errorHandler(CAEN_DGTZ_OpenDigitizer(linkType, linkNum, conetNode, VMEBaseAddress, &handle));
        return handle;
    }
    static CAEN_DGTZ_BoardInfo_t getRawDigitizerBoardInfo(int handle) 
    {
        CAEN_DGTZ_BoardInfo_t boardInfo;
        errorHandler(CAEN_DGTZ_GetInfo(handle, &boardInfo));
        return boardInfo;
    }
    static CAEN_DGTZ_DPPFirmware_t getRawDigitizerDPPFirmware(int handle) 
    {
        CAEN_DGTZ_DPPFirmware_t firmware;
        errorHandler(_CAEN_DGTZ_GetDPPFirmwareType(handle, &firmware));
        return firmware;
    }
    static void closeRawDigitizer(int handle) 
    {
        errorHandler(CAEN_DGTZ_CloseDigitizer(handle));
    }
    
    
    class Digitizer
    {
    private:
        Digitizer() {}
        Digitizer(int handle) : handle_(handle) { boardInfo_ = getRawDigitizerBoardInfo(handle_); }

    protected:
        int handle_;
        CAEN_DGTZ_BoardInfo_t boardInfo_;
        Digitizer(int handle, CAEN_DGTZ_BoardInfo_t boardInfo) : handle_(handle) { boardInfo_ = boardInfo; }

    public:
        public:

        /* Digitizer creation */
        static Digitizer* open(CAEN_DGTZ_ConnectionType linkType, int linkNum, int conetNode, uint32_t VMEBaseAddress);
        static Digitizer* USB(int linkNum) { return open(CAEN_DGTZ_USB,linkNum,0,0); }
        static Digitizer* USB(int linkNum, uint32_t VMEBaseAddress) { return open(CAEN_DGTZ_USB,linkNum,0,VMEBaseAddress);}

        /* Destruction */
        static void close(int handle) { closeRawDigitizer(handle); }
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
         */
        EventInfo getEventInfo(ReadoutBuffer buffer, int32_t n)
        { EventInfo info; errorHandler(CAEN_DGTZ_GetEventInfo(handle_, buffer.data, buffer.dataSize, n, &info, &info.data)); return info; }

        void* decodeEvent(EventInfo info, void* event)
        { errorHandler(CAEN_DGTZ_DecodeEvent(handle_, info.data, &event)); return event; }

        DPPEvents& getDPPEvents(ReadoutBuffer buffer, DPPEvents& events)
        { _CAEN_DGTZ_GetDPPEvents(handle_, buffer.data, buffer.dataSize, events.ptr, events.nEvents); return events; }

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

        /* TODO: CAEN_DGTZ_GetDecimationFactor fails with GenericError
         * on DT5740_171 and V1740D_137, apparently a mismatch between
         * DigitizerTable value end value read from register in the
         * V1740 specific case. */
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

        /* TODO: CAEN_DGTZ_GetGroupDCOffset fails with GenericError on
         * V1740D_137, something fails in the V1740 specific case. */
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

        /* TODO: CAEN_DGTZ_GetGroupTriggerThreshold fails with ReadDeviceRegisterFail
         * on V1740D_137. */
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

        /* TODO: fails with InvalidParam om DT5740_171 and V1740D_137, seems to fail
         * deep in readout when the digtizer library calls ReadRegister 0x1n80 */
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

        virtual uint32_t getRunDelay() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setRunDelay(uint32_t delay) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getGateWidth(uint32_t group) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setGateWidth(uint32_t group, uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setGateWidth(uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getGateOffset(uint32_t group) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setGateOffset(uint32_t group, uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setGateOffset(uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getFixedBaseline(uint32_t group) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setFixedBaseline(uint32_t group, uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setFixedBaseline(uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getTriggerHoldOffWidth(uint32_t group) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setTriggerHoldOffWidth(uint32_t group, uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setTriggerHoldOffWidth(uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getDPPAlgorithmControl(uint32_t group) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setDPPAlgorithmControl(uint32_t group, uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setDPPAlgorithmControl(uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyDPPAlgorithmControl getEasyDPPAlgorithmControl(uint32_t group) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setEasyDPPAlgorithmControl(uint32_t group, EasyDPPAlgorithmControl settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setEasyDPPAlgorithmControl(EasyDPPAlgorithmControl settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getShapedTriggerWidth(uint32_t group) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setShapedTriggerWidth(uint32_t group, uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setShapedTriggerWidth(uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getBoardConfiguration() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setBoardConfiguration(uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void unsetBoardConfiguration(uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyBoardConfiguration getEasyBoardConfiguration() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setEasyBoardConfiguration(EasyBoardConfiguration settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void unsetEasyBoardConfiguration(EasyBoardConfiguration settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getAggregateOrganization() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setAggregateOrganization(uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getAcquisitionControl() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setAcquisitionControl(uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyAcquisitionControl getEasyAcquisitionControl() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setEasyAcquisitionControl(EasyAcquisitionControl settings) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getAcquisitionStatus() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual EasyAcquisitionStatus getEasyAcquisitionStatus() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getGlobalTriggerMask() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setGlobalTriggerMask(uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getFrontPanelTRGOUTEnableMask() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setFrontPanelTRGOUTEnableMask(uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getFrontPanelIOControl() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setFrontPanelIOControl(uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getROCFPGAFirmwareRevision() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getFanSpeedControl() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setFanSpeedControl(uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getDisableExternalTrigger() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setDisableExternalTrigger(uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getReadoutControl() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setReadoutControl(uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

        virtual uint32_t getAggregateNumberPerBLT() { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setAggregateNumberPerBLT(uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

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
        virtual uint32_t channelsPerGroup() const override { return 8; }  // 8 channels per group for x740

        /* Get / Set RunDelay
         * @delay: delay in units of 8 ns
         *
         * When the start of Run is given synchronously to several boards connected in Daisy chain, it is necessary
         * to compensate for the delay in the propagation of the Start (or Stop) signal through the chain. This register
         * sets the delay, expressed in trigger clock cycles between the arrival of the Start signal at the input of
         * the board (either on S-IN/GPI or TRG-IN) and the actual start of Run. The delay is usually zero for the
         * last board in the chain and rises going backwards along the chain.
         */
        uint32_t getRunDelay() override
        { uint32_t delay; errorHandler(CAEN_DGTZ_ReadRegister(handle_, 0x8170, &delay)); return delay; }
        virtual void setRunDelay(uint32_t delay) override
        { errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x8170, delay)); }

    };

    class Digitizer740DPP : public Digitizer740 {
    private:
        Digitizer740DPP();
        friend Digitizer *
        Digitizer::open(CAEN_DGTZ_ConnectionType linkType, int linkNum, int conetNode, uint32_t VMEBaseAddress);

    protected:
        Digitizer740DPP(int handle, CAEN_DGTZ_BoardInfo_t boardInfo) : Digitizer740(handle, boardInfo) {}

    public:
        /* Get / Set GateWidth
         * @group
         * @value: Number of samples for the Gate width. Each sample corresponds to 16 ns - 12 bits
         */
        uint32_t getGateWidth(uint32_t group) override
        {
            if (group >= groups())
                errorHandler(CAEN_DGTZ_InvalidChannelNumber);
            uint32_t value;
            errorHandler(CAEN_DGTZ_ReadRegister(handle_, 0x1030 | group<<8 , &value));
            return value;
        }
        void setGateWidth(uint32_t group, uint32_t value) override
        {
            if (group >= groups())
                errorHandler(CAEN_DGTZ_InvalidChannelNumber);
            errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x1030 | group<<8, value & 0xFFF));
        }
        void setGateWidth(uint32_t value) override
        { errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x8030, value & 0xFFF)); }

        /* Get / Set GateOffset
         * @group
         * @value: Number of samples for the Gate Offset width. Each sample corresponds to 16 ns. - 12 bits
         */
        uint32_t getGateOffset(uint32_t group) override
        {
            if (group >= groups())
                errorHandler(CAEN_DGTZ_InvalidChannelNumber);
            uint32_t value;
            errorHandler(CAEN_DGTZ_ReadRegister(handle_, 0x1034 | group<<8 , &value));
            return value;
        }
        void setGateOffset(uint32_t group, uint32_t value) override
        {
            if (group >= groups())
                errorHandler(CAEN_DGTZ_InvalidChannelNumber);
            errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x1034 | group<<8, value & 0xFFF));
        }
        void setGateOffset(uint32_t value) override
        { errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x8034, value & 0xFFF)); }

        /* Get / Set FixedBaseline
         * @group
         * @value: Value of Fixed Baseline in LSB counts - 12 bits
         */
        uint32_t getFixedBaseline(uint32_t group) override
        {
            if (group >= groups())
                errorHandler(CAEN_DGTZ_InvalidChannelNumber);
            uint32_t value;
            errorHandler(CAEN_DGTZ_ReadRegister(handle_, 0x1038 | group<<8 , &value));
            return value;
        }
        void setFixedBaseline(uint32_t group, uint32_t value) override
        {
            if (group >= groups())
                errorHandler(CAEN_DGTZ_InvalidChannelNumber);
            errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x1038 | group<<8, value & 0xFFF));
        }
        void setFixedBaseline(uint32_t value) override
        { errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x8038, value & 0xFFF)); }

        /* TODO: switch DPPPreTrigger to use CAENDigitizer functions? */

        /* Get / Set DPPPreTrigger
         * @group
         * @samples: Number of samples Ns of the Pre Trigger width. The value is
         * expressed in steps of sampling frequency (16 ns).
         * NOTE: the Pre Trigger value must be greater than the Gate Offset value by at least 112 ns.
         *
         */
        uint32_t getDPPPreTriggerSize(int group) override
        {
            if (group >= groups() || group < 0)
                errorHandler(CAEN_DGTZ_InvalidChannelNumber);
            uint32_t samples;
            errorHandler(CAEN_DGTZ_ReadRegister(handle_, 0x103C | group<<8 , &samples));
            return samples;
        }
        void setDPPPreTriggerSize(uint32_t samples) override
        { errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x803C, samples & 0xFFF)); }

        void setDPPPreTriggerSize(int group, uint32_t samples) override
        {
            if (group >= groups() || group < 0)
                errorHandler(CAEN_DGTZ_InvalidChannelNumber);
            { errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x103C | group<<8, samples & 0xFFF)); }
        }

        /* Get / Set DPP Algorithm Control
         * @group
         * @mask: bitmask for a large number of settings. Please refer
         * to register docs for the mask details - 32 bits.
         */
        uint32_t getDPPAlgorithmControl(uint32_t group) override
        {
            if (group >= groups())
                errorHandler(CAEN_DGTZ_InvalidChannelNumber);
            uint32_t mask;
            errorHandler(CAEN_DGTZ_ReadRegister(handle_, 0x1040 | group<<8 , &mask));
            return mask;
        }
        void setDPPAlgorithmControl(uint32_t group, uint32_t mask) override
        {
            if (group >= groups())
                errorHandler(CAEN_DGTZ_InvalidChannelNumber);
            errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x1040 | group<<8, mask));
        }
        void setDPPAlgorithmControl(uint32_t mask) override
        { errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x8040, mask)); }

        EasyDPPAlgorithmControl getEasyDPPAlgorithmControl(uint32_t group) override
        {
            uint32_t mask = getDPPAlgorithmControl(group);
            return bits2edppac(mask);
        }
        void setEasyDPPAlgorithmControl(uint32_t group, EasyDPPAlgorithmControl settings) override
        {
            uint32_t mask = edppac2bits(settings);
            setDPPAlgorithmControl(group, mask);
        }
        void setEasyDPPAlgorithmControl(EasyDPPAlgorithmControl settings) override
        {
            uint32_t mask = edppac2bits(settings);
            setDPPAlgorithmControl(mask);
        }

        /* Get / Set TriggerHoldOffWidth
         * @group
         * @value: Set the Trigger Hold-Off width in steps of 16 ns - 16 bits
         */
        uint32_t getTriggerHoldOffWidth(uint32_t group) override
        {
            if (group >= groups())
                errorHandler(CAEN_DGTZ_InvalidChannelNumber);
            uint32_t value;
            errorHandler(CAEN_DGTZ_ReadRegister(handle_, 0x1074 | group<<8 , &value));
            return value;
        }
        void setTriggerHoldOffWidth(uint32_t group, uint32_t value) override
        {
            if (group >= groups())
                errorHandler(CAEN_DGTZ_InvalidChannelNumber);
            errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x1074 | group<<8, value & 0xFFFF));
        }
        void setTriggerHoldOffWidth(uint32_t value) override
        { errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x8074, value & 0xFFFF)); }

        /* TODO: on V1740D ShapedTriggerWidth causes CommError */

        /* Get / Set ShapedTriggerWidth
         * @group
         * @value: Set the number of samples for the Shaped Trigger width in trigger clock cycles (16 ns step) - 16 bits
         */
        uint32_t getShapedTriggerWidth(uint32_t group) override
        {
            if (group >= groups())
                errorHandler(CAEN_DGTZ_InvalidChannelNumber);
            uint32_t value;
            errorHandler(CAEN_DGTZ_ReadRegister(handle_, 0x1078 | group<<8 , &value));
            return value;
        }
        void setShapedTriggerWidth(uint32_t group, uint32_t value) override
        {
            if (group >= groups())
                errorHandler(CAEN_DGTZ_InvalidChannelNumber);
            errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x1078 | group<<8, value & 0xFFFF));
        }
        void setShapedTriggerWidth(uint32_t value) override
        { errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x8078, value & 0xFFFF)); }

        /* TODO: wrap AMC Firmware Revision from register docs? */

        /* NOTE: Get / Set DC Offset is handled in GroupDCOffset */

        /* NOTE: Get / Set Channel Enable Mask of Group is handled in
         * ChannelGroupMask */

        /* TODO: wrap Group n Low Channels DC Offset Individual Correction from register docs? */

        /* TODO: wrap Group n High Channels DC Offset Individual Correction from register docs? */

        /* TODO: wrap Individual Trigger Threshold of Group n Sub Channel m from register docs? */

        /* Get / Set Board Configuration
         * @mask: a bitmask covering a number of settings. Please refer
         * to register docs - 32 bits.
         *
         * According to register docs the bits [0:3,5:7,9:11,14:15,22:31]
         * must be 0 and the bits [4,8,18,19] must be 1 so we always
         * force compliance by a bitwise-or with 0x000C0110 followed
         * by a bitwise-and with 0x003F3110 for the set operation.
         * Similarly we prevent mangling by a bitwise-and with the
         * inverted combination 0x99800 for the unset operation.
         *
         * NOTE: Read mask from 0x8000, BitSet mask with 0x8004 and
         *       BitClear mask with 0x8008.
         */
        uint32_t getBoardConfiguration() override
        {
            uint32_t mask;
            errorHandler(CAEN_DGTZ_ReadRegister(handle_, 0x8000, &mask));
            return mask;
        }
        void setBoardConfiguration(uint32_t mask) override
        { errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x8004, (mask | 0x000C0110) & 0x003F3110)); }
        void unsetBoardConfiguration(uint32_t mask) override
        { errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x8008, mask & 0x099800)); }
        EasyBoardConfiguration getEasyBoardConfiguration() override
        {
            uint32_t mask;
            mask = getBoardConfiguration();
            return bits2ebc(mask);
        }
        void setEasyBoardConfiguration(EasyBoardConfiguration settings) override
        {
            /* NOTE: according to DPP register docs individualTrigger,
             * timeStampRecording and chargeRecording MUST all be 1 */
            assert(settings.individualTrigger == 1);
            assert(settings.timeStampRecording == 1);
            assert(settings.chargeRecording == 1);
            uint32_t mask = ebc2bits(settings);
            setBoardConfiguration(mask);
        }
        void unsetEasyBoardConfiguration(EasyBoardConfiguration settings) override
        {
            /* NOTE: according to docs individualTrigger, timeStampRecording
             * and chargeRecording MUST all be 1. Thus we do NOT allow
             * unset.
             */
            assert(settings.individualTrigger == 0);
            assert(settings.timeStampRecording == 0);
            assert(settings.chargeRecording == 0);
            uint32_t mask = ebc2bits(settings);
            unsetBoardConfiguration(mask);
        }

        /* Get / Set AggregateOrganization
         * @value: Aggregate Organization. Nb: the number of aggregates is
         * equal to N_aggr = 2^Nb . Please refer to register doc for values - 4 bits
         */
        uint32_t getAggregateOrganization() override
        {
            uint32_t value;
            errorHandler(CAEN_DGTZ_ReadRegister(handle_, 0x800C, &value));
            return value;
        }
        void setAggregateOrganization(uint32_t value) override
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

        /* NOTE: According to the CAEN DPP register docs the bits
         * [18:19] should always be 1, and in CAENDigitizer docs it
         * sounds like setDPPAcquisitionMode with the only valid modes
         * there (Mixed or List) should both set them accordingly, but
         * apparently it doesn't really happen on V1740D with DPP
         * firmware. Reported upstream.
         *
         * TODO: switch to the CAEN Get / SetDPPAcquisitionMode when fixed?
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

        /* Get / Set Acquisition Control
         * @mask: a bitmask covering a number of settings. Please refer
         * to register docs - 12 bits.
         */
        uint32_t getAcquisitionControl() override
        {
            uint32_t mask;
            errorHandler(CAEN_DGTZ_ReadRegister(handle_, 0x8100, &mask));
            return mask;
        }
        void setAcquisitionControl(uint32_t mask) override
        { errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x8100, mask & 0x0FFF)); }
        EasyAcquisitionControl getEasyAcquisitionControl() override
        {
            uint32_t mask;
            mask = getAcquisitionControl();
            return bits2eac(mask);
        }
        void setEasyAcquisitionControl(EasyAcquisitionControl settings) override
        {
            uint32_t mask = eac2bits(settings);
            setAcquisitionControl(mask);
        }

        /* Get Acquisition Status
         * Returns a bitmask covering a number of status fields. Please refer
         * to register docs - 32 bits.
         */
        uint32_t getAcquisitionStatus() override
        {
            uint32_t mask;
            errorHandler(CAEN_DGTZ_ReadRegister(handle_, 0x8104, &mask));
            return mask;
        }
        EasyAcquisitionStatus getEasyAcquisitionStatus() override
        {
            uint32_t mask;
            mask = getAcquisitionStatus();
            return bits2eas(mask);
        }

        /* TODO: wrap Software Trigger from register docs? */

        /* Get / Set Global Trigger Mask
         * @mask: a bitmask covering a number of settings. Please refer
         * to register docs - 32 bits.
         */
        /* TODO: wrap Global Trigger Mask in user-friendly struct? */
        uint32_t getGlobalTriggerMask() override
        {
            uint32_t mask;
            errorHandler(CAEN_DGTZ_ReadRegister(handle_, 0x810C, &mask));
            return mask;
        }
        void setGlobalTriggerMask(uint32_t mask) override
        { errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x810C, mask)); }

        /* Get / Set Front Panel TRG-OUT (GPO) Enable Mask
         * @mask: a bitmask covering a number of settings. Please refer
         * to register docs - 32 bits.
         */
        /* TODO: wrap FrontPanelTRGOUTEnableMask in user-friendly struct? */
        uint32_t getFrontPanelTRGOUTEnableMask() override
        {
            uint32_t mask;
            errorHandler(CAEN_DGTZ_ReadRegister(handle_, 0x8110, &mask));
            return mask;
        }
        void setFrontPanelTRGOUTEnableMask(uint32_t mask) override
        { errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x8110, mask)); }

        /* TODO: wrap LVDS I/O Data from register docs? */

        /* Get / Set Front Panel I/O Control mask
         * @mask: a bitmask covering a number of settings. Please refer
         * to register docs - 32 bits.
         */
        /* TODO: wrap FrontPanelIOControl in user-friendly struct */
        uint32_t getFrontPanelIOControl() override
        {
            uint32_t mask;
            errorHandler(CAEN_DGTZ_ReadRegister(handle_, 0x811C, &mask));
            return mask;
        }
        void setFrontPanelIOControl(uint32_t mask) override
        { errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x811C, mask)); }

        /* TODO: is Group Enable Mask from register docs equal to
         * GroupEnableMask? */

        /* Get ROC FPGA Firmware Revision
         * Returns a bitmask covering a number of status fields. Please refer
         * to register docs - 32 bits.
         */
        uint32_t getROCFPGAFirmwareRevision() override
        {
            uint32_t mask;
            errorHandler(CAEN_DGTZ_ReadRegister(handle_, 0x8124, &mask));
            return mask;
        }

        /* TODO: wrap Voltage Level Mode Configuration from register docs? */
        /* TODO: wrap Software Clock Sync from register docs? */

        /* TODO: is Board Info from register docs equal to GetInfo? */

        /* TODO: wrap Analog Monitor Mode from register docs? */

        /* TODO: is Event Size Info from register docs already covered
         * by existing Event function? */

        /* TODO: wrap Time Bomb Downcounter from register docs? */

        /* Get / Set Fan Speed Control
         * @mask: a bitmask covering a number of settings. Please refer
         * to register docs - 32 bits.
         */
        /* TODO: wrap FanSpeedControl in user-friendly struct */
        uint32_t getFanSpeedControl() override
        {
            uint32_t mask;
            errorHandler(CAEN_DGTZ_ReadRegister(handle_, 0x8168, &mask));
            return mask;
        }
        void setFanSpeedControl(uint32_t mask) override
        { errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x8168, mask)); }

        /* NOTE: Get / Set Run/Start/Stop Delay from register docs is
         * already covered by RunDelay. */

        /* TODO: wrap Board Failure Status from register docs? */

        /* Get / Set Disable External Trigger on TRG-IN connector
         * @value: a boolean to set external trigger state - 1 bit.
         */
        uint32_t getDisableExternalTrigger() override
        {
            uint32_t value;
            errorHandler(CAEN_DGTZ_ReadRegister(handle_, 0x817C, &value));
            return value;
        }
        void setDisableExternalTrigger(uint32_t value) override
        { errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0x817C, value & 0x1)); }

        /* TODO: wrap Front Panel LVDS I/O New Features from register docs? */

        /* Get / Set Readout Control
         * @mask: a bitmask covering a number of settings. Please refer
         * to register docs - 32 bits.
         */
        /* TODO: wrap ReadoutControl in user-friendly struct */
        uint32_t getReadoutControl() override
        {
            uint32_t mask;
            errorHandler(CAEN_DGTZ_ReadRegister(handle_, 0xEF00, &mask));
            return mask;
        }
        void setReadoutControl(uint32_t mask) override
        { errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0xEF00, mask)); }

        /* TODO: wrap Readout Status from register docs? */
        /* TODO: wrap Board ID from register docs? */

        /* TODO: wrap MCST Base Address and Control from register docs? */
        /* TODO: wrap Relocation Address from register docs? */
        /* TODO: wrap Interrupt Status/ID from register docs? */
        /* TODO: wrap Interrupt Event Number from register docs? */

        /* Get / Set Aggregate Number per BLT
         * @value: Number of complete aggregates to be transferred for
         * each block transfer (BLT) - 10 bits.
         */
        uint32_t getAggregateNumberPerBLT() override
        {
            uint32_t value;
            errorHandler(CAEN_DGTZ_ReadRegister(handle_, 0xEF1C, &value));
            return value;
        }
        void setAggregateNumberPerBLT(uint32_t value) override
        { errorHandler(CAEN_DGTZ_WriteRegister(handle_, 0xEF1C, value & 0x03FF)); }

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

} // namespace caen
#endif //_CAEN_HPP
