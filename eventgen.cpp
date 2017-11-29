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

#include <signal.h>
#include <chrono>
#include <thread>
#include <iostream>
#include <string>
#include <ctime>

#include "DataFormat.hpp"

#define EVENTFIELDS (3)

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

int main(int argc, char **argv) {
    if (argc > 3)
    {
        std::cout << "Usage: " << argv[0] << " <address> <port>" << std::endl;
        std::cout << "Generates events and sends them out as UDP packages to " << std::endl;
        std::cout << "given <address> and <port>." << std::endl;
        return -1;
    }

    /* Helpers */
    std::string address = "127.0.0.1";
    uint16_t port = 12345;
    uint32_t eventsSent = 0;
    std::string flavor;

    
    uint32_t eventIndex = 0, eventsTarget = 0;
    /* Dummy data */
    std::string digitizer, digitizerModel, digitizerID;
    uint32_t channel = 0, charge = 0, localtime = 0;
    uint64_t globaltime = 0;
    
    /* Act on command-line arguments */
    if (argc > 1) {
        address = std::string(argv[1]);
    }
    if (argc > 2) {
        port = std::stoi(argv[2]);
    }
    std::cout << "Sending UDP packages to: " << address << ":" << port << std::endl;

    /* Prepare and start event handling */
    std::cout << "Setup event generator" << std::endl;

    /* TODO: setup UDP sender */
    
    /* TODO: switch to real data format */
    uint32_t data[EVENTFIELDS];

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
            std::cout << "Generate fake events" << std::endl;
            /* TODO: actualy receive data here - fake for now */
            digitizerModel = "V1740D";
            digitizerID = "137";
            digitizer = "V1740D_137";
            globaltime = std::time(nullptr);

            /* Loop generating variable number of events */
            flavor = "list";
            eventsTarget = 1 + globaltime % 3;

            for (eventIndex = 0; eventIndex < eventsTarget; eventIndex++) {
                /* Create a new dataset named after event index under
                 * globaltime group if it doesn't exist already. */
                channel = (eventIndex % 2) * 31;
                localtime = (globaltime + eventIndex) & 0xFFFF;
                charge = 242 + (localtime+eventIndex*13) % 100;
                std::cout << "Sending event from " << digitizer << " channel " << channel << " localtime " << localtime << " charge " << charge << std::endl;
                data[0] = channel;
                data[1] = localtime;
                data[2] = charge;
                /* TODO: really send! */
                eventsSent += 1;
            }
            /* TODO: remove this sleep once we change to real data packages? */
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

    /* TODO: close UDP listener */

    std::cout << "Shutting down." << std::endl;

    return 0;
}
