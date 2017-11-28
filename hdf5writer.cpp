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
 * A simple daemon listening for data and writing it to HDF5 file.
 *
 */

#include <signal.h>
#include <chrono>
#include <thread>
#include <iostream>
#include <fstream>
#include <string>
#include "H5Cpp.h"

using namespace H5;

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
    if (argc > 1)
    {
        std::cout << "Usage: " << argv[0] << " [<config_file>]" << std::endl;
        std::cout << "Reads in a partial/full configuration in <config_file> " << std::endl;
        std::cout << "and configures the hdf5writer accordingly." << std::endl;
        return -1;
    }

    /* Helpers */
    uint32_t eventsWritten = 0;

    uint32_t eventIndex = 0, i = 0, j = 0;

    /* Path helpers */
    std::string configFileName;
    std::string outputFileName;
    
    /* Read-in and write resulting digitizer configuration */
    if (argc > 1) {
        configFileName = std::string(argv[1]);
        std::cout << "Reading hdf5writer configuration from" << configFileName << std::endl;
    } else {
        std::cout << "Using default hdf5writer configuration." << std::endl;
    }

    /* Prepare and start event handling */
    std::cout << "Setup hdf5writer" << std::endl;

    /* Set up interrupt handler and start handling acquired data */
    setup_interrupt_handler();

    std::cout << "Running file writer loop - Ctrl-C to interrupt" << std::endl;

    uint32_t throttleDown = 100;
    bool keepRunning = true;
    while(keepRunning) {
        /* Continuously receive and dump data */
        if (throttleDown) {
            /* NOTE: for running without hogging CPU if nothing to do */
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            throttleDown = 100;
        }
        try {
            std::cout << "Receive data" << std::endl;
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

    /* Stop file writer */
    std::cout << "Stop file writer" << std::endl;
    
    /* Clean up after all */
    std::cout << "Clean up after file writer" << std::endl;

    std::cout << "Shutting down." << std::endl;

    return 0;
}
