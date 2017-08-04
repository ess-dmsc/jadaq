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

#include <CAENDigitizer.h>
#include <string>
#include <cstdint>
#include <CAENDigitizerType.h>
#include <initializer_list>


namespace caen {

  /// helper routine to compress comparison of a variable against several values
  template <typename T>
  bool is_in(const T& val, const std::initializer_list<T>& list){
    for (const auto& i : list) {
      if (val == i) {
        return true;
      }
    }
    return false;
  }


  class Error : public std::exception
    {
    private:
        CAEN_DGTZ_ErrorCode code_;
      std::string where_;
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
      Error(CAEN_DGTZ_ErrorCode code, std::string where = "") : code_(code), where_(where) {}
        virtual const char * what() const noexcept
        {
            return digitizerErrorString(code_);
        }
      virtual const char * where() const noexcept
      {
        return where_.c_str();
      }

        int code() const { return code_; }
    };
  static inline void errorHandler(CAEN_DGTZ_ErrorCode code, std::string caller)
    {
        if (code != CAEN_DGTZ_Success) {
          throw Error(code, caller);
        }
    }
  // class to capture the caller and pass it to the error handling function
  // adopted from https://stackoverflow.com/a/378165
  class Reporter
  {
  public:
    Reporter(std::string Caller, std::string File, int Line)
      : caller_(Caller)
      , file_(File)
      , line_(Line)
    {}

    void operator()(CAEN_DGTZ_ErrorCode code)
    {
      // std::cout
      //   << "Reporter: errorHandler() is being called by "
      //   << caller_ << "() in " << file_ << ":" << line_ << std::endl;
      // can use the original name here, as it is still defined
      return errorHandler(code, caller_);
    }
  private:
    std::string   caller_;
    std::string   file_;
    int           line_;

  };

  // remove the symbol for the function, then define a new version that instead
  // creates a stack temporary instance of Reporter initialized with the caller
#  undef errorHandler
#  define errorHandler Reporter(__FUNCTION__,__FILE__,__LINE__)


    struct ReadoutBuffer { char* data; uint32_t size; };

    struct EventInfo : CAEN_DGTZ_EventInfo_t {char* data;};

    class Digitizer
    {
    private:
        Digitizer() {}
        Digitizer(int handle) : handle_(handle) { errorHandler(CAEN_DGTZ_GetInfo(handle,&boardInfo_)); }

    protected:
        int handle_;
        CAEN_DGTZ_BoardInfo_t boardInfo_;
        Digitizer(int handle, CAEN_DGTZ_BoardInfo_t boardInfo) : handle_(handle) { boardInfo_ = boardInfo; }

    public:
        public:
        const std::string modelName() const {return std::string(boardInfo_.ModelName);}
        uint32_t modelNo() const {return boardInfo_.Model; }
      virtual uint32_t channels() const { return boardInfo_.Channels; }
      virtual uint32_t groups() const { return 1; }
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

      bool hasDppFw(){return (std::atoi(boardInfo_.AMC_FirmwareRel) != STANDARD_FW_CODE);}
      bool isDppQdcFw(){return (std::atoi(boardInfo_.AMC_FirmwareRel) == 135);} // value from CAEN UM4868 - 740 DPP-QDC Registers User Manual rev. 2, page 2 (value not defined in CAENDigitizer lib up to v2.9.1)
      bool isDppCiFw(){return is_in(std::atoi(boardInfo_.AMC_FirmwareRel), {V1720_DPP_CI_CODE, V1743_DPP_CI_CODE});}
      bool is751Family(){return (boardInfo_.FamilyCode == CAEN_DGTZ_XX751_FAMILY_CODE);}
      bool is740Family(){return (boardInfo_.FamilyCode == CAEN_DGTZ_XX740_FAMILY_CODE);}

        static Digitizer* open(CAEN_DGTZ_ConnectionType linkType, int linkNum, int conetNode, uint32_t VMEBaseAddress);
        static Digitizer* USB(int linkNum) { return open(CAEN_DGTZ_USB,linkNum,0,0); }


