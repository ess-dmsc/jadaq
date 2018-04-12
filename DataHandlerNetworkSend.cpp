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

DataHandlerNetworkSend::DataHandlerNetworkSend(std::string address, std::string port)
{
    try {
        udp::resolver resolver(sendIOService);
        udp::resolver::query query(udp::v4(), address.c_str(), port.c_str());
        //TODO Handle result array properly
        remoteEndpoint = *resolver.resolve(query);
        socket = new udp::socket(sendIOService);
        socket->open(udp::v4());
        sendBuf = new char[MAXBUFSIZE];
    } catch (std::exception& e) {
        std::cerr << "ERROR in UDP connection setup to " << address << ":" << port << " : " << e.what() << std::endl;
        throw;
    }

}

DataHandlerNetworkSend::~DataHandlerNetworkSend()
{
    delete[] sendBuf;
}

/*
// Pack and send out UDP
DEBUG(std::cout << "Packing events from " << name() << std::endl;)
commHelper->packedEvents = Data::packEventData(commHelper->eventData);
// Send data to preconfigured receiver
DEBUG(std::cout << "Sending " << commHelper->eventData->listEventsLength << " list and " <<
commHelper->eventData->waveformEventsLength << " waveform events packed into " <<
commHelper->packedEvents.dataSize << "b from " << name() << std::endl;)
commHelper->socket->send_to(boost::asio::buffer((char*)(commHelper->packedEvents.data), commHelper->packedEvents.dataSize), commHelper->remoteEndpoint);
*/