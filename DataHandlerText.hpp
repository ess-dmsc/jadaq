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
 * Write data to plain text file
 *
 */

#ifndef JADAQ_DATAHANDLERTEXT_HPP
#define JADAQ_DATAHANDLERTEXT_HPP

#include <fstream>
#include <vector>
#include <map>
#include <iomanip>
#include <functional>
#include "DataHandler.hpp"

template <template <typename...> typename C, typename E>
class DataHandlerText: public DataHandler<E>
{
private:
    std::fstream* file = nullptr;
    std::map<uint32_t, typename DataHandler<E>::template ContainerPair<C>* > bufferMap;
    typename DataHandler<E>::template ContainerPair<C>* addDigitizer_(uint32_t digitizerID)
    {
        *file << "# digitizerID: " << digitizerID << std::endl;
        typename DataHandler<E>::template ContainerPair<C>* buffers = new typename DataHandler<E>::template ContainerPair<C>();
        bufferMap[digitizerID] = buffers;
        return buffers;
    }

public:
    DataHandlerText(uuid runID)
            : DataHandler<E>(runID)
    {
        std::string filename = "jadaq-run-" + runID.toString() + ".txt";
        file = new std::fstream(filename,std::fstream::out);
        if (!file->is_open())
        {
            throw std::runtime_error("Could not open text data file: \"" + filename + "\"");
        }
        *file << "# runID: " << runID << std::endl;
    }

    ~DataHandlerText()
    {
        assert(file);
        for(auto itr: bufferMap)
        {
            write(itr.second->current, itr.first);
            assert(itr.second->next->empty());
            delete itr.second;
        }
        file->close();
    }

    void addDigitizer(uint32_t digitizerID) override { addDigitizer_(digitizerID); }

    size_t handle(const DPPEventAccessor<E>& accessor, uint32_t digitizerID) override
    {
        namespace ph = std::placeholders;
        typename DataHandler<E>::template ContainerPair<C>* myBuffers;
        auto itr = bufferMap.find(digitizerID);
        if (itr == bufferMap.end())
        {
            myBuffers = addDigitizer_(digitizerID);
        } else {
            myBuffers = itr->second;
        }
        size_t events = this->handle_(accessor, myBuffers, std::bind(&DataHandlerText::write,this,ph::_1,digitizerID));
        assert(myBuffers->next->empty());
        return events;
    }

    void write(const C<E>* buffer, uint32_t digitizerID){
        *file << "@" << this->globalTimeStamp << std::endl;
        for(const Data::ListElement422& element: *buffer)
        {
            *file << std::setw(10) << element.localTime << " " << std::setw(10) << digitizerID << " " <<
                  std::setw(10) << element.channel << " " << std::setw(10) << element.adcValue << "\n";
        }

    }

};

#endif //JADAQ_DATAHANDLERTEXT_HPP
