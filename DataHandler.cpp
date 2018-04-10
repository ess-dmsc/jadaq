//
// Created by troels on 4/4/18.
//

#include "DataHandler.hpp"

DataHandlerFile::DataHandlerFile(std::string fileName)
{
    file = new std::fstream(fileName,std::fstream::out);
    if (!file->is_open())
    {
        throw std::runtime_error("Could not open data dump file: \"" + fileName + "\"");
    }
}

DataHandlerFile::~DataHandlerFile()
{
    if (file)
    {
        file->close();
    }
}

DataHandlerNetwork::DataHandlerNetwork(std::string address, std::string port)
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

DataHandlerNetwork::~DataHandlerNetwork()
{
    delete[] sendBuf;
}