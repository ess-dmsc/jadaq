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
#include <H5Cpp.h>
#include <H5PacketTable.h>
#include "DataFormat.hpp"
#include "container.hpp"

class DataWriterHDF5
{
private:
    struct DigitizerInfo
    {
        FL_PacketTable* previous = nullptr;
        FL_PacketTable* current = nullptr;
        H5::Group* group = nullptr;
        uint64_t currentTimeStamp = 0;
        FL_PacketTable*& getTable(uint64_t timeStamp)
        {
            if (timeStamp == currentTimeStamp)
                return current;
            else if(timeStamp < currentTimeStamp)
                return previous;
            else {
                if (previous)
                    delete(previous);
                previous = current;
                currentTimeStamp = timeStamp;
                current = nullptr;
                return current;
            }
        }
    };
    H5::H5File* file = nullptr;
    H5::Group* root = nullptr;
    std::mutex mutex;
    std::map<uint32_t, DigitizerInfo> digitizerInfo;

    DigitizerInfo& getDigitizerInfo(uint32_t digitizerID)
    {
        auto itr = digitizerInfo.find(digitizerID);
        if (itr != digitizerInfo.end())
        {
            return itr->second;
        } else
        {
            DigitizerInfo info;
            info.group = new H5::Group(file->createGroup(std::to_string(digitizerID)));
            digitizerInfo[digitizerID] = info;
            return digitizerInfo[digitizerID];
        }
    }
    void writeAttribute(std::string name, H5::DataSet& dataset, const H5::PredType& type, const void* data) const
    {
        try {
            H5::Attribute a = dataset.createAttribute(name, type, H5::DataSpace(H5S_SCALAR));
            a.write(type,data);
            a.close();
        } catch (H5::Exception& e)
        {
            std::cerr << "ERROR: DataWriterHDF5 can not writeAttribute \"" << name << "\"." << std::endl;
            throw;
        }
    }


public:
  DataWriterHDF5(std::string pathname, std::string runID)
    {
        if (!pathname.empty() && *pathname.rbegin() != '/')
          pathname += '/';
        std::string filename = pathname + "jadaq-run-" + runID + ".h5";
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
        getDigitizerInfo(digitizerID);
        mutex.unlock();
    }

    static bool network() { return false; }

    template <typename E>
    void operator()(const jadaq::buffer<E>* buffer, uint32_t digitizerID, uint64_t globalTimeStamp)
    {
        if (buffer->size() < 1)
            return;
        mutex.lock();
        DigitizerInfo& info = getDigitizerInfo(digitizerID);
        FL_PacketTable*& table = info.getTable(globalTimeStamp);
        if (table == nullptr)
        {
            table = new FL_PacketTable(info.group->getId(), std::to_string(globalTimeStamp).c_str(),
            buffer->begin()->h5type().getId(),1); // TODO find a suitable chunk size - last argument
        }
        if (table->AppendPackets(buffer->size(),(void*)buffer->data())); // Fuck this is the worst interface ever!
        {
            std::cerr << "Error while writing to HDF5 file: " <<
                      "\n\t " << "HDF5::write( " << digitizerID << ", " << globalTimeStamp <<
                      ", " << buffer->size() << " )" << std::endl;

        }
        mutex.unlock();
    }
};


#endif //JADAQ_DATAHANDLERHDF5_HPP
