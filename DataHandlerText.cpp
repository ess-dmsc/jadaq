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

#include <iomanip>
#include <functional>
#include "DataHandlerText.hpp"

DataHandlerText::DataHandlerText(uuid runID)
        : DataHandler(runID)
{
    std::string filename = "jadaq-run-" + runID.toString() + ".txt";
    file = new std::fstream(filename,std::fstream::out);
    if (!file->is_open())
    {
        throw std::runtime_error("Could not open text data file: \"" + filename + "\"");
    }
    *file << "# runID: " << runID << std::endl;
}

DataHandlerText::~DataHandlerText()
{
    if (file)
    {
        file->close();
    }
}

size_t DataHandlerText::handle(const DPPEventLE422Accessor& accessor, uint32_t digitizerID)
{
    using namespace std::placeholders;
    DataHandler::ContainerPair<std::vector<Data::ListElement422> >* myBuffers;
    auto itr = buffers.find(digitizerID);
    if (itr == buffers.end())
    {
        myBuffers = addDigitizer_(digitizerID);
    } else {
        myBuffers = itr->second;
    }
    return handle_(accessor, myBuffers, std::bind(&DataHandlerText::write,this,_1,digitizerID));
}

void DataHandlerText::write(const std::vector<Data::ListElement422>* buffer, uint32_t digitizerID)
{
    *file << "@" << globalTimeStamp << std::endl;
    for(const Data::ListElement422& element: *buffer)
    {
        *file << std::setw(10) << element.localTime << " " << std::setw(10) << digitizerID << " " <<
              std::setw(10) << element.channel << " " << std::setw(10) << element.adcValue << "\n";
    }

}


