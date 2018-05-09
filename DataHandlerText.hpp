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
#include "DataHandler.hpp"

class DataHandlerText: public DataHandler
{
public:
    DataHandlerText(uuid runID);
    ~DataHandlerText();
    void addDigitizer(uint32_t digitizerID) { addDigitizer_(digitizerID); }
    size_t handle(const DPPEventLE422Accessor& accessor, uint32_t digitizerID) override;
    void write(const std::vector<Data::ListElement422>* buffer, uint32_t digitizerID);
private:
    std::fstream* file = nullptr;
    std::map<uint32_t, DataHandler::ContainerPair<std::vector, Data::ListElement422>* > bufferMap;
    DataHandler::ContainerPair<std::vector, Data::ListElement422>* addDigitizer_(uint32_t digitizerID)
    {
        *file << "# digitizerID: " << digitizerID << std::endl;
        DataHandler::ContainerPair<std::vector, Data::ListElement422>* buffers = new DataHandler::ContainerPair<std::vector, Data::ListElement422>();
        bufferMap[digitizerID] = buffers;
        return buffers;
    }

};

#endif //JADAQ_DATAHANDLERTEXT_HPP
