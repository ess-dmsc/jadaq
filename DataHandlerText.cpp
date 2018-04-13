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
#include "DataHandlerText.hpp"

DataHandlerText::DataHandlerText(std::string fileName)
{
    file = new std::fstream(fileName,std::fstream::out);
    if (!file->is_open())
    {
        throw std::runtime_error("Could not open text data file: \"" + fileName + "\"");
    }
}

DataHandlerText::~DataHandlerText()
{
    if (file)
    {
        file->close();
    }
}

void DataHandlerText::initialize(uuid runID_, uint32_t digitizerID_)
{
    runID = runID_;
    digitizerID = digitizerID_;
    *file << "# runID: " << runID << std::endl <<
          "# digitizerID: " << digitizerID << std::endl;
}

void DataHandlerText::addEvent(Data::ListElement422 event)
{
    *file << std::setw(16) << event.localTime << " " << digitizerID << " " << std::setw(8) << event.channel << " " << std::setw(8) << event.adcValue << "\n";
}

