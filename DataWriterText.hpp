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
#include <iomanip>
#include <mutex>
#include <cassert>
#include "uuid.hpp"
#include "DataFormat.hpp"
#include "container.hpp"
#include "DataWriter.hpp"

class DataWriterText: public DataWriter
{
private:
    std::fstream* file = nullptr;
    std::mutex mutex;
    template <typename E, template<typename...> typename C>
    void write(const C<E>* buffer, uint32_t digitizerID, uint64_t globalTimeStamp)
    {
        mutex.lock();
        *file << "#" << std::setw(10) << "digitID" << " ";
        E::headerOn(*file);
        *file << std::endl << "@" << globalTimeStamp << std::endl;
        for(const E& element: *buffer)
        {
            *file << std::setw(10) << digitizerID << " " << element << "\n";
        }
        mutex.unlock();
    }

public:
    DataWriterText(uuid runID)
    {
        std::string filename = "jadaq-run-" + runID.toString() + ".txt";
        file = new std::fstream(filename,std::fstream::out);
        if (!file->is_open())
        {
            throw std::runtime_error("Could not open text data file: \"" + filename + "\"");
        }
        *file << "# runID: " << runID << std::endl;
    }

    ~DataWriterText() override
    {
        assert(file);
        mutex.lock(); // Wait if someone is still writing data
        file->close();
        mutex.unlock();
    }

    void addDigitizer(uint32_t digitizerID) override
    {
        mutex.lock();
        *file << "# digitizerID: " << digitizerID << std::endl;
        mutex.unlock();
    }

    void operator()(const jadaq::vector<Data::ListElement422>* buffer, uint32_t digitizerID, uint64_t globalTimeStamp) override
    { write(buffer,digitizerID,globalTimeStamp); }

    void operator()(const jadaq::set<Data::ListElement422>* buffer, uint32_t digitizerID, uint64_t globalTimeStamp) override
    { write(buffer,digitizerID,globalTimeStamp); }

    void operator()(const jadaq::buffer<Data::ListElement422>* buffer, uint32_t digitizerID, uint64_t globalTimeStamp) override
    {
        throw std::runtime_error("Error: jadaq::buffer not supported by DataWriterText.");
    }

};

#endif //JADAQ_DATAHANDLERTEXT_HPP