        ~Digitizer() { errorHandler(CAEN_DGTZ_CloseDigitizer(handle_)); }

        int handle() { return handle_; }

        void writeRegister(uint32_t address, uint32_t value)
        { errorHandler(CAEN_DGTZ_WriteRegister(handle_, address, value)); }

        uint32_t readRegister(uint32_t address)
        { uint32_t value; errorHandler(CAEN_DGTZ_ReadRegister(handle_, address, &value)); return value; }

        void reset() { errorHandler(CAEN_DGTZ_Reset(handle_)); }

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

      uint32_t getChannelDCOffset(uint32_t channel)
      { uint32_t offset; errorHandler(CAEN_DGTZ_GetChannelDCOffset(handle_, channel, &offset)); return offset; }

      void setChannelDCOffset(uint32_t channel, uint32_t offset)
      { errorHandler(CAEN_DGTZ_SetChannelDCOffset(handle_, channel, offset)); }

        uint32_t getGroupDCOffset(uint32_t group)
        { uint32_t offset; errorHandler(CAEN_DGTZ_GetGroupDCOffset(handle_, group, &offset)); return offset; }

        void setGroupDCOffset(uint32_t group, uint32_t offset)
        { errorHandler(CAEN_DGTZ_SetGroupDCOffset(handle_, group, offset)); }


      CAEN_DGTZ_TriggerMode_t getSWTriggerMode()
        { CAEN_DGTZ_TriggerMode_t mode; errorHandler(CAEN_DGTZ_GetSWTriggerMode(handle_, &mode)); return mode; }

      void setSWTriggerMode(CAEN_DGTZ_TriggerMode_t mode)
      { errorHandler(CAEN_DGTZ_SetSWTriggerMode(handle_, mode)); }


      CAEN_DGTZ_TriggerMode_t getChannelSelfTrigger(uint32_t channel)
      { CAEN_DGTZ_TriggerMode_t mode; errorHandler(CAEN_DGTZ_GetChannelSelfTrigger(handle_, channel, &mode)); return mode; }

      void setChannelSelfTrigger(uint32_t channel, CAEN_DGTZ_TriggerMode_t mode)
      { errorHandler(CAEN_DGTZ_SetChannelSelfTrigger(handle_, mode, 1<<channel)); }

        CAEN_DGTZ_TriggerMode_t getGroupSelfTrigger(uint32_t group)
        { CAEN_DGTZ_TriggerMode_t mode; errorHandler(CAEN_DGTZ_GetGroupSelfTrigger(handle_, group, &mode)); return mode; }
        void setGroupSelfTrigger(uint32_t group, CAEN_DGTZ_TriggerMode_t mode)
        { errorHandler(CAEN_DGTZ_SetGroupSelfTrigger(handle_, mode, 1<<group)); }

      uint32_t getChannelTriggerThreshold(uint32_t channel)
      { uint32_t treshold; errorHandler(CAEN_DGTZ_GetChannelTriggerThreshold(handle_, channel, &treshold)); return treshold; }

      void setChannelTriggerThreshold(uint32_t channel, uint32_t treshold)
      { errorHandler(CAEN_DGTZ_SetChannelTriggerThreshold(handle_, channel, treshold)); }

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

      void setRunSynchronizationMode(CAEN_DGTZ_RunSyncMode_t mode)
      { errorHandler(CAEN_DGTZ_SetRunSynchronizationMode(handle_, mode));}

      CAEN_DGTZ_RunSyncMode_t getRunSynchronizationMode()
      { CAEN_DGTZ_RunSyncMode_t mode; errorHandler(CAEN_DGTZ_GetRunSynchronizationMode(handle_, &mode)); return mode;}

      void setOutputSignalMode(CAEN_DGTZ_OutputSignalMode_t mode)
      { errorHandler(CAEN_DGTZ_SetOutputSignalMode(handle_, mode));}

      CAEN_DGTZ_OutputSignalMode_t getOutputSignalMode()
      { CAEN_DGTZ_OutputSignalMode_t mode; errorHandler(CAEN_DGTZ_GetOutputSignalMode(handle_, &mode)); return mode;}

