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

size_t DataHandlerText::handle(DPPEventLE422Accessor& accessor, uint32_t digitizerID)
{
    size_t events = 0;
    for (uint16_t channel = 0; channel < accessor.channels(); channel++)
    {
        for (uint32_t i = 0; i < accessor.events(channel); ++i)
        {
            Data::ListElement422 event = accessor.listElement422(channel,i);
            *file << std::setw(10) << event.localTime << " " << std::setw(10) << digitizerID << " " <<
                  std::setw(10) << event.channel << " " << std::setw(10) << event.adcValue << "\n";
            events += 1;
        }
    }
    return events;
}

