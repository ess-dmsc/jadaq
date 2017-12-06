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

#include <getopt.h>
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
    std::cout << "Usage: " << name << " [<options>] [<jadaq_config_file>]" << std::endl;
    std::cout << "Where <options> can be:" << std::endl;
    std::cout << "--address / -a ADDRESS     optional UDP network address to send to." << std::endl;
    std::cout << "--port / -p PORT           optional UDP network port to send to." << std::endl;
    std::cout << "--confoverride / -c FILE   optional conf overrides on CAEN format." << std::endl;
    std::cout << "--dumpprefix / -d PREFIX   optional prefix for output dump to file." << std::endl;
    std::cout << "--registerdump / -r FILE   optional file to save register dump in." << std::endl;
    std::cout << std::endl << "Reads in a configuration from <jadaq_config_file> " << std::endl;
    std::cout << "and configures the digitizer(s) accordingly." << std::endl;
    std::cout << "The current/resulting digitizer settings automatically " << std::endl;
    std::cout << "gets written out to <config_file>.out ." << std::endl;
    std::cout << "After configuration the acquisition is started from all " << std::endl;
    std::cout << "configured digitizers and for each data readout the " << std::endl;
    std::cout << "events are unpacked/decoded. They can be written to disk " << std::endl;
    std::cout << "on request and sent out on the network as UDP packages if " << std::endl;
    std::cout << "both address and port are provided. Use a broadcast address" << std::endl;
    std::cout << "to target multiple listeners." << std::endl;
    std::cout << "If the optional simpledump prefix is provided the " << std::endl;
    std::cout << "individual timestamps and charges are sequentially recorded " << std::endl;
    std::cout << "to a corresponding per-channel file with digitizer and " << std::endl;
    std::cout << "channel appended (i.e. use 'output/list' for files named " << std::endl;
    std::cout << "list-DIGITIZER-charge-00.txt to ...-64.txt in 'output' dir." << std::endl;
    std::cout << "If the optional register dump file is provided a read-out " << std::endl;
    std::cout << "of the important digitizer registers is dumped there." << std::endl;
}

