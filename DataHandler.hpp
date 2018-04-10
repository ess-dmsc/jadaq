//
// Created by troels on 4/4/18.
//

#ifndef JADAQ_DATAHANDLER_HPP
#define JADAQ_DATAHANDLER_HPP

#include <boost/asio.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
#include <fstream>
#include "DataFormat.hpp"


class DataHandler {
public:
    virtual void addEvent() = 0;
};

class DataHandlerFile: public DataHandler
{
public:
    DataHandlerFile(std::string fileName);
    ~DataHandlerFile();
private:
    std::fstream* file = nullptr;
};


/* Default to jumbo frame sized buffer */
#define MAXBUFSIZE (9000)
using boost::asio::ip::udp;
class DataHandlerNetwork: public DataHandler
{
public:
    DataHandlerNetwork(std::string address, std::string port);
    ~DataHandlerNetwork();
private:
    boost::asio::io_service sendIOService;
    udp::endpoint remoteEndpoint;
    udp::socket *socket = nullptr;
    char *sendBuf;
};


#endif //JADAQ_DATAHANDLER_HPP
