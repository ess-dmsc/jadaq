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

#ifndef JADAQ_DATAHANDLERHDF5_HPP
#define JADAQ_DATAHANDLERHDF5_HPP

#include <string>
#include <H5Cpp.h>
#include "DataHandler.hpp"

template <template <typename...> typename C, typename E>
class DataHandlerHDF5: public DataHandler<E>
{
public:
    DataHandlerHDF5(uuid runID)
            : DataHandler<E>(runID)
    {
        std::string filename = "jadaq-run-" + runID.toString() + ".md5";
        try
        {
            //TODO: Handle existing file
            file = new H5::H5File(filename, H5F_ACC_TRUNC);
        } catch (H5::Exception& e)
        {
            std::cerr << "ERROR: could not open/create HDF5-file \"" << filename <<  "\":" << e.getDetailMsg() << std::endl;
            throw;
        }
    }

    ~DataHandlerHDF5();
    size_t handle(const DPPEventAccessor<E>& accessor, uint32_t digitizerID) override;
private:
    H5::H5File* file = nullptr;
};

#endif //JADAQ_DATAHANDLERHDF5_HPP