int main(int argc, char **argv) {
    const char* const short_opts = "a:c:d:hp:r:";
    const option long_opts[] = {
        {"address", 1, nullptr, 'a'},
        {"confoverride", 1, nullptr, 'c'},
        {"dumpprefix", 1, nullptr, 'd'},
        {"help", 0, nullptr, 'h'},
        {"port", 1, nullptr, 'p'},
        {"registerdump", 1, nullptr, 'r'},
        {nullptr, 0, nullptr, 0}
    };

    /* Default option values */
    std::string address = DEFAULT_UDP_ADDRESS, port = DEFAULT_UDP_PORT;
    std::string configFileName = "";
    std::string overrideFileName = "";
    std::string channelDumpPrefix = "";
    std::string registerDumpFileName = "";
    bool overrideEnabled = false, channelDumpEnabled = false;
    /* TODO: debug and enable wavedump */
    bool waveDumpEnabled = false, registerDumpEnabled = false;
    bool sendEventEnabled = false;

    /* Parse command line options */
    while (true) {
        const auto opt = getopt_long(argc, argv, short_opts, long_opts, nullptr);
        if (-1 == opt)
            break;

        switch (opt) {
        case 'a':
            address = std::string(optarg);
            break;
        case 'c':
            overrideFileName = std::string(optarg);
            overrideEnabled = true;
            break;
        case 'd':
            channelDumpPrefix = std::string(optarg);
            channelDumpEnabled = true;
            waveDumpEnabled = true;
            break;
        case 'p':
            port = std::string(optarg);
            break;
        case 'r':
            registerDumpFileName = std::string(optarg);
            registerDumpEnabled = true;
            break;
        case 'h': // -h or --help
        case '?': // Unrecognized option
        default:
            usageHelp(argv[0]);
            break;
        }
    }

    if (address.length() > 0 && port.length() > 0) {
            sendEventEnabled = true;
    }
    
    /* No further command-line arguments */
    if (argc - optind > 1) {
        usageHelp(argv[0]);
        exit(1);
    }
    if (argc - optind > 1) {
        configFileName = argv[optind];
    }
    

    /* Helpers */
    uint64_t fullTimeTags[MAX_CHANNELS];
    uint32_t eventsFound = 0, bytesRead = 0, eventsUnpacked = 0;
    uint32_t eventsDecoded = 0, eventsSent = 0;
    uint32_t totalEventsFound = 0, totalBytesRead = 0, totalEventsUnpacked = 0;
    uint32_t totalEventsDecoded = 0, totalEventsSent = 0;

    uint32_t eventIndex = 0, decodeChannels = 0, j = 0, k = 0;
    caen::BasicEvent basicEvent;
    caen::BasicDPPEvent basicDPPEvent;
    caen::BasicDPPWaveforms basicDPPWaveforms;
    uint32_t charge = 0, timestamp = 0, channel = 0;
    uint64_t globaltime = 0, count = 0;
    uint16_t *samples = NULL;

    /* Communication helpers */
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

    /* Read-in and write resulting digitizer configuration */
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
    
    if (channelDumpEnabled) {
        boost::filesystem::path prefix(channelDumpPrefix);
        boost::filesystem::path dir = prefix.parent_path();
        try {
            boost::filesystem::create_directories(dir);
        } catch (std::exception& e) {
            std::cerr << "WARNING: failed to create channel dump output dir " << dir << " : " << e.what() << std::endl;                
        }
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
            for (channel=0; channel<MAX_CHANNELS; channel++) {
                path.str("");
                path.clear();
                path << channelDumpPrefix << "-" << digitizer.name() << "-charge-" << std::setfill('0') << std::setw(2) << channel << ".txt";
                channelChargeWriters[channel].open(path.str(), std::ofstream::out);
            }
            if (digitizer.caenHasDPPWaveformsEnabled() && waveDumpEnabled) {
                std::cout << "Dumping individual recorded channel waveforms from " << digitizer.name() << " in files " << channelDumpPrefix << "-" << digitizer.name() << "-wave-CHANNEL.txt" << std::endl;
                channelWaveWriters = new std::ofstream[MAX_CHANNELS];
                wave_writer_map[digitizer.name()] = channelWaveWriters;
                for (channel=0; channel<MAX_CHANNELS; channel++) {
                    path.str("");
                    path.clear();
                    path << channelDumpPrefix << "-" << digitizer.name() << "-wave-" << std::setfill('0') << std::setw(2) << channel << ".txt";
                    channelWaveWriters[channel].open(path.str(), std::ofstream::out);
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
        for (channel = 0; channel < MAX_CHANNELS; channel++) {
            fullTimeTags[channel] = 0;
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
                    for (channel = 0; channel < MAX_CHANNELS; channel++) {
                        eventsFound += digitizer.caenGetPrivDPPEvents().nEvents[channel];
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
                    /* TODO: add check to make sure send_buf always fits eventData */
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
                    for (channel = 0; channel < MAX_CHANNELS; channel++) {
                        for (j = 0; j < digitizer.caenGetPrivDPPEvents().nEvents[channel]; j++) {
                            /* NOTE: we don't want to muck with underlying
                             * event type here, so we rely on the wrapped
                             * extraction and pull out timestamp, charge,
                             * etc from the resulting BasicDPPEvent. */
                            
                            basicDPPEvent = digitizer.caenExtractBasicDPPEvent(digitizer.caenGetPrivDPPEvents(), channel, j);
                            /* We use the same 4 byte range for charge as CAEN sample */
                            charge = basicDPPEvent.charge & 0xFFFF;
                            /* TODO: include timestamp high bits from Extra field? */
                            /* NOTE: timestamp is 64-bit for PHA events
                             * but we just consistently clip to 32-bit
                             * for now. */ 
                            timestamp = basicDPPEvent.timestamp & 0xFFFFFFFF;

                            /* On rollover we increment the high 32 bits*/
                            if (timestamp < (uint32_t)(fullTimeTags[channel])) {
                                fullTimeTags[channel] &= 0xFFFFFFFF00000000;
                                fullTimeTags[channel] += 0x100000000;
                            }
                            /* Always insert current timestamp in the low 32 bits */
                            fullTimeTags[channel] = fullTimeTags[channel] & 0xFFFFFFFF00000000 | timestamp & 0x00000000FFFFFFFF;
                            
                            std::cout << digitizer.name() << " channel " << channel << " event " << j << " charge " << charge << " at time " << fullTimeTags[channel] << " (" << timestamp << ")"<< std::endl;

                            if (channelDumpEnabled) {
                                /* NOTE: write in same "%16lu %8d" format as CAEN sample */
                                // TODO: which of these formats should we keep?
                                /* TODO: change to a single file per
                                 * digitizer with columns: 
                                 * globaltime localtime digtizerid channel charge
                                 */
                                channelChargeWriters = charge_writer_map[digitizer.name()];
                                //channelChargeWriters[channel] << std::setw(16) << fullTimeTags[channel] << " " << std::setw(8) << charge << std::endl;
                                //channelChargeWriters[channel] << digitizer.name() << " " << std::setw(8) << channel << " " << std::setw(16) << fullTimeTags[channel] << " " << std::setw(8) << charge << std::endl;
                                channelChargeWriters[channel] << std::setw(16) << timestamp << " " << digitizer.serial() << " " << std::setw(8) << channel << " " << std::setw(8) << charge << std::endl;
                            }
                            
                            /* Only try to decode waveforms if digitizer is actually
                             * configured to record them in the first place. */
                            if (digitizer.caenHasDPPWaveformsEnabled()) {
                                try {
                                    digitizer.caenDecodeDPPWaveforms(digitizer.caenGetPrivDPPEvents(), channel, j, digitizer.caenGetPrivDPPWaveforms());

                                    std::cout << "Decoded " << digitizer.caenDumpPrivDPPWaveforms() << " DPP event waveforms from event " << j << " on channel " << channel << " from " << digitizer.name() << std::endl;
                                    eventsDecoded += 1;
                                    if (channelDumpEnabled and waveDumpEnabled) {
                                        channelWaveWriters = wave_writer_map[digitizer.name()];
                                        /* NOTE: we don't want to muck with underlying
                                         * event type here, so we rely on the wrapped
                                         * extraction and pull out
                                         * values from the resulting
                                         * BasicDPPWaveforms. */
                                        basicDPPWaveforms = digitizer.caenExtractBasicDPPWaveforms(digitizer.caenGetPrivDPPWaveforms());
                                        for(k=0; k<basicDPPWaveforms.Ns; k++) {
                                            channelWaveWriters[k] << basicDPPWaveforms.Trace1[j];                 /* samples */
                                            channelWaveWriters[k] << 2000 + 200 * basicDPPWaveforms.DTrace1[j];  /* gate    */
                                            channelWaveWriters[k] << 1000 + 200 *basicDPPWaveforms.DTrace2[j];  /* trigger */
                                            if (basicDPPWaveforms.DTrace3 != NULL)
                                                channelWaveWriters[k] << 500 + 200 * basicDPPWaveforms.DTrace3[j];   /* trg hold off */
                                            if (basicDPPWaveforms.DTrace4 != NULL)
                                                channelWaveWriters[k] << 100 + 200 * basicDPPWaveforms.DTrace4[j];  /* overthreshold */
                                            channelWaveWriters[k] << std::endl;
                                        }
                                    }                                    
                                } catch(std::exception& e) {
                                    std::cerr << "failed to decode waveforms for event " << j << " on channel " << channel << " from " << digitizer.name() << " : " << e.what() << std::endl;
                                }
                            }                            

                            if (sendEventEnabled) {
                                std::cout << "Filling event at " << globaltime << " from " << digitizer.name() << " channel " << channel << " localtime " << timestamp << " charge " << charge << std::endl;
                                eventData->listEvents[eventIndex].localTime = timestamp;
                                eventData->listEvents[eventIndex].extendTime = 0;
                                eventData->listEvents[eventIndex].adcValue = charge;
                                eventData->listEvents[eventIndex].channel = channel;
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
                        /* TODO: where do we get real channel from in these events?!? */
                        /*       it looks like decoded events is really a matrix
                         *       of channel events a bit like in the DPPEvents case */
                        channel = 4242;
                        basicEvent = digitizer.caenExtractBasicEvent(digitizer.caenGetPrivEventInfo(), digitizer.caenGetPrivEvent(), channel, eventIndex);
                        timestamp = basicEvent.timestamp;
                        count = basicEvent.count;
                        samples = basicEvent.samples;
                        if (sendEventEnabled) {
                            std::cout << "Filling event at " << globaltime << " from " << digitizer.name() << " channel " << channel << " localtime " << timestamp << " sample count " << count << std::endl;
                            eventData->waveformEvents[eventIndex].localTime = timestamp;
                            eventData->waveformEvents[eventIndex].waveformLength = count;
                            memcpy(eventData->waveformEvents[eventIndex].waveform, samples, (count * sizeof(samples[0])));
                            eventData->waveformEvents[eventIndex].channel = channel;
                        }   
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
            for (channel=0; channel<MAX_CHANNELS; channel++) {
                channelChargeWriters[channel].close();
            }
            charge_writer_map.erase(digitizer.name());
            delete[] channelChargeWriters;
        }

        if (digitizer.caenIsDPPFirmware()) {
            if (digitizer.caenHasDPPWaveformsEnabled()) {
                if (channelDumpEnabled && waveDumpEnabled) {
                    std::cout << "Closing channel wave dump files for " << digitizer.name() << std::endl;
                    channelWaveWriters = wave_writer_map[digitizer.name()];
                    for (channel=0; channel<MAX_CHANNELS; channel++) {
                        channelWaveWriters[channel].close();
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
