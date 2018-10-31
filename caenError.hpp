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
 * Wrap CAEN errors.
 *
 */

#ifndef _CAEN_ERROR_HPP
#define _CAEN_ERROR_HPP

#include <CAENDigitizer.h>
#include <CAENDigitizerType.h>
#include <cstdint>
#include <string>

namespace caen {
class Error : public std::exception {
private:
  CAEN_DGTZ_ErrorCode code_;
#ifdef DEBUG
  std::string where_;
#endif
public:
#define __ERR_TO_STR(X)                                                        \
  case (X):                                                                    \
    return (#X);
  static const char *digitizerErrorString(CAEN_DGTZ_ErrorCode code) {
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
    default:
      return "Unknown Error";
    }
  }

  Error(CAEN_DGTZ_ErrorCode code
#ifdef DEBUG
        ,
        std::string where = ""
#endif
        )
      : code_(code)
#ifdef DEBUG
        ,
        where_(where)
#endif
  {
  }

  virtual const char *what() const noexcept {
    return digitizerErrorString(code_);
  }

#ifdef DEBUG
  virtual const char *where() const noexcept { return where_.c_str(); }
#endif
  int code() const { return code_; }
};
static inline void errorHandler(CAEN_DGTZ_ErrorCode code
#ifdef DEBUG
                                ,
                                std::string caller
#endif
                                ) {
  if (code != CAEN_DGTZ_Success) {
    throw Error(code
#ifdef DEBUG
                ,
                caller
#endif
                );
  }
}

#ifdef DEBUG
class Reporter {
public:
  Reporter(std::string func) : func_(func) {}
  void operator()(CAEN_DGTZ_ErrorCode code) {
    return errorHandler(code, caller_);
  }

private:
  std::string func_;
};
#undef errorHandler
#define errorHandler Reporter(__FUNC__)
#endif
} // namespace caen
#endif //_CAEN_ERROR_HPP
