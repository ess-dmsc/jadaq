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

#ifndef JADAQ_NETWORKRECEIVE_HPP
#define JADAQ_NETWORKRECEIVE_HPP

#include <boost/asio.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/bind.hpp>
#include "DataHandler.hpp"
#include "DataFormat.hpp"
#include "uuid.hpp"

using boost::asio::ip::udp;

class NetworkReceive
{
public:
    NetworkReceive(std::string address, std::string port);
    ~NetworkReceive() = default;
    void run(volatile sig_atomic_t* interrupt);
    static constexpr const char* listenAll = "*";
private:
    boost::asio::io_service ioService;
    udp::endpoint endpoint;
    udp::socket *socket = nullptr;
    jadaq::buffer<Data::ListElement422> receiveBuffer;
    const size_t bufferSize = Data::maxBufferSize;
    uuid runID{0};
    uint64_t currentTimestamp = 0;
    DataWriter dataWriter;
    std::map<uint32_t ,jadaq::buffer<Data::ListElement422> > dataBuffers;
    void newDataWriter(uuid newID);
    void newTimeStamp(uint64_t timeStamp);
};


#endif //JADAQ_NETWORKRECEIVE_HPP
