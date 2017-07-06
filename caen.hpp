//
// Created by troels on 6/14/17.
//

#ifndef _CAEN_HPP
#define _CAEN_HPP

#include <CAENDigitizer.h>
#include <string>
#include <cstdint>

namespace caen {
    class Error : public std::exception
    {
    private:
        CAEN_DGTZ_ErrorCode code_;
    public:
#define __ERR_TO_STR(X) case (X) : return (#X);
        static const char* digitizerErrorString(CAEN_DGTZ_ErrorCode code)
        {
            switch (code) {
                __ERR_TO_STR(CAEN_DGTZ_Success)
                __ERR_TO_STR(CAEN_DGTZ_CommError)
                __ERR_TO_STR(CAEN_DGTZ_GenericError)
                __ERR_TO_STR(CAEN_DGTZ_InvalidParam)
                __ERR_TO_STR(CAEN_DGTZ_InvalidLinkType)
                __ERR_TO_STR(CAEN_DGTZ_InvalidHandle)
                __ERR_TO_STR(CAEN_DGTZ_MaxDevicesError)
                __ERR_TO_STR(CAEN_DGTZ_BadBoardType)
                __ERR_TO_STR(CAEN_DGTZ_BadInterruptLev)
                __ERR_TO_STR(CAEN_DGTZ_BadEventNumber)
                __ERR_TO_STR(CAEN_DGTZ_ReadDeviceRegisterFail)
                __ERR_TO_STR(CAEN_DGTZ_WriteDeviceRegisterFail)
                __ERR_TO_STR(CAEN_DGTZ_InvalidChannelNumber)
                __ERR_TO_STR(CAEN_DGTZ_ChannelBusy)
                __ERR_TO_STR(CAEN_DGTZ_FPIOModeInvalid)
                __ERR_TO_STR(CAEN_DGTZ_WrongAcqMode)
                __ERR_TO_STR(CAEN_DGTZ_FunctionNotAllowed)
                __ERR_TO_STR(CAEN_DGTZ_Timeout)
                __ERR_TO_STR(CAEN_DGTZ_InvalidBuffer)
                __ERR_TO_STR(CAEN_DGTZ_EventNotFound)
                __ERR_TO_STR(CAEN_DGTZ_InvalidEvent)
                __ERR_TO_STR(CAEN_DGTZ_OutOfMemory)
                __ERR_TO_STR(CAEN_DGTZ_CalibrationError)
                __ERR_TO_STR(CAEN_DGTZ_DigitizerNotFound)
                __ERR_TO_STR(CAEN_DGTZ_DigitizerAlreadyOpen)
                __ERR_TO_STR(CAEN_DGTZ_DigitizerNotReady)
                __ERR_TO_STR(CAEN_DGTZ_InterruptNotConfigured)
                __ERR_TO_STR(CAEN_DGTZ_DigitizerMemoryCorrupted)
                __ERR_TO_STR(CAEN_DGTZ_DPPFirmwareNotSupported)
                __ERR_TO_STR(CAEN_DGTZ_InvalidLicense)
                __ERR_TO_STR(CAEN_DGTZ_InvalidDigitizerStatus)
                __ERR_TO_STR(CAEN_DGTZ_UnsupportedTrace)
                __ERR_TO_STR(CAEN_DGTZ_InvalidProbe)
                __ERR_TO_STR(CAEN_DGTZ_NotYetImplemented)
                default : return "Unknown Error";
            }
        }
        Error(CAEN_DGTZ_ErrorCode code) : code_(code) {}
        virtual const char * what() const noexcept
        {
            return digitizerErrorString(code_);
        }

        int code() const { return code_; }
    };
    static inline void errorHandler(CAEN_DGTZ_ErrorCode code)
    {
        if (code != CAEN_DGTZ_Success) {
            throw Error(code);
        }
    }
    struct ReadoutBuffer { char* data; uint32_t size; };
    struct EventInfo : CAEN_DGTZ_EventInfo_t {char* data;};
    class Digitizer
    {
    private:
        int handle_;
    public:
        class BoardInfo : public CAEN_DGTZ_BoardInfo_t
        {
        public:
            const std::string modelName() const {return std::string(ModelName);}
            uint32_t modelNo() const {return Model; }
            uint32_t channels() const { return Channels; }
            uint32_t formFactor() const { return  FormFactor; }
            uint32_t familyCode() const { return FamilyCode; }
            const std::string ROCfirmwareRel() const { return std::string(ROC_FirmwareRel); }
            const std::string AMCfirmwareRel() const { return std::string(AMC_FirmwareRel); }
            uint32_t serialNumber() const { return SerialNumber; }
            uint32_t PCBrevision() const { return PCB_Revision; }
            uint32_t ADCbits() const { return ADC_NBits; }
            int commHandle() const { return CommHandle; }
            int VMEhandle() const { return VMEHandle; }
            const std::string license() const { return std::string(License); }
        };
        Digitizer() : handle_(-1) {}
        Digitizer(int handle) : handle_(handle) {}

        Digitizer(CAEN_DGTZ_ConnectionType linkType, int linkNum, int conetNode, uint32_t VMEBaseAddress)
        { errorHandler(CAEN_DGTZ_OpenDigitizer(linkType,linkNum,conetNode,VMEBaseAddress,&handle_)); }

        ~Digitizer() { if (handle_ >= 0) errorHandler(CAEN_DGTZ_CloseDigitizer(handle_)); }

        int handle() { return handle_; }

        void open(CAEN_DGTZ_ConnectionType linkType, int linkNum, int conetNode, uint32_t VMEBaseAddress)
        { errorHandler(CAEN_DGTZ_OpenDigitizer(linkType,linkNum,conetNode,VMEBaseAddress,&handle_)); }

        void openUSB(int linkNum)
        { errorHandler(CAEN_DGTZ_OpenDigitizer(CAEN_DGTZ_USB,linkNum,0,0,&handle_)); }

        void close()
        { errorHandler(CAEN_DGTZ_CloseDigitizer(handle_)); }

        void writeRegister(uint32_t address, uint32_t value)
        { errorHandler(CAEN_DGTZ_WriteRegister(handle_, address, value)); }

        uint32_t readRegister(uint32_t address)
        { uint32_t value; errorHandler(CAEN_DGTZ_ReadRegister(handle_, address, &value)); return value; }

        void reset() { errorHandler(CAEN_DGTZ_Reset(handle_)); }

        BoardInfo getInfo() {BoardInfo bi; errorHandler(CAEN_DGTZ_GetInfo(handle_,&bi)); return bi;}

        void calibrate()
        { errorHandler(CAEN_DGTZ_Calibrate(handle_)); }

        uint32_t readTemperature(int32_t ch)
        {uint32_t temp; errorHandler(CAEN_DGTZ_ReadTemperature(handle_, ch, &temp)); return temp;}

        uint32_t getRecordLength()
        { uint32_t size; errorHandler(CAEN_DGTZ_GetRecordLength(handle_,&size)); return size; }

        void setRecordLength(uint32_t size)
        { errorHandler(CAEN_DGTZ_SetRecordLength(handle_,size)); }

        void clearData()
        { errorHandler(CAEN_DGTZ_ClearData(handle_)); }

        void disableEventAlignedReadout()
        { errorHandler(CAEN_DGTZ_DisableEventAlignedReadout(handle_)); }

        uint32_t getMaxNumEventsBLT()
        { uint32_t n; errorHandler(CAEN_DGTZ_GetMaxNumEventsBLT(handle_, &n)); return n; }

        void setMaxNumEventsBLT(uint32_t n)
        { errorHandler(CAEN_DGTZ_SetMaxNumEventsBLT(handle_, n)); }

        void sendSWtrigger()
        { errorHandler(CAEN_DGTZ_SendSWtrigger(handle_)); }

        uint32_t getChannelEnableMask()
        { uint32_t mask; errorHandler(CAEN_DGTZ_GetChannelEnableMask(handle_, &mask)); return mask;}

        void setChannelEnableMask(uint32_t mask)
        { errorHandler(CAEN_DGTZ_SetChannelEnableMask(handle_, mask)); }

        uint32_t getGroupEnableMask()
        { uint32_t mask; errorHandler(CAEN_DGTZ_GetGroupEnableMask(handle_, &mask)); return mask;}

        void setGroupEnableMask(uint32_t mask)
        { errorHandler(CAEN_DGTZ_SetGroupEnableMask(handle_, mask)); }

        ReadoutBuffer mallocReadoutBuffer()
        { ReadoutBuffer b; errorHandler(CAEN_DGTZ_MallocReadoutBuffer(handle_, &b.data, &b.size)); return b; }

        void freeReadoutBuffer(ReadoutBuffer b)
        { errorHandler(CAEN_DGTZ_FreeReadoutBuffer(&b.data)); }

        ReadoutBuffer readData(ReadoutBuffer buffer,CAEN_DGTZ_ReadMode_t mode)
        { errorHandler(CAEN_DGTZ_ReadData(handle_, mode, buffer.data, &buffer.size)); return buffer; }

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

        uint32_t getGroupDCOffset(uint32_t group)
        { uint32_t offset; errorHandler(CAEN_DGTZ_GetGroupDCOffset(handle_, group, &offset)); return offset; }

        void setGroupDCOffset(uint32_t group, uint32_t offset)
        { errorHandler(CAEN_DGTZ_SetGroupDCOffset(handle_, group, offset)); }

        CAEN_DGTZ_TriggerMode_t getGroupSelfTrigger(uint32_t group)
        { CAEN_DGTZ_TriggerMode_t mode; errorHandler(CAEN_DGTZ_GetGroupSelfTrigger(handle_, group, &mode)); return mode; }

        void setGroupSelfTrigger(uint32_t group, CAEN_DGTZ_TriggerMode_t mode)
        { errorHandler(CAEN_DGTZ_SetGroupSelfTrigger(handle_, mode, 1<<group)); }

        uint32_t getGroupTriggerThreshold(uint32_t group)
        { uint32_t treshold; errorHandler(CAEN_DGTZ_GetGroupTriggerThreshold(handle_, group, &treshold)); return treshold; }

        void setGroupTriggerThreshold(uint32_t group, uint32_t treshold)
        { errorHandler(CAEN_DGTZ_SetGroupTriggerThreshold(handle_, group, treshold)); }

        uint32_t getChannelGroupMask(uint32_t group)
        { uint32_t mask; errorHandler(CAEN_DGTZ_GetChannelGroupMask(handle_, group, &mask)); return mask; }

        void setChannelGroupMask(uint32_t group, uint32_t mask)
        { errorHandler(CAEN_DGTZ_SetChannelGroupMask(handle_, group, mask)); }

        CAEN_DGTZ_TriggerPolarity_t getTriggerPolarity(uint32_t channel)
        { CAEN_DGTZ_TriggerPolarity_t polarity; errorHandler(CAEN_DGTZ_GetTriggerPolarity(handle_, channel, &polarity)); return polarity; }

        void setTriggerPolarity(uint32_t channel, CAEN_DGTZ_TriggerPolarity_t polarity)
        { errorHandler(CAEN_DGTZ_SetTriggerPolarity(handle_, channel, polarity)); }

        void startAcquisition()
        { errorHandler(CAEN_DGTZ_SWStartAcquisition(handle_)); }

        void stopAcquisition()
        { errorHandler(CAEN_DGTZ_SWStopAcquisition(handle_)); }

        uint32_t getNumEvents(ReadoutBuffer buffer)
        {uint32_t n; errorHandler(CAEN_DGTZ_GetNumEvents(handle_, buffer.data, buffer.size, &n)); return n; }

        EventInfo getEventInfo(ReadoutBuffer buffer, int32_t n)
        { EventInfo info; errorHandler(CAEN_DGTZ_GetEventInfo(handle_, buffer.data, buffer.size, n, &info, &info.data)); return info; }

        CAEN_DGTZ_UINT16_EVENT_t* allocateEvent()
        { CAEN_DGTZ_UINT16_EVENT_t* event; errorHandler(CAEN_DGTZ_AllocateEvent(handle_, (void**)&event)); return event; }

        void freeEvent(CAEN_DGTZ_UINT16_EVENT_t* event)
        { errorHandler(CAEN_DGTZ_FreeEvent(handle_, (void**)&event)); }

        CAEN_DGTZ_UINT16_EVENT_t* decodeEvent(EventInfo info, CAEN_DGTZ_UINT16_EVENT_t* event)
        { errorHandler(CAEN_DGTZ_DecodeEvent(handle_, info.data, (void**)&event)); return event; }



    };



} // namespace caen
#endif //_CAEN_HPP
