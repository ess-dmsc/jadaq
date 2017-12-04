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
#include <iomanip>
#include <fstream>
#include <map>
#include "Configuration.hpp"
#include "CAENConf.hpp"
#include <boost/filesystem.hpp>
#include <boost/asio.hpp>
#include <boost/algorithm/string/regex.hpp>

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

int main(int argc, char **argv) {
    if (argc < 2 or argc > 6)
    {
        std::cout << "Usage: " << argv[0] << " <jadaq_config_file> [<override_config_file>] [<send_events_to>] [simple_output_file_prefix] [<register_dump_file>]" << std::endl;
        std::cout << "Reads in a partial/full configuration in <jadaq_config_file> " << std::endl;
        std::cout << "and configures the digitizer(s) accordingly." << std::endl;
        std::cout << "The current/resulting digitizer settings automatically " << std::endl;
        std::cout << "gets written out to <config_file>.out ." << std::endl;
        std::cout << "If the optional <override_config_file> on the CAEN sample " << std::endl;
        std::cout << "format is provided, any options there will be overriden." << std::endl;
        std::cout << "If the optional <send_events_to> address:port string is " << std::endl;
        std::cout << "provided all acquired events are sent on UDP there as well." << std::endl;
        std::cout << "If the optional <simple_output_file_prefix> is provided the " << std::endl;
        std::cout << "individual timestamps and charges are sequentially recorded " << std::endl;
        std::cout << "to a corresponding per-channel file with digitizer and " << std::endl;
        std::cout << "channel appended (i.e. use 'output/list' for files named " << std::endl;
        std::cout << "list-DIGITIZER-charge-00.txt to ...-64.txt in 'output' dir." << std::endl;
        std::cout << "If the optional <register_dump_file> is provided a read " << std::endl;
        std::cout << "out of the important digitizer registers is dumped there." << std::endl;
        std::cout << "After configuration the acquisition is started from all " << std::endl;
        std::cout << "configured digitizers and for each data readout the " << std::endl;
        std::cout << "events are unpacked/decoded and sent out on UDP." << std::endl;
        return -1;
    }

    /* Helpers */
    uint64_t fullTimeTags[MAX_CHANNELS];
    uint32_t eventsFound = 0, bytesRead = 0, eventsUnpacked = 0;
    uint32_t eventsDecoded = 0, eventsSent = 0;
    uint32_t totalEventsFound = 0, totalBytesRead = 0, totalEventsUnpacked = 0;
    uint32_t totalEventsDecoded = 0, totalEventsSent = 0;

    uint32_t eventIndex = 0, decodeChannels = 0, i = 0, j = 0, k = 0;
    caen::BasicDPPEvent basicEvent;
    caen::BasicDPPWaveforms basicWaveforms;
    uint32_t charge = 0, timestamp = 0;
    uint64_t globaltime = 0;

    /* Communication helpers */
    std::string address = "127.0.0.1", port = "12345";
    boost::asio::io_service io_service;
    udp::endpoint receiver_endpoint;
    udp::socket *socket = NULL;
    /* NOTE: use a static buffer of MAXBUFSIZE bytes for sending */
    char send_buf[MAXBUFSIZE];
    Data::EventData *eventData;
    Data::Meta *metadata;
    Data::PackedEvents packedEvents;

    /* Active digitizers */
    std::vector<Digitizer> digitizers;

    /* Path helpers */
    std::string configFileName;
    std::string overrideFileName;
    std::string channelDumpPrefix;
    std::string registerDumpFileName;
    
    /* Read-in and write resulting digitizer configuration */
    configFileName = std::string(argv[1]);
    std::ifstream configFile(configFileName);
    if (!configFile.good())
    {
        std::cerr << "Could not open jadaq configuration file: " << configFileName << std::endl;
        /* NOTE: fall back to hard coded digitizer for sample setup */
        digitizers.push_back(Digitizer(0,0x11130000));
    } else {
        std::cout << "Reading digitizer configuration from" << configFileName << std::endl;
        /* NOTE: switch verbose (2nd) arg on here to enable conf warnings */ 
        Configuration configuration(configFile, false);
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

        // Extract a vector of all configured digitizers for later
        digitizers = configuration.getDigitizers();
    }
    configFile.close();    

    /* Helpers for output - eventually move to asio receiver */
    std::ofstream *channelChargeWriters, *channelWaveWriters;
    typedef std::map<const std::string, std::ofstream *> ofstream_map;
    ofstream_map charge_writer_map;
    ofstream_map wave_writer_map;
    std::stringstream path;
    bool overrideEnabled = false, channelDumpEnabled = false;
    /* TODO: debug and enable wavedump */
    bool waveDumpEnabled = false, registerDumpEnabled = false;
    bool sendEventEnabled = false;
    
    if (argc > 2 && strlen(argv[2]) > 0) {
        overrideEnabled = true;
        overrideFileName = std::string(argv[2]);
    }
    if (argc > 3 && strlen(argv[3])) {
        sendEventEnabled = true;
        std::vector <std::string> parts;
        boost::algorithm::split_regex(parts, argv[3], boost::regex(":"));
        address = parts[0];
        port = parts[1];
    }
    if (argc > 4 && strlen(argv[4])) {
        channelDumpEnabled = true;
        channelDumpPrefix = std::string(argv[4]);
        boost::filesystem::path prefix(channelDumpPrefix);
        boost::filesystem::path dir = prefix.parent_path();
        try {
            boost::filesystem::create_directories(dir);
        } catch (std::exception& e) {
            std::cerr << "WARNING: failed to create channel dump output dir " << dir << " : " << e.what() << std::endl;                
        }
    }
    if (argc > 5 && strlen(argv[5])) {
        registerDumpEnabled = true;
        registerDumpFileName = std::string(argv[5]);
    }

    /* Prepare and start acquisition for all digitizers */
    std::cout << "Setup acquisition from " << digitizers.size() << " digitizer(s)." << std::endl;
    for (Digitizer& digitizer: digitizers)
    {
        /* NOTE: apply overrides from provided CAEN config. 
         *       this can be used to mimic CAEN QDC sample.
         */
        if (overrideEnabled) {
            std::cout << "Override " << digitizer.name() << " configuration with " << overrideFileName << std::endl;
            BoardParameters params;
            if (setup_parameters(&params, (char *)overrideFileName.c_str()) < 0) {
                std::cerr << "Error in setup parameters from " << overrideFileName << std::endl;
            } else if (configure_digitizer(digitizer.caen()->handle(), digitizer.caen(), &params) < 0) {
                std::cerr << "Error in configuring digitizer with overrides from " << overrideFileName << std::endl;
            }
        }

        /* Record timestamps and charges in per-channel files on request */
        if (channelDumpEnabled) {
            std::cout << "Dumping individual recorded channel charges from " << digitizer.name() << " in files " << channelDumpPrefix << "-" << digitizer.name() << "-charge-CHANNEL.txt" << std::endl;
            channelChargeWriters = new std::ofstream[MAX_CHANNELS];
            charge_writer_map[digitizer.name()] = channelChargeWriters;
            for (int i=0; i<MAX_CHANNELS; i++) { 
                path.str("");
                path.clear();
                path << channelDumpPrefix << "-" << digitizer.name() << "-charge-" << std::setfill('0') << std::setw(2) << i << ".txt";
                channelChargeWriters[i].open(path.str(), std::ofstream::out);
            }
            if (digitizer.caenHasDPPWaveformsEnabled() && waveDumpEnabled) {
                std::cout << "Dumping individual recorded channel waveforms from " << digitizer.name() << " in files " << channelDumpPrefix << "-" << digitizer.name() << "-wave-CHANNEL.txt" << std::endl;
                channelWaveWriters = new std::ofstream[MAX_CHANNELS];
                wave_writer_map[digitizer.name()] = channelWaveWriters;
                for (int i=0; i<MAX_CHANNELS; i++) { 
                    path.str("");
                    path.clear();
                    path << channelDumpPrefix << "-" << digitizer.name() << "-wave-" << std::setfill('0') << std::setw(2) << i << ".txt";
                    channelWaveWriters[i].open(path.str(), std::ofstream::out);
                }
                
            }
        }
        if (sendEventEnabled) {
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
        }

        /* Dump actual digitizer registers for debugging on request */
        if (registerDumpEnabled) {
            std::cout << "Dumping digitizer " << digitizer.name() << " registers in " << registerDumpFileName << std::endl;
            if (dump_configuration(digitizer.caen()->handle(), (char *)registerDumpFileName.c_str()) < 0) {
                std::cerr << "Error in dumping digitizer registers in " << registerDumpFileName << std::endl;
                exit(1); 
            }
        }

        /* Init helpers */
        for (i = 0; i < MAX_CHANNELS; i++) {
            fullTimeTags[i] = 0;
        }

        /* Prepare buffers - must happen AFTER digitizer has been configured! */
        std::cout << "Prepare readout buffer for digitizer " << digitizer.name() << std::endl;
        digitizer.caenMallocPrivReadoutBuffer();

        /* We need to allocate additional space for events - format
         * depends on whether digitizer runs DPP or not */
        if (digitizer.caenIsDPPFirmware()) {
            std::cout << "Prepare DPP event buffer for digitizer " << digitizer.name() << std::endl;
            digitizer.caenMallocPrivDPPEvents();

            if (digitizer.caenHasDPPWaveformsEnabled()) {
                std::cout << "Prepare DPP waveforms buffer for digitizer " << digitizer.name() << std::endl;
                digitizer.caenMallocPrivDPPWaveforms();
                std::cout << "Allocated DPP waveforms buffer: " << digitizer.caenDumpPrivDPPWaveforms() << std::endl;
            }
        } else {
            std::cout << "Prepare event buffer for digitizer " << digitizer.name() << std::endl;
            digitizer.caenMallocPrivEvent();
        }
    }

    std::cout << "Start acquisition from " << digitizers.size() << " digitizer(s)." << std::endl;
    for (Digitizer& digitizer: digitizers) {
        std::cout << "Start acquisition on digitizer " << digitizer.name() << std::endl;
        digitizer.caenStartAcquisition();
    }

    /* Set up interrupt handler and start handling acquired data */
    setup_interrupt_handler();

    std::cout << "Running acquisition loop - Ctrl-C to interrupt" << std::endl;

    uint32_t throttleDown = 0;
    bool keepRunning = true;
    while(keepRunning) {
        /* Continuously acquire and process data:
         *   - read out data
         *   - decode data
         *   - pack data
         *   - send out on UDP
         */
        if (throttleDown > 0) {
            /* NOTE: for running without hogging CPU if nothing to do */
            std::this_thread::sleep_for(std::chrono::milliseconds(throttleDown));
        }
        try {
            std::cout << "Read out data from " << digitizers.size() << " digitizer(s)." << std::endl;
            /* Read out acquired data for all digitizers */
            /* TODO: split digitizer handling into threads */
            for (Digitizer& digitizer: digitizers)
            {
                /* NOTE: check and skip if there's no events to read */
                std::cout << "Read at most " << digitizer.caenGetPrivReadoutBuffer().size << "b data from " << digitizer.name() << std::endl;
                digitizer.caenReadData(digitizer.caenGetPrivReadoutBuffer());
                bytesRead = digitizer.caenGetPrivReadoutBuffer().dataSize;
                totalBytesRead += bytesRead;
                std::cout << "Read " << bytesRead << "b of acquired data" << std::endl;

                globaltime = std::time(nullptr);

                if (digitizer.caenIsDPPFirmware()) {
                    std::cout << "Unpack aggregated DPP events from " << digitizer.name() << std::endl;
                    digitizer.caenGetDPPEvents(digitizer.caenGetPrivReadoutBuffer(), digitizer.caenGetPrivDPPEvents());
                    eventsFound = 0;
                    for (i = 0; i < MAX_CHANNELS; i++) {
                        eventsFound += digitizer.caenGetPrivDPPEvents().nEvents[i];
                    }
                    eventsUnpacked = eventsFound;
                    if (eventsFound < 1)
                        continue;
                    totalEventsUnpacked += eventsUnpacked;
                    totalEventsFound += eventsFound;
                    std::cout << "Unpacked " << eventsUnpacked << " DPP events from all channels." << std::endl;
                } else {
                    eventsFound = digitizer.caenGetNumEvents(digitizer.caenGetPrivReadoutBuffer());
                    std::cout << "Acquired data from " << digitizer.name() << " contains " << eventsFound << " event(s)." << std::endl;
                    if (eventsFound < 1) {
                        std::cout << "No events found - no further handling." << std::endl;
                        throttleDown = std::min((uint32_t)2000, 2*throttleDown + 100);
                        continue;
                    }
                    totalEventsFound += eventsFound;
                    eventsUnpacked = 0;
                }

                if (sendEventEnabled) {
                    /* Reset send buffer each time to prevent any stale data */
                    memset(send_buf, 0, MAXBUFSIZE);
                    eventData = Data::setupEventData((void *)send_buf, MAXBUFSIZE, eventsFound, 0);
                    std::cout << "Prepared eventData " << eventData << " from send_buf " << (void *)send_buf << std::endl;
                    metadata = eventData->metadata;
                    /* NOTE: safe copy with explicit string termination */
                    strncpy(eventData->metadata->digitizerModel, digitizer.model().c_str(), MAXMODELSIZE);
                    eventData->metadata->digitizerModel[MAXMODELSIZE-1] = '\0';
                    eventData->metadata->digitizerID = std::stoi(digitizer.serial());
                    eventData->metadata->globalTime = globaltime;
                    std::cout << "Prepared eventData has " << eventData->listEventsLength << " listEvents " << std::endl;
                }

                if (digitizer.caenIsDPPFirmware()) {
                    eventIndex = 0;
                    eventsDecoded = 0;
                    for (i = 0; i < MAX_CHANNELS; i++) {
                        for (j = 0; j < digitizer.caenGetPrivDPPEvents().nEvents[i]; j++) {
                            /* NOTE: we don't want to muck with underlying
                             * event type here, so we rely on the wrapped
                             * extraction and pull out timestamp, charge,
                             * etc from the resulting BasicDPPEvent. */
                            
                            basicEvent = digitizer.caenExtractBasicDPPEvent(digitizer.caenGetPrivDPPEvents(), i, j);
                            /* We use the same 4 byte range for charge as CAEN sample */
                            charge = basicEvent.charge & 0xFFFF;
                            /* TODO: include timestamp high bits from Extra field? */
                            /* NOTE: timestamp is 64-bit for PHA events
                             * but we just consistently clip to 32-bit
                             * for now. */ 
                            timestamp = basicEvent.timestamp & 0xFFFFFFFF;

                            /* On rollover we increment the high 32 bits*/
                            if (timestamp < (uint32_t)(fullTimeTags[i])) {
                                fullTimeTags[i] &= 0xFFFFFFFF00000000;
                                fullTimeTags[i] += 0x100000000;
                            }
                            /* Always insert current timestamp in the low 32 bits */
                            fullTimeTags[i] = fullTimeTags[i] & 0xFFFFFFFF00000000 | timestamp & 0x00000000FFFFFFFF;
                            
                            std::cout << digitizer.name() << " channel " << i << " event " << j << " charge " << charge << " at time " << fullTimeTags[i] << " (" << timestamp << ")"<< std::endl;

                            if (channelDumpEnabled) {
                                /* NOTE: write in same "%16lu %8d" format as CAEN sample */
                                // TODO: which of these formats should we keep?
                                /* TODO: change to a single file per
                                 * digitizer with columns: 
                                 * globaltime localtime digtizerid channel charge
                                 */
                                channelChargeWriters = charge_writer_map[digitizer.name()];
                                //channelChargeWriters[i] << std::setw(16) << fullTimeTags[i] << " " << std::setw(8) << charge << std::endl;
                                //channelChargeWriters[i] << digitizer.name() << " " << std::setw(8) << i << " " << std::setw(16) << fullTimeTags[i] << " " << std::setw(8) << charge << std::endl;
                                channelChargeWriters[i] << std::setw(16) << timestamp << " " << digitizer.serial() << " " << std::setw(8) << i << " " << std::setw(8) << charge << std::endl;
                            }
                            
                            /* Only try to decode waveforms if digitizer is actually
                             * configured to record them in the first place. */
                            if (digitizer.caenHasDPPWaveformsEnabled()) {
                                try {
                                    digitizer.caenDecodeDPPWaveforms(digitizer.caenGetPrivDPPEvents(), i, j, digitizer.caenGetPrivDPPWaveforms());

                                    std::cout << "Decoded " << digitizer.caenDumpPrivDPPWaveforms() << " DPP event waveforms from event " << j << " on channel " << i << " from " << digitizer.name() << std::endl;
                                    eventsDecoded += 1;
                                    if (channelDumpEnabled and waveDumpEnabled) {
                                        channelWaveWriters = wave_writer_map[digitizer.name()];
                                        /* NOTE: we don't want to muck with underlying
                                         * event type here, so we rely on the wrapped
                                         * extraction and pull out
                                         * values from the resulting
                                         * BasicDPPWaveforms. */
                                        basicWaveforms = digitizer.caenExtractBasicDPPWaveforms(digitizer.caenGetPrivDPPWaveforms());
                                        for(k=0; k<basicWaveforms.Ns; k++) {
                                            channelWaveWriters[k] << basicWaveforms.Trace1[j];                 /* samples */
                                            channelWaveWriters[k] << 2000 + 200 * basicWaveforms.DTrace1[j];  /* gate    */
                                            channelWaveWriters[k] << 1000 + 200 *basicWaveforms.DTrace2[j];  /* trigger */
                                            if (basicWaveforms.DTrace3 != NULL)
                                                channelWaveWriters[k] << 500 + 200 * basicWaveforms.DTrace3[j];   /* trg hold off */
                                            if (basicWaveforms.DTrace4 != NULL)
                                                channelWaveWriters[k] << 100 + 200 * basicWaveforms.DTrace4[j];  /* overthreshold */
                                            channelWaveWriters[k] << std::endl;
                                        }
                                    }                                    
                                } catch(std::exception& e) {
                                    std::cerr << "failed to decode waveforms for event " << j << " on channel " << i << " from " << digitizer.name() << " : " << e.what() << std::endl;
                                }
                            }                            

                            if (sendEventEnabled) {
                                std::cout << "Filling event at " << globaltime << " from " << digitizer.name() << " channel " << i << " localtime " << timestamp << " charge " << charge << std::endl;
                                eventData->listEvents[eventIndex].localTime = timestamp;
                                eventData->listEvents[eventIndex].extendTime = 0;
                                eventData->listEvents[eventIndex].adcValue = charge;
                                eventData->listEvents[eventIndex].channel = i;
                            }
                            eventIndex += 1;
                        }
                    }
                } else { 
                    /* Handle the non-DPP case */
                    eventsDecoded = 0;
                    for (eventIndex=0; eventIndex < eventsFound; eventIndex++) {
                        digitizer.caenGetEventInfo(digitizer.caenGetPrivReadoutBuffer(), eventIndex);
                        std::cout << "Unpacked event " << digitizer.caenGetPrivEventInfo().EventCounter << "  of " << eventsFound << " events from " << digitizer.name() << std::endl;
                        eventsUnpacked += 1;
                        digitizer.caenDecodeEvent(digitizer.caenGetPrivEventInfo(), digitizer.caenGetPrivEvent());
                        std::cout << "Decoded event " << digitizer.caenGetPrivEventInfo().EventCounter << "  of " << eventsFound << " events from " << digitizer.name() << std::endl;
                        eventsDecoded += 1;
                        /* TODO: enable something like the following */
                        /*
                        basicEvent = digitizer.caenExtractBasicEvent(digitizer.caenGetPrivEventInfo());
                        timestamp = basicEvent.timestamp & 0xFFFFFFFF;
                        charge = basicEvent.charge;
                        channel = basicEvent.channel;
                        if (sendEventEnabled) {
                            std::cout << "Filling event at " << globaltime << " from " << digitizer.name() << " channel " << i << " localtime " << timestamp << " charge " << charge << std::endl;
                            eventData->listEvents[eventIndex].localTime = timestamp;
                            eventData->listEvents[eventIndex].extendTime = 0;
                            eventData->listEvents[eventIndex].adcValue = charge;
                            eventData->listEvents[eventIndex].channel = i;
                        }   
                        */
                    }
                    totalEventsUnpacked += eventsUnpacked;
                    totalEventsDecoded += eventsDecoded;
                }

                    
                /* Pack and send out UDP */
                eventsSent = 0;
                if (sendEventEnabled) {
                    std::cout << "Packing events at " << globaltime << " from " << digitizer.name() << std::endl;
                    packedEvents = Data::packEventData(eventData, eventsFound, 0);
                    /* Send data to preconfigured receiver */
                    std::cout << "Sending " << eventsUnpacked << " packed events of " << packedEvents.dataSize << "b at " << globaltime << " from " << digitizer.name() << " to " << address << ":" << port << std::endl;
                    socket->send_to(boost::asio::buffer((char*)(packedEvents.data), packedEvents.dataSize), receiver_endpoint);
                    eventsSent += eventsUnpacked;
                }
                totalEventsSent += eventsSent;

                throttleDown = 0;
            }
            std::cout << "= Accumulated Stats =" << std::endl;
            std::cout << "Bytes read: " << totalBytesRead << std::endl;
            std::cout << "Aggregated events found: " << totalEventsFound << std::endl;
            std::cout << "Individual events unpacked: " << totalEventsUnpacked << std::endl;
            std::cout << "Individual events decoded: " << totalEventsDecoded << std::endl;
            std::cout << "Individual events sent: " << totalEventsSent << std::endl;

        } catch(std::exception& e) {
            std::cerr << "unexpected exception during acquisition: " << e.what() << std::endl;
            /* NOTE: throttle down on errors */
            throttleDown = 2000;
        }
        if (interrupted) {
            std::cout << "caught interrupt - stop acquisition and clean up." << std::endl;
            break;
        }
    }

    /* Stop acquisition for all digitizers */
    std::cout << "Stop acquisition from " << digitizers.size() << " digitizer(s)." << std::endl;
    for (Digitizer& digitizer: digitizers)
    {
        std::cout << "Stop acquisition on digitizer " << digitizer.name() << std::endl;
        digitizer.caenStopAcquisition();
    }
    /* Clean up after all digitizers: buffers, etc. */
    std::cout << "Clean up after " << digitizers.size() << " digitizer(s)." << std::endl;
    for (Digitizer& digitizer: digitizers)
    {
        if (channelDumpEnabled) {
            std::cout << "Closing channel charge dump files for " << digitizer.name() << std::endl;
            channelChargeWriters = charge_writer_map[digitizer.name()];
            for (int i=0; i<MAX_CHANNELS; i++) { 
                channelChargeWriters[i].close();
            }
            charge_writer_map.erase(digitizer.name());
            delete[] channelChargeWriters;
        }

        if (digitizer.caenIsDPPFirmware()) {
            if (digitizer.caenHasDPPWaveformsEnabled()) {
                if (channelDumpEnabled && waveDumpEnabled) {
                    std::cout << "Closing channel wave dump files for " << digitizer.name() << std::endl;
                    channelWaveWriters = wave_writer_map[digitizer.name()];
                    for (int i=0; i<MAX_CHANNELS; i++) { 
                        channelWaveWriters[i].close();
                    }
                    wave_writer_map.erase(digitizer.name());
                    delete[] channelWaveWriters;
                }
                std::cout << "Free DPP waveforms buffer for " << digitizer.name() << std::endl;
                digitizer.caenFreePrivDPPWaveforms();
            }
            std::cout << "Free DPP event buffer for " << digitizer.name() << std::endl;
            digitizer.caenFreePrivDPPEvents();            
        } else {
            std::cout << "Free event buffer for " << digitizer.name() << std::endl;
            digitizer.caenFreePrivEvent();
        }
        std::cout << "Free readout buffer for " << digitizer.name() << std::endl;
        digitizer.caenFreePrivReadoutBuffer();
    }
    for (Digitizer& digitizer: digitizers)
    {
        std::cout << "Close digitizer " << digitizer.name() << std::endl;
        delete digitizer.caen();
    }

    std::cout << "Acquisition complete - shutting down." << std::endl;

    /* Clean up after run */

    return 0;
}
