/**
 * jadaq (Just Another DAQ)
 * Copyright (C) 2018  Troels Blum <troels@blum.dk>
 *
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
 * Write data to HDF5 file
 *
 */

#include "DataHandlerHDF5.hpp"

DataHandlerHDF5::DataHandlerHDF5(std::string fileName)
{
    try
    {
        //TODO: Handle existing file
        file = new H5::H5File(fileName, H5F_ACC_TRUNC);
    } catch (H5::Exception& e)
    {
        std::cerr << "ERROR: could not open/create HDF5-file \"" << fileName <<  "\":" << e.getDetailMsg() << std::endl;
        throw;
    }

}

DataHandlerHDF5::~DataHandlerHDF5()
{

}

void DataHandlerHDF5::initialize(uuid runID, uint32_t digitizerID)
{

}

void DataHandlerHDF5::addEvent(Data::ListElement422 event)
{

}

void DataHandlerHDF5::tick(uint64_t timeStamp)
{

}