      /// x751 only
      void setDESMode(CAEN_DGTZ_EnaDis_t mode)
      { errorHandler(CAEN_DGTZ_SetDESMode(handle_, mode));}
      /// x751 only
      CAEN_DGTZ_EnaDis_t getDESMode()
      { CAEN_DGTZ_EnaDis_t mode; errorHandler(CAEN_DGTZ_GetDESMode(handle_, &mode)); return mode;}

      //@{
      /** DPP-FW methods */
      void setDPPPreTriggerSize(int channel, uint32_t samples)
      { errorHandler(CAEN_DGTZ_SetDPPPreTriggerSize(handle_, channel, samples)); }

      uint32_t getDPPPreTriggerSize(int channel)
      { uint32_t samples; errorHandler(CAEN_DGTZ_GetDPPPreTriggerSize(handle_, channel, &samples)); return samples; }


      void setChannelPulsePolarity(uint32_t channel, CAEN_DGTZ_PulsePolarity_t polarity)
      { errorHandler(CAEN_DGTZ_SetChannelPulsePolarity(handle_, channel, polarity)); }

      CAEN_DGTZ_PulsePolarity_t getChannelPulsePolarity(uint32_t channel)
      { CAEN_DGTZ_PulsePolarity_t polarity; errorHandler(CAEN_DGTZ_GetChannelPulsePolarity(handle_, channel, &polarity)); return polarity; }

      void setDPPAcquisitionMode(CAEN_DGTZ_DPP_AcqMode_t mode, CAEN_DGTZ_DPP_SaveParam_t param)
      { errorHandler( CAEN_DGTZ_SetDPPAcquisitionMode(handle_, mode, param)); }

      void getDPPAcquisitionMode(CAEN_DGTZ_DPP_AcqMode_t &mode, CAEN_DGTZ_DPP_SaveParam_t &param)
      { errorHandler( CAEN_DGTZ_GetDPPAcquisitionMode(handle_, &mode, &param)); }

      void setDPPTriggerMode(CAEN_DGTZ_DPP_TriggerMode_t mode)
      { errorHandler( CAEN_DGTZ_SetDPPTriggerMode(handle_, mode)); }

      CAEN_DGTZ_DPP_TriggerMode_t getDPPTriggerMode()
      { CAEN_DGTZ_DPP_TriggerMode_t mode; errorHandler( CAEN_DGTZ_GetDPPTriggerMode(handle_, &mode)); return mode;}

      //@}

    }; // class Digitizer

    class Digitizer740 : public Digitizer
    {
    private:
        Digitizer740();
        Digitizer740(int handle, CAEN_DGTZ_BoardInfo_t boardInfo) : Digitizer(handle,boardInfo) {}
        friend Digitizer* Digitizer::open(CAEN_DGTZ_ConnectionType linkType, int linkNum, int conetNode, uint32_t VMEBaseAddress);
    public:
      virtual uint32_t channels() const { return groups()*channelsPerGroup(); }
      virtual uint32_t groups() const { return boardInfo_.Channels; } // for x740: boardInfo.Channels stores number of groups
      virtual uint32_t channelsPerGroup() const { return 8; }  // 8 channels per group for x740
    };

    Digitizer* Digitizer::open(CAEN_DGTZ_ConnectionType linkType, int linkNum, int conetNode, uint32_t VMEBaseAddress)
    {
        int handle;
        CAEN_DGTZ_BoardInfo_t boardInfo;
        errorHandler(CAEN_DGTZ_OpenDigitizer(linkType,linkNum,conetNode,VMEBaseAddress,&handle));
        errorHandler(CAEN_DGTZ_GetInfo(handle,&boardInfo));
        switch (boardInfo.FamilyCode)
        {
            case  CAEN_DGTZ_XX740_FAMILY_CODE:
                return new Digitizer740(handle,boardInfo);
            default:
                return new Digitizer(handle,boardInfo);;
        }
    }

} // namespace caen
#endif //_CAEN_HPP
