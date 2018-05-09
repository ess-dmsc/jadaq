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
 * Send collected data over the network
 *
 */

#include <iostream>
#include "DataHandlerNetworkSend.hpp"

DataHandlerNetworkSend::DataHandlerNetworkSend(std::string address, std::string port, uuid runID)
        : DataHandler(runID)
        , numElements(0)
        , elementType(Data::None)
        , digitizerID(0)
{
    try {
        udp::resolver resolver(ioService);
        udp::resolver::query query(udp::v4(), address.c_str(), port.c_str());
        //TODO Handle result array properly
        remoteEndpoint = *resolver.resolve(query);
        socket = new udp::socket(ioService);
        socket->open(udp::v4());
        buffer = new char[bufferSize];
    } catch (std::exception& e) {
        std::cerr << "ERROR in UDP connection setup to " << address << ":" << port << " : " << e.what() << std::endl;
        throw;
    }

}

DataHandlerNetworkSend::~DataHandlerNetworkSend()
{
    delete[] buffer;
}

void DataHandlerNetworkSend::addDigitizer(uint32_t digitizerID_)
{
    // TODO: This is where we will send the configuration over TCP
    if (digitizerID == 0)
    {
        digitizerID = digitizerID_;
    } else {
        throw std::runtime_error{"Error in DataHandlerNetworkSend::addDigitizer: Only one digitizer supported. Already set."};
    }

}

// TODO Template this function?
size_t DataHandlerNetworkSend::handle(const DPPEventLE422Accessor& accessor, uint32_t digitizerID)
{
    Data::ListElement422* elementPtr = (Data::ListElement422*)(buffer + sizeof(Data::Header));
    elementPtr += numElements;
    size_t events = 0;
    for (uint16_t channel = 0; channel < accessor.channels(); channel++)
    {
        for (uint32_t i = 0; i < accessor.events(channel); ++i)
        {
            *elementPtr = accessor.listElement422(channel,i);
            numElements += 1;
            elementPtr += 1;
            events += 1;
            if ((char*)(elementPtr + 1) > buffer + bufferSize) // would the next element overflow the buffer
            {
                send();
            }

        }
    }

    return events;
}

void DataHandlerNetworkSend::send()
{
    if (numElements > 0 and elementType != Data::None)
    {
        Data::Header *header = (Data::Header *) buffer;
        header->runID = runID.value();
        header->globalTime = 0; // TODO
        header->digitizerID = digitizerID;
        header->version = Data::currentVersion;
        header->elementType = elementType;
        header->numElements = numElements;

        size_t dataSize = sizeof(Data::Header) + Data::elementSize(elementType) * numElements;
        socket->send_to(boost::asio::buffer(buffer, dataSize), remoteEndpoint);
    }
    numElements = 0;
    elementType = Data::None;
}
