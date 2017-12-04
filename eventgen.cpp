/**
 * jadaq (Just Another DAQ)
 * Copyright (C) 2017  Troels Blum <troels@blum.dk>
 * Contributions from  Jonas Bardino <bardino@nbi.ku.dk>
 *
 * @file
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
 * A simple event generator sending out dummy digitizer event packages
 * on UDP for e.g. testing the hdf5writer.
 *
 */

#include <getopt.h>
#include <signal.h>
#include <chrono>
#include <thread>
#include <iostream>
#include <string>
#include <ctime>
#include <boost/array.hpp>
#include <boost/asio.hpp>

#include "DataFormat.hpp"

using boost::asio::ip::udp;

/* Keep running marker and interrupt signal handler */
static int interrupted = 0;
static void interrupt_handler(int s)
{
    interrupted = 1;
}
static void setup_interrupt_handler()
{
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = interrupt_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);
    sigaction(SIGTERM, &sigIntHandler, NULL);
}

void usageHelp(char *name) 
{
    std::cout << "Usage: " << name << " [<options>]" << std::endl;
    std::cout << "Where <options> can be:" << std::endl;
    std::cout << "--address / -a ADDRESS   the UDP network address to send to." << std::endl;
    std::cout << "--port / -p PORT         the UDP network port to send to." << std::endl;
    std::cout << std::endl << "Generates events and sends them out on the network " << std::endl;
    std::cout << "as UDP packages for the provided destination. Use a broadcast " << std::endl;
    std::cout << "address to target multiple listeners." << std::endl;
}

int main(int argc, char **argv) {
    const char* const short_opts = "a:hp:";
    const option long_opts[] = {
        {"address", 1, nullptr, 'a'},
        {"help", 0, nullptr, 'h'},
        {"port", 1, nullptr, 'p'},
        {nullptr, 0, nullptr, 0}
    };

    /* Default option values */
    std::string address = DEFAULT_UDP_ADDRESS, port = DEFAULT_UDP_PORT;

    /* Parse command line options */
    while (true) {
        const auto opt = getopt_long(argc, argv, short_opts, long_opts, nullptr);
        if (-1 == opt)
            break;

        switch (opt) {
        case 'a':
            address = std::string(optarg);
            break;
        case 'p':
            port = std::string(optarg);
            break;
        case 'h': // -h or --help
        case '?': // Unrecognized option
        default:
            usageHelp(argv[0]);
            break;
        }
    }

    /* No further command-line arguments */
    if (argc - optind > 0) {
        usageHelp(argv[0]);
        exit(1);
    }

    /* Helpers */
    boost::asio::io_service io_service;
    udp::endpoint receiver_endpoint;
    udp::socket *socket = NULL;
    /* NOTE: use a static buffer of MAXBUFSIZE bytes for sending */
    char send_buf[MAXBUFSIZE];
    Data::EventData *eventData;
    Data::Meta *metadata;
    Data::PackedEvents packedEvents;

    uint32_t eventsSent = 0;
    uint32_t eventIndex = 0, eventsTarget = 0;

    /* For generated events */
    std::string flavor;
    std::string digitizer, digitizerModel;
    uint16_t digitizerID = 0;
    uint32_t channel = 0, charge = 0, localtime = 0;
    uint64_t globaltime = 0;
    
    std::cout << "Sending UDP packages to: " << address << ":" << port << std::endl;

    /* Setup UDP sender */
    try {
        udp::resolver resolver(io_service);
        udp::resolver::query query(udp::v4(), address.c_str(), port.c_str());
        receiver_endpoint = *resolver.resolve(query);
        socket = new udp::socket(io_service);
        socket->open(udp::v4());
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    /* Prepare and start event generator */
    std::cout << "Setup event generator" << std::endl;

    /* Set up interrupt handler and start handling acquired data */
    setup_interrupt_handler();

    std::cout << "Running event generator loop - Ctrl-C to interrupt" << std::endl;

    uint32_t throttleDown = 0;
    bool keepRunning = true;
    while(keepRunning) {
        /* Continuously receive and dump data */
        if (throttleDown > 0) {
            /* NOTE: for running without hogging CPU if nothing to do */
            std::this_thread::sleep_for(std::chrono::milliseconds(throttleDown));
        }
        try {
            digitizerModel = "V1740D";
            digitizerID = 137;
            digitizer = "V1740D_137";
            globaltime = std::time(nullptr);
            flavor = "list";
            eventsTarget = 1 + globaltime % 3;

            /* Reset send buffer each time to prevent any stale data */
            memset(send_buf, 0, MAXBUFSIZE);
            eventData = Data::setupEventData((void *)send_buf, MAXBUFSIZE, eventsTarget, 0);

            std::cout << "Prepared eventData " << eventData << " from send_buf " << (void *)send_buf << std::endl;

            metadata = eventData->metadata;
            /* NOTE: safe copy with explicit string termination */
            strncpy(eventData->metadata->digitizerModel, digitizerModel.c_str(), MAXMODELSIZE);
            eventData->metadata->digitizerModel[MAXMODELSIZE-1] = '\0';
            eventData->metadata->digitizerID = digitizerID;
            eventData->metadata->globalTime = globaltime;

            std::cout << "Prepared eventData has " << eventData->listEventsLength << " listEvents " << std::endl;

            /* Loop generating variable number of events */
            std::cout << "Generate " << eventsTarget << " fake events from " << digitizer << std::endl;

            for (eventIndex = 0; eventIndex < eventsTarget; eventIndex++) {
                /* Create a new dataset named after event index under
                 * globaltime group if it doesn't exist already. */
                channel = (eventIndex % 2) * 31;
                localtime = (globaltime + eventIndex) & 0xFFFF;
                charge = 242 + (localtime+eventIndex*13) % 100;
                std::cout << "Filling event at " << globaltime << " from " << digitizer << " channel " << channel << " localtime " << localtime << " charge " << charge << std::endl;
                std::cout << "DEBUG: listEvents at " << eventData->listEvents << std::endl;
                eventData->listEvents[eventIndex].localTime = localtime;
                eventData->listEvents[eventIndex].extendTime = 0;
                eventData->listEvents[eventIndex].adcValue = charge;
                eventData->listEvents[eventIndex].channel = channel;
            }

            std::cout << "Packing event at " << globaltime << " from " << digitizer << " channel " << channel << " localtime " << localtime << " charge " << charge << std::endl;
            packedEvents = Data::packEventData(eventData, eventsTarget, 0);
            /* Send data to preconfigured receiver */
            std::cout << "Sending packed event of " << packedEvents.dataSize << "b at " << globaltime << " from " << digitizer << " channel " << channel << " localtime " << localtime << " charge " << charge << std::endl;
            socket->send_to(boost::asio::buffer((char*)(packedEvents.data), packedEvents.dataSize), receiver_endpoint);
            eventsSent += eventsTarget;

            /* TODO: make this delay command-line configurable? */
            throttleDown = 1000;
        } catch(std::exception& e) {
            std::cerr << "unexpected exception during reception: " << e.what() << std::endl;
            /* NOTE: throttle down on errors */
            throttleDown = 2000;
        }
        if (interrupted) {
            std::cout << "caught interrupt - stop file writer and clean up." << std::endl;
            break;
        }
    }

    /* Stop event generator and clean up */
    std::cout << "Stop event generator and clean up" << std::endl;

    /* Close UDP listener */
    if (socket != NULL)
        delete socket;

    std::cout << "Shutting down." << std::endl;

    return 0;
}
