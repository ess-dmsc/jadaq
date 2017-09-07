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

/*
 * This file contains functions and definitions that belong in the CAENDigitizer library.
 * But either they are not there or they are not exposed
 */


#ifndef JADAQ_CAENDIGITIZER_H
#define JADAQ_CAENDIGITIZER_H

#include <CAENDigitizer.h>

#define V1740_DPP_QDC_CODE    (0x87)
#define CAEN_DGTZ_DPPFirmware_QDC (CAEN_DGTZ_DPPFirmware_ZLE+1)

CAEN_DGTZ_ErrorCode CAENDGTZ_API CAEN_DGTZ_GetDPPFirmwareType(int handle, CAEN_DGTZ_DPPFirmware_t* firmware);

#endif //JADAQ_CAENDIGITIZER_H
