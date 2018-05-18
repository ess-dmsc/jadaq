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
private:
    typedef typename DataHandler<E>::template ContainerPair<C> ContainerPair;
    H5::H5File* file = nullptr;
    H5::Group* root = nullptr;
    std::map<uint32_t, std::pair<H5::Group*, ContainerPair*> > containerMap;
    std::pair<H5::Group*, ContainerPair*> addDigitizer_(uint32_t digitizerID)
    {
        H5::Group* digitizer = new H5::Group(file->createGroup(std::to_string(digitizerID)));
        ContainerPair* buffers = new ContainerPair{};
        auto res = std::make_pair(digitizer,buffers);
        containerMap[digitizerID] = res;
        return res;
    }

public:
    DataHandlerHDF5(uuid runID)
            : DataHandler<E>(runID)
    {
        std::string filename = "jadaq-run-" + runID.toString() + ".md5";
        try
        {
            file = new H5::H5File(filename, H5F_ACC_TRUNC);
            root = new H5::Group(file->openGroup("/"));
        } catch (H5::Exception& e)
        {
            std::cerr << "ERROR: could not open/create HDF5-file \"" << filename <<  "\":" << e.getDetailMsg() << std::endl;
            throw;
        }
    }

    ~DataHandlerHDF5()
    {
        assert(file);
        for(auto itr: containerMap)
        {
            write(itr.second->current, itr.first);
            assert(itr.second->next->empty());
            delete itr.second;
        }
        root->close();
        delete root;
        file->close();
        delete file;
    }
    void addDigitizer(uint32_t digitizerID) override { addDigitizer_(digitizerID); }

    size_t handle(const DPPEventAccessor<E>& accessor, uint32_t digitizerID) override
    {
        namespace ph = std::placeholders;
        std::pair<H5::Group*, ContainerPair*> containers;

        auto itr = containerMap.find(digitizerID);
        if (itr == containerMap.end())
        {
            containers = addDigitizer_(digitizerID);
        } else {
            containers= itr->second;
        }
        size_t events = this->handle_(accessor, containers.second, std::bind(&DataHandlerHDF5::write,this,ph::_1,containers.first));
        assert(containers.second->next->empty());
        return events;
    }

    void write(const C<E>* buffer, H5::Group* digitizer)
    {
        const hsize_t size[1] = {buffer->size()};
        H5::DataSpace dataspace(1, size);
        H5::DataSet dataset = digitizer->createDataSet(std::to_string(this->globalTimeStamp), E::h5type(), dataspace );
        H5::Attribute a = dataset.createAttribute("globalTimeStamp", H5::PredType::NATIVE_UINT64, H5::DataSpace(H5S_SCALAR));
        a.write(H5::PredType::NATIVE_UINT64,&this->globalTimeStamp);
        a.close();
        dataset.write(buffer->data(), E::h5type());
    }


};

#endif //JADAQ_DATAHANDLERHDF5_HPP
