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

#ifndef JADAQ_DATAWRITERWORK_HPP
#define JADAQ_DATAWRITERWORK_HPP


/* Default to jumbo frame sized buffer */
#include <boost/asio.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/bind.hpp>
#include "uuid.hpp"
#include "DataFormat.hpp"
#include "container.hpp"
#include "DataWriter.hpp"

using boost::asio::ip::udp;

class DataWriterNetwork: public DataWriter
{
private:
    uuid runID;
    boost::asio::io_service ioService;
    udp::endpoint remoteEndpoint;
    udp::socket *socket = nullptr;

    template <typename E>
    void write(const jadaq::buffer<E>* buffer, uint32_t digitizerID, uint64_t globalTimeStamp)
    {
        Data::Header* header = (Data::Header*)buffer->data;
        header->runID = runID.value();
        header->globalTime = globalTimeStamp;
        header->digitizerID = digitizerID;
        header->version = Data::currentVersion;
        header->elementType = E::type();
        header->numElements = (uint16_t)buffer->size();

        socket->send_to(boost::asio::buffer(buffer->data, buffer->data_size()), remoteEndpoint);

    }

public:
    DataWriterNetwork(const std::string& address, const std::string& port, uuid runID_)
            : runID(runID_)
    {
        try {
            udp::resolver resolver(ioService);
            udp::resolver::query query(udp::v4(), address.c_str(), port.c_str());
            //TODO Handle result array properly
            remoteEndpoint = *resolver.resolve(query);
            socket = new udp::socket(ioService);
            socket->open(udp::v4());
        } catch (std::exception& e) {
            std::cerr << "ERROR in UDP connection setup to " << address << ":" << port << " : " << e.what() << std::endl;
            throw;
        }

    }

    void addDigitizer(uint32_t digitizerID) override
    {
        // TODO: This is where we will send the configuration over TCP
    }

    void operator()(const jadaq::vector<Data::ListElement422>* buffer, uint32_t digitizerID, uint64_t globalTimeStamp) override
    {
        throw std::runtime_error("Error: jadaq::vector not supported by DataWriterNetwork.");
    }
    void operator()(const jadaq::set<Data::ListElement422>* buffer, uint32_t digitizerID, uint64_t globalTimeStamp) override
    {
        throw std::runtime_error("Error: jadaq::set not supported by DataWriterNetwork.");
    }
    void operator()(const jadaq::buffer<Data::ListElement422>* buffer, uint32_t digitizerID, uint64_t globalTimeStamp) override
    {
        write(buffer,digitizerID,globalTimeStamp);
    }
};


#endif //JADAQ_DATAWRITERWORK_HPP
