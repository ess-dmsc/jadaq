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

#ifndef JADAQ_DATAHANDLERNETWORKSEND_HPP
#define JADAQ_DATAHANDLERNETWORKSEND_HPP


/* Default to jumbo frame sized buffer */
#include <boost/asio.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/bind.hpp>
#include "DataHandler.hpp"

using boost::asio::ip::udp;
class DataHandlerNetworkSend: public DataHandler
{
public:
    DataHandlerNetworkSend(std::string address, std::string port, uuid runID);
    ~DataHandlerNetworkSend();
    void addDigitizer(uint32_t digitizerID) override;
    size_t handle(DPPEventLE422Accessor& accessor, uint32_t digitizerID) override;
private:
    uint32_t digitizerID;
    boost::asio::io_service ioService;
    udp::endpoint remoteEndpoint;
    udp::socket *socket = nullptr;
    char* buffer;
    const size_t bufferSize = DataHandler::maxBufferSize;
    uint16_t numElements;
    Data::ElementType elementType;
    void send();
};


#endif //JADAQ_DATAHANDLERNETWORKSEND_HPP
