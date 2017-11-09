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
    std::vector<Digitizer> digitizers;
    uint32_t numEvents = 0;
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
        /* Extract a vector of all configured digitizers for later */
        digitizers = configuration.getDigitizers();
    }
    configFile.close();

    std::cout << "Starting acquisition - Ctrl-C to interrupt" << std::endl;

    /* Start acquisition for all digitizers */
    std::cout << "Start acquisition from all " << digitizers.size() << " digitizers." << std::endl;
    for (Digitizer& digitizer: digitizers)
    {
        std::cout << "Prepare readout buffer for digitizer " << digitizer.name() << std::endl;
        digitizer.caenMallocPrivReadoutBuffer();
        /* We need to allocate additional space for events - format
         * depends on whether digitizer runs DPP or not */
        if (digitizer.caenIsDPPFirmware()) {
            std::cout << "Prepare DPP event buffer for digitizer " << digitizer.name() << std::endl;
            digitizer.caenMallocPrivDPPEvents();
            std::cout << "Prepare DPP waveforms buffer for digitizer " << digitizer.name() << std::endl;
            digitizer.caenMallocPrivDPPWaveforms();
            std::cout << "Allocated DPP waveforms buffer: " << digitizer.caenDumpPrivDPPWaveforms() << std::endl;
        } else {
            std::cout << "Prepare event buffer for digitizer " << digitizer.name() << std::endl;
            digitizer.caenMallocPrivEvent();
        }
        std::cout << "Start acquisition on digitizer " << digitizer.name() << std::endl;
        digitizer.caenStartAcquisition();
    }

    /* Set up interrupt handler and start handling acquired data */
    setup_interrupt_handler();

    uint32_t throttleDown = 100;
    while(true) {
        /* Continuously acquire and process data:
         *   - read out data
         *   - decode data
         *   - pack data
         *   - send out on UDP
         */
        if (throttleDown) {
            /* NOTE: for running without hogging CPU if nothing to do */
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            throttleDown = 100;
        }
        try {
            std::cout << "Read out data from all " << digitizers.size() << " digitizers." << std::endl;
            /* Read out acquired data for all digitizers */
            for (Digitizer& digitizer: digitizers)
            {
                /* TODO: check and skip if there's no data to read? */
                std::cout << "Read at most " << digitizer.caenGetPrivReadoutBuffer().size << "b data from digitizer " << digitizer.name() << std::endl;
                digitizer.caenReadData(digitizer.caenGetPrivReadoutBuffer());
                std::cout << "Read " << digitizer.caenGetPrivReadoutBuffer().dataSize << "b of acquired data" << std::endl;
                if (digitizer.caenIsDPPFirmware()) {
                    if (digitizer.caenGetPrivReadoutBuffer().dataSize > 0) {
                        digitizer.caenGetDPPEvents(digitizer.caenGetPrivReadoutBuffer(), digitizer.caenGetPrivDPPEvents());
                        std::cout << "Unpacked " << digitizer.caenGetPrivDPPEvents().nEvents[0] << " DPP events from channel 0." << std::endl;
                        digitizer.caenDecodeDPPWaveforms(digitizer.caenGetPrivDPPEvents(), 0, 0, digitizer.caenGetPrivDPPWaveforms());
                        std::cout << "Decoded DPP waveforms" << std::endl;
                        std::cout << "Decoded " << digitizer.caenDumpPrivDPPWaveforms() << " DPP event waveforms from first event on channel 0." << std::endl;
                    /* TODO: pack and send out UDP */
                    } else {
                        std::cout << "No events found - no further handling." << std::endl;
                    }
                } else {
                    numEvents = digitizer.caenGetNumEvents(digitizer.caenGetPrivReadoutBuffer());
                    std::cout << "Acquired data contains  " << numEvents << " events." << std::endl;
                    for (uint32_t eventNumber=0; eventNumber < numEvents; eventNumber++) {
                        digitizer.caenGetEventInfo(digitizer.caenGetPrivReadoutBuffer(), eventNumber);
                        std::cout << "Unpacked event " << digitizer.caenGetPrivEventInfo().EventCounter << "  of " << numEvents << " events." << std::endl;
                        digitizer.caenDecodeEvent(digitizer.caenGetPrivEventInfo(), digitizer.caenGetPrivEvent());
                        std::cout << "Decoded event " << digitizer.caenGetPrivEventInfo().EventCounter << "  of " << numEvents << " events." << std::endl;
                    }
                    if (numEvents < 1) {
                        std::cout << "No events found - no further handling." << std::endl;
                        throttleDown = 1000;
                        continue;
                    }
                    
                    /* TODO: pack and send out UDP */
                }
                throttleDown = 100;
            }
        } catch(std::exception& e) {
            std::cerr << "unexpected exception during acquisition: " << e.what() << std::endl;
            /* NOTE: throttle down on errors */
            throttleDown = 2000;
        }
        if (interrupted) {
            std::cout << "caught interrupt - stop acquisition." << std::endl;
            /* Stop acquisition for all digitizers */
            std::cout << "Stop acquisition from all " << digitizers.size() << " digitizers." << std::endl;
            for (Digitizer& digitizer: digitizers)
            {
                std::cout << "Stop acquisition on digitizer " << digitizer.name() << std::endl;
                digitizer.caenStopAcquisition();
                if (digitizer.caenIsDPPFirmware()) {
                    std::cout << "Free DPP event buffer for " << digitizer.name() << std::endl;
                    digitizer.caenFreePrivDPPEvents();
                    std::cout << "Free DPP waveforms buffer for " << digitizer.name() << std::endl;
                    digitizer.caenFreePrivDPPWaveforms();
                } else {
                    std::cout << "Free event buffer for " << digitizer.name() << std::endl;
                    digitizer.caenFreePrivEvent();
                }
                std::cout << "Free readout buffer for " << digitizer.name() << std::endl;
                digitizer.caenFreePrivReadoutBuffer();
            }
            break;
        }
    }
    std::cout << "Acquisition complete - shutting down." << std::endl;

    /* Clean up after run */

    return 0;
}
