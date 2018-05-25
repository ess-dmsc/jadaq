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
#include <mutex>
#include <map>
#include <vector>
#include <set>
#include <cassert>
#include "uuid.hpp"
#include <H5Cpp.h>
#include "DataWriterNull.hpp"

class DataWriterHDF5
{
private:
    H5::H5File* file = nullptr;
    H5::Group* root = nullptr;
    std::mutex mutex;
    std::map<uint32_t, H5::Group*> digitizerMap;
    H5::Group* addDigitizer_(uint32_t digitizerID)
    {
        H5::Group* digitizerGroup = new H5::Group(file->createGroup(std::to_string(digitizerID)));
        digitizerMap[digitizerID] = digitizerGroup;
        return digitizerGroup;
    }
    void writeAttribute(std::string name, H5::DataSet& dataset, const H5::PredType& type, const void* data) const
    {
        H5::Attribute a = dataset.createAttribute(name, type, H5::DataSpace(H5S_SCALAR));
        a.write(type,data);
        a.close();
    }

    template <typename E, template<typename...> typename C>
    void write(const C<E>* buffer, uint32_t digitizerID, uint64_t globalTimeStamp)
    {
        mutex.lock();
        H5::Group* digitizerGroup;
        auto itr = digitizerMap.find(digitizerID);
        if (itr != digitizerMap.end())
        {
            digitizerGroup = itr->second;
        } else
        {
            digitizerGroup = addDigitizer_(digitizerID);
        }
        const hsize_t size[1] = {buffer->size()};
        H5::DataSpace dataspace(1, size);
        H5::DataSet dataset = digitizerGroup->createDataSet(std::to_string(globalTimeStamp), E::h5type(), dataspace );
        writeAttribute("globalTimestamp", dataset,H5::PredType::NATIVE_UINT64,&globalTimeStamp);
        dataset.write(buffer->data(), E::h5type());
        mutex.unlock();
    }

public:
    DataWriterHDF5(uuid runID)
    {
        std::string filename = "jadaq-run-" + runID.toString() + ".h5";
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

    ~DataWriterHDF5()
    {
        assert(file);
        mutex.lock(); // Wait if someone is still writing data
        root->close();
        delete root;
        file->close();
        delete file;
        mutex.unlock();
    }
    void addDigitizer(uint32_t digitizerID)
    {
        mutex.lock();
        addDigitizer_(digitizerID);
        mutex.unlock();
    }

    template <typename E>
    void operator()(const std::vector<E>* buffer, uint32_t digitizerID, uint64_t globalTimeStamp)
    { write(buffer,digitizerID,globalTimeStamp); }

    template <typename E>
    void operator()(const std::set<E>* buffer, uint32_t digitizerID, uint64_t globalTimeStamp)
    {
        std::vector<E> v(buffer->begin(), buffer->end());
        write(&v,digitizerID,globalTimeStamp);
    }

};

#endif //JADAQ_DATAHANDLERHDF5_HPP
