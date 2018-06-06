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
 * Receive jadaq data over the network and hand off to a DataHandler
 *
 */

#include "NetworkReceive.hpp"
#include "DataWriterText.hpp"

NetworkReceive::NetworkReceive(std::string address, std::string port)
{
    try
    {
        /* Bind to any address if not explicitly provided */
        if (address == listenAll || address == "") {
            endpoint = udp::endpoint(udp::v4(), std::stoi(port));
        } else {
            endpoint = udp::endpoint(boost::asio::ip::address::from_string(address), std::stoi(port));
        }
        socket = new udp::socket(ioService, endpoint);
    }
    catch (std::exception& e)
    {
        std::cerr << "ERROR in UDP connection setup to " << address << ":" << port << " : " << e.what() << std::endl;
        throw;
    }
}

void NetworkReceive::newDataWriter(uuid newID)
{
    for (auto& db: dataBuffers)
    {
        uint32_t digitizerID = db.first;
        jadaq::vector<Data::ListElement422>& buffer = db.second;
        (*dataWriter)(&buffer,digitizerID,currentTimestamp);
    }
    dataBuffers.clear();
    delete dataWriter;
    runID = newID;
    dataWriter = new DataWriterText(runID);

}

void NetworkReceive::newTimeStamp(uint64_t timeStamp)
{
    for (auto& db: dataBuffers)
    {
        uint32_t digitizerID = db.first;
        jadaq::vector<Data::ListElement422>& buffer = db.second;
        (*dataWriter)(&buffer,digitizerID,currentTimestamp);
        buffer.clear();
    }
    currentTimestamp = timeStamp;
}


void NetworkReceive::run(volatile sig_atomic_t* interrupt)
{
    while(true)
    {
        size_t receivedBytes;
        udp::endpoint remoteEndpoint;
        try
        {
            receivedBytes = socket->receive_from(boost::asio::buffer((receiveBuffer.data), Data::maxBufferSize), remoteEndpoint);
        }
        catch (boost::system::system_error& error)
        {
            std::cerr << "ERROR receiving UDP package: " << error.what() << std::endl;
            break;
        }
        if (receivedBytes >= sizeof(Data::Header))
        {
            Data::Header* header = (Data::Header*) receiveBuffer.data;
            if (header->version != Data::currentVersion)
            {
                uint8_t* version = (uint8_t*)&(header->version);
                std::cerr << "ERROR UDP data version unsupported: " << version[0] << "," << version[1] << std::endl;
                continue;
            }
            if (header->elementType != Data::List422)
            {
                std::cerr << "ERROR UDP element type unsupported: " << std::endl;
                continue;
            }
            receiveBuffer.setElements(header->numElements);
            uuid id(header->runID);
            if (id != runID)
            {
                newDataWriter(id);
            }
            if (header->globalTime == currentTimestamp)
            {
                uint32_t digitizerID = header->digitizerID;
                dataBuffers[digitizerID].insert(receiveBuffer.begin(),receiveBuffer.end());
            }
            else if (header->globalTime > currentTimestamp)
            {
                newTimeStamp(header->globalTime);
                uint32_t digitizerID = header->digitizerID;
                dataBuffers[digitizerID].insert(receiveBuffer.begin(),receiveBuffer.end());
            }
            else
            {
                std::cerr << "Warning: skipping old timestamp." << std::endl;
            }

        } else {
            std::cerr << "ERROR receiving UDP package: package too small" << std::endl;
        }
        if(*interrupt)
        {
            std::cout << "Caught interrupt - stop data server and clean up." << std::endl;
            break;
        }
    }
    newTimeStamp(-1);
}
