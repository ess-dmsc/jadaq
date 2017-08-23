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

#include "caen.hpp"
namespace caen {

    Digitizer *Digitizer::open(CAEN_DGTZ_ConnectionType linkType, int linkNum, int conetNode, uint32_t VMEBaseAddress) {
        int handle;
        CAEN_DGTZ_BoardInfo_t boardInfo;
        errorHandler(CAEN_DGTZ_OpenDigitizer(linkType, linkNum, conetNode, VMEBaseAddress, &handle));
        errorHandler(CAEN_DGTZ_GetInfo(handle, &boardInfo));
        switch (boardInfo.FamilyCode) {
            case CAEN_DGTZ_XX740_FAMILY_CODE:
                return new Digitizer740(handle, boardInfo);
            default:
                return new Digitizer(handle, boardInfo);;
        }
    }

} // namespace caen