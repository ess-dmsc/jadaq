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
#include "DataHandlerText.hpp"

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
    buffer = new char[bufferSize];
}

NetworkReceive::~NetworkReceive()
{
    delete[] buffer;
}

void NetworkReceive::start(int* keepRunning)
{
    while(keepRunning)
    {
        size_t receivedBytes;
        udp::endpoint remoteEndpoint;
        try
        {
            receivedBytes = socket->receive_from(boost::asio::buffer((buffer), DataHandler::maxBufferSize), remoteEndpoint);
        }
        catch (boost::system::system_error& error)
        {
            std::cerr << "ERROR receiving UDP package: " << error.what() << std::endl;
            throw;
        }
        if (receivedBytes >= sizeof(Data::Header))
        {
            Data::Header* header = (Data::Header*) buffer;
            uuid runID(header->runID);
            uint32_t digitizerID = header->digitizerID;
            auto itr = dataHandlers.find(std::make_pair(runID,digitizerID));
            DataHandler* dataHandler;
            if (itr != dataHandlers.end())
            {
                dataHandler = itr->second;
            } else {
                dataHandler = new DataHandlerText<std::vector,Data::ListElement422>(runID);
//                dataHandler->addDigitizer(digitizerID);
                dataHandlers.insert(std::make_pair(std::make_pair(runID,digitizerID),dataHandler));
            }
            if (header->version != Data::currentVersion)
            {
                uint8_t* version = (uint8_t*)&(header->version);
                std::cerr << "ERROR UDP data version unsupported: " << version[0] << "," << version[1] << std::endl;
            }
            if (header->elementType != Data::List422)
            {
                std::cerr << "ERROR UDP element type unsupported: " << std::endl;
            }
            //dataHandler->tick(header->globalTime);
            Data::ListElement422* listElement = (Data::ListElement422*)(buffer + sizeof(Data::Header));
            uint16_t numElements = header->numElements;
            for (uint16_t i = 0; i < numElements; ++i)
            {
                //dataHandler->addEvent(listElement[i]);
            }

        } else {
            std::cerr << "ERROR receiving UDP package: package too small" << std::endl;
        }
    }
}
