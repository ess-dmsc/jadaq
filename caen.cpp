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
 * This file just does the runtime initialization of a digitizer.
 *
 */

#include "caen.hpp"
namespace caen {

/** Convenience helper to detect and instantiate most specific
 * Digitizer class for further use.
 * @param linkType: which kind of link or bus to connect with
 * @param linkNum: device index on the link or bus
 * @param conetNode: node index if using CONET
 * @param VMEBaseAddress: device address if using VME connection
 * @returns
 * Abstracted digitizer instance of the appropriate kind.
 */

// TODO get rid of "Raw" functions
Digitizer *Digitizer::open(CAEN_DGTZ_ConnectionType linkType, int linkNum,
                           int conetNode, uint32_t VMEBaseAddress) {
  int handle;
  CAEN_DGTZ_BoardInfo_t boardInfo;
  CAEN_DGTZ_DPPFirmware_t firmware;
  handle = openRawDigitizer(linkType, linkNum, conetNode, VMEBaseAddress);
  boardInfo = getRawDigitizerBoardInfo(handle);
  firmware = getRawDigitizerDPPFirmware(handle);
  /* NOTE: Digitizer destructor takes care of closing Digitizer handle */
  switch (boardInfo.FamilyCode) {
  case CAEN_DGTZ_XX740_FAMILY_CODE:
    if (firmware == CAEN_DGTZ_DPPFirmware_QDC)
      return new Digitizer740DPP(handle, boardInfo);
    else
      return new Digitizer740(handle, boardInfo);
  default:
    return new Digitizer(handle, boardInfo);
    ;
  }
}

} // namespace caen
