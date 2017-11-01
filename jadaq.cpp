/**
 * jadaq (Just Another DAQ)
 * Copyright (C) 2017  Troels Blum <troels@blum.dk>
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
 * Full acquisition control flow to intialize digitizer, acquire data
 * and send it out on UDP.
 *
 */

#include <signal.h>
#include <chrono>
#include <thread>
#include <iostream>
#include <fstream>
#include "Configuration.hpp"

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
    if (argc != 2)
    {
        std::cout << "Usage: " << argv[0] << " <config_file> " << std::endl;
        std::cout << "Reads in a partial/full configuration in <config_file> " << std::endl;
        std::cout << "and writes out the current/resulting digitizer settings " << std::endl;
        std::cout << "in <config_file>.out ." << std::endl;
        std::cout << "Then sets up the acquisition from the digitizer and " << std::endl;
        std::cout << "data readout, decoding and delivery on UDP." << std::endl;
        return -1;
    }

    /*  Read-in and write resulting digitizer configuration */
    std::string configFileName(argv[1]);
    std::ifstream configFile(configFileName);
    if (!configFile.good())
    {
        std::cerr << "Could not open configuration file: " << configFileName << std::endl;
    } else {
        std::cout << "Reading digitizer configuration from" << configFileName << std::endl;
        Configuration configuration(configFile);
        std::string outFileName = configFileName+".out";
        std::ofstream outFile(outFileName);
        if (!outFile.good())
        {
            std::cerr << "Unable to open output file: " << outFileName << std::endl;
        } else {
            std::cout << "Writing current digitizer configuration to " << outFileName << std::endl;
            configuration.write(outFile);
            outFile.close();
        }
    }
    configFile.close();

    std::cout << "Starting acquisition - Ctrl-C to interrupt" << std::endl;

    /* TODO: Start acquisition from digitizer */

    /* Set up interrupt handler and start handling acquired data */
    setup_interrupt_handler();
    while(true) {
        try {
            /* TODO: Continuously handle acquired data:
             *       - read out data
             *       - decode data
             *       - pack data
             *       - send out on UDP
             */

            // TMP: for testing without hogging CPU
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        } catch(std::exception& e) {
            std::cerr << "unexpected exception during acquisition: " << e.what() << std::endl;
            /* NOTE: throttle down on errors */
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        if (interrupted) {
            std::cout << "caught interrupt - stop acquisition." << std::endl;
            break;
        }
    }
    std::cout << "Acquisition complete - shutting down." << std::endl;

    /* Clean up after run */

    return 0;
}
