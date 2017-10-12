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

        virtual uint32_t getFixedBaseline(uint32_t group) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setFixedBaseline(uint32_t group, uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }
        virtual void setFixedBaseline(uint32_t value) { errorHandler(CAEN_DGTZ_FunctionNotAllowed); }

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
         * to compensate for the delay in the propaga on of the Start (or Stop) signal through the chain. This register
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
    };

} // namespace caen
#endif //_CAEN_HPP
