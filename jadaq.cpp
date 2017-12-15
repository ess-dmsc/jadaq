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
#include <atomic>
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

/* A simple helper to get current time since epoch in milliseconds */
#define getTimeMsecs() (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count())


/* Shared runtime configuration */
struct RuntimeConf {
    std::string address;
    std::string port;
    std::string configFileName;
    std::string overrideFileName;
    std::string channelDumpPrefix;
    std::string registerDumpFileName;
    bool overrideEnabled;
    bool channelDumpEnabled;
    bool waveDumpEnabled;
    bool registerDumpEnabled;
    bool sendEventEnabled;
    uint32_t stopAfterEvents;
    uint32_t stopAfterSeconds;
    uint32_t workerThreads;
};

/* Per-readout stats */
struct LocalStats {
    uint32_t bytesRead = 0; 
    uint32_t eventsFound = 0;
    uint32_t eventsUnpacked = 0;
    uint32_t eventsDecoded = 0; 
    uint32_t eventsSent = 0;
};

/* Use atomic to make sure updates from different threads won't cause races */
struct TotalStats {
    std::atomic<uint32_t> bytesRead;
    std::atomic<uint32_t> runMilliseconds;
    std::atomic<uint32_t> eventsFound;
    std::atomic<uint32_t> eventsUnpacked;
    std::atomic<uint32_t> eventsDecoded;
    std::atomic<uint32_t> eventsSent;
};

/* Per-digitizer communication helpers */
struct CommHelper {
    boost::asio::io_service sendIOService;
    udp::endpoint remoteEndpoint;
    udp::socket *socket = NULL;
    /* NOTE: use a static buffer of MAXBUFSIZE bytes for sending */
    char sendBuf[MAXBUFSIZE];
    Data::EventData *eventData;
    Data::Meta *metadata;
    Data::PackedEvents packedEvents;
};

/* Keep running marker and interrupt signal handler */
static int interrupted = 0;
static void interrupt_handler(int s) {
    interrupted = 1;
}
static void setup_interrupt_handler() {
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
    std::cout << "--address / -a ADDRESS     optional UDP network address to send to (unset by default)." << std::endl;
    std::cout << "--port / -p PORT           optional UDP network port to send to (default is " << DEFAULT_UDP_PORT << ")." << std::endl;
    std::cout << "--confoverride / -c FILE   optional conf overrides on CAEN format." << std::endl;
    std::cout << "--dumpprefix / -d PREFIX   optional prefix for output dump to file." << std::endl;
    std::cout << "--registerdump / -r FILE   optional file to save register dump in." << std::endl;
    std::cout << "--targetevents / -e COUNT  stop acquisition loop after COUNT events." << std::endl;
    std::cout << "--targettime / -t SECS     stop acquisition loop after SECS seconds." << std::endl;
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
    const char* const short_opts = "a:c:d:e:hp:r:t:";
    const option long_opts[] = {
        {"address", 1, nullptr, 'a'},
        {"confoverride", 1, nullptr, 'c'},
        {"dumpprefix", 1, nullptr, 'd'},
        {"targetevents", 1, nullptr, 'e'},
        {"help", 0, nullptr, 'h'},
        {"port", 1, nullptr, 'p'},
        {"registerdump", 1, nullptr, 'r'},
        {"targettime", 1, nullptr, 't'},
        {nullptr, 0, nullptr, 0}
    };

    /* Default conf values - file writes and UDP send disabled by default */
    RuntimeConf conf;
    conf.address = "";
    conf.port = DEFAULT_UDP_PORT;
    conf.configFileName = "";
    conf.overrideFileName = "";
    conf.channelDumpPrefix = "";
    conf.registerDumpFileName = "";
    conf.overrideEnabled = false;
    conf.channelDumpEnabled = false;
    conf.waveDumpEnabled = false;
    conf.registerDumpEnabled = false;
    conf.sendEventEnabled = false;
    conf.stopAfterEvents = 0;
    conf.stopAfterSeconds = 0;
    conf.workerThreads = 1;

    /* Parse command line options */
    while (true) {
        const auto opt = getopt_long(argc, argv, short_opts, long_opts, nullptr);
        if (-1 == opt)
            break;

        switch (opt) {
        case 'a':
            conf.address = std::string(optarg);
            break;
        case 'c':
            conf.overrideFileName = std::string(optarg);
            conf.overrideEnabled = true;
            break;
        case 'd':
            conf.channelDumpPrefix = std::string(optarg);
            conf.channelDumpEnabled = true;
            conf.waveDumpEnabled = true;
            break;
        case 'e':
            conf.stopAfterEvents = std::stoi(optarg);
            break;
        case 'p':
            conf.port = std::string(optarg);
            break;
        case 'r':
            conf.registerDumpFileName = std::string(optarg);
            conf.registerDumpEnabled = true;
            break;
        case 't':
            conf.stopAfterSeconds = std::stoi(optarg);
            break;
        case 'h': // -h or --help
        case '?': // Unrecognized option
        default:
            usageHelp(argv[0]);
            exit(0);
            break;
        }
    }

    if (conf.address.length() > 0 && conf.port.length() > 0) {
            conf.sendEventEnabled = true;
    }
    
    /* No further command-line arguments */
    if (argc - optind > 1) {
        usageHelp(argv[0]);
        exit(1);
    }
    if (argc - optind > 0) {
        conf.configFileName = argv[optind];        
    }

    /* Total acquisition stats */
    TotalStats totals;
    totals.bytesRead = 0;
    totals.runMilliseconds = 0;    
    totals.eventsFound = 0;
    totals.eventsUnpacked = 0;
    totals.eventsDecoded = 0;
    totals.eventsSent = 0;    

    /* Singleton helpers */
    uint64_t acquisitionStart = 0, acquisitionStopped = 0;

    /* Active digitizers */
    std::vector<Digitizer> digitizers;
    typedef std::map<const std::string, CommHelper *> comm_helper_map;
    comm_helper_map commHelpers;

    /* Read-in and write resulting digitizer configuration */
    std::ifstream configFile(conf.configFileName);
    if (!configFile.good())
    {
        std::cerr << "Could not open jadaq configuration file: " << conf.configFileName << std::endl;
        /* NOTE: fall back to hard coded digitizer for sample setup */
        digitizers.push_back(Digitizer(0,0x11130000));
    } else {
        std::cout << "Reading digitizer configuration from" << conf.configFileName << std::endl;
        /* NOTE: switch verbose (2nd) arg on here to enable conf warnings */ 
        Configuration configuration(configFile, false);
        std::string outFileName = conf.configFileName+".out";
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

    /* Helpers for output - eventually move to stand-alone asio receiver? */
    std::ofstream *channelChargeWriters, *channelWaveWriters;
    typedef std::map<const std::string, std::ofstream *> ofstream_map;
    ofstream_map charge_writer_map;
    ofstream_map wave_writer_map;
    std::stringstream path;
    
    if (conf.channelDumpEnabled) {
        boost::filesystem::path prefix(conf.channelDumpPrefix);
        boost::filesystem::path dir = prefix.parent_path();
        try {
            boost::filesystem::create_directories(dir);
        } catch (std::exception& e) {
            std::cerr << "WARNING: failed to create channel dump output dir " << dir << " : " << e.what() << std::endl;
            exit(1);
        }
    }

    /* Prepare and start acquisition for all digitizers */
    std::cout << "Setup acquisition from " << digitizers.size() << " digitizer(s)." << std::endl;
    for (Digitizer& digitizer: digitizers)
    {
        /* NOTE: apply overrides from provided CAEN config. 
         *       this can be used to mimic CAEN QDC sample.
         */
        if (conf.overrideEnabled) {
            std::cout << "Override " << digitizer.name() << " configuration with " << conf.overrideFileName << std::endl;
            BoardParameters params;
            if (setup_parameters(&params, (char *)conf.overrideFileName.c_str()) < 0) {
                std::cerr << "Error in setup parameters from " << conf.overrideFileName << std::endl;
            } else if (configure_digitizer(digitizer.caen()->handle(), digitizer.caen(), &params) < 0) {
                std::cerr << "Error in configuring digitizer with overrides from " << conf.overrideFileName << std::endl;
            }
        }

        /* Record timestamps and charges in per-channel files on request */
        if (conf.channelDumpEnabled) {
            std::cout << "Dumping individual recorded channel charges from " << digitizer.name() << " in files " << conf.channelDumpPrefix << "-" << digitizer.name() << "-charge-CHANNEL.txt" << std::endl;
            channelChargeWriters = new std::ofstream[MAX_CHANNELS];
            charge_writer_map[digitizer.name()] = channelChargeWriters;
            for (int channel=0; channel<MAX_CHANNELS; channel++) {
                path.str("");
                path.clear();
                path << conf.channelDumpPrefix << "-" << digitizer.name() << "-charge-" << std::setfill('0') << std::setw(2) << channel << ".txt";
                channelChargeWriters[channel].open(path.str(), std::ofstream::out);
            }
            if (digitizer.caenHasDPPWaveformsEnabled() && conf.waveDumpEnabled) {
                std::cout << "Dumping individual recorded channel waveforms from " << digitizer.name() << " in files " << conf.channelDumpPrefix << "-" << digitizer.name() << "-wave-CHANNEL.txt" << std::endl;
                channelWaveWriters = new std::ofstream[MAX_CHANNELS];
                wave_writer_map[digitizer.name()] = channelWaveWriters;
                for (int channel=0; channel<MAX_CHANNELS; channel++) {
                    path.str("");
                    path.clear();
                    path << conf.channelDumpPrefix << "-" << digitizer.name() << "-wave-" << std::setfill('0') << std::setw(2) << channel << ".txt";
                    channelWaveWriters[channel].open(path.str(), std::ofstream::out);
                }
                
            }
        }
        if (conf.sendEventEnabled) {
            /* Setup UDP sender for this digitizer */
            try {
                CommHelper *digitizerComm = new CommHelper();
                commHelpers[digitizer.name()] = digitizerComm;
                udp::resolver resolver(digitizerComm->sendIOService);
                udp::resolver::query query(udp::v4(), conf.address.c_str(), conf.port.c_str());
                digitizerComm->remoteEndpoint = *resolver.resolve(query);
                digitizerComm->socket = new udp::socket(digitizerComm->sendIOService);
                digitizerComm->socket->open(udp::v4());
            } catch (std::exception& e) {
                std::cerr << "ERROR in UDP connection setup to " << conf.address << " : " << e.what() << std::endl;
                exit(1);
            }
        }

        /* Dump actual digitizer registers for debugging on request */
        if (conf.registerDumpEnabled) {
            std::cout << "Dumping digitizer " << digitizer.name() << " registers in " << conf.registerDumpFileName << std::endl;
            if (dump_configuration(digitizer.caen()->handle(), (char *)conf.registerDumpFileName.c_str()) < 0) {
                std::cerr << "Error in dumping digitizer registers in " << conf.registerDumpFileName << std::endl;
                exit(1); 
            }
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

    acquisitionStart = getTimeMsecs();
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
         *   - optionally dump data on simple format
         *   - optionally pack and send out data on UDP
         */
        if (conf.stopAfterEvents > 0 && conf.stopAfterEvents <= totals.eventsFound ||
            conf.stopAfterSeconds > 0 && conf.stopAfterSeconds * 1000 <= totals.runMilliseconds) {
            std::cout << "Stop condition reached: ran for " << totals.runMilliseconds / 1000.0 << " seconds (target is " << conf.stopAfterSeconds << ") and handled " << totals.eventsFound << " events (target is " << conf.stopAfterEvents << ")." << std::endl;
            keepRunning = false;
            break;
        } else if (conf.stopAfterEvents > 0) {
            std::cout << "Handled " << totals.eventsFound << " out of " << conf.stopAfterEvents << " events allowed so far." << std::endl;            
        }
        if (throttleDown > 0) {
            /* NOTE: for running without hogging CPU if nothing to do */
            std::this_thread::sleep_for(std::chrono::milliseconds(throttleDown));
        }
        try {
            std::cout << "Read out data from " << digitizers.size() << " digitizer(s)." << std::endl;
            /* Read out acquired data for all digitizers */
            /* TODO: split digitizer handling loop into separate threads */
            /* TODO: additionally hand off decoding and sending to threads? */
            for (Digitizer& digitizer: digitizers) {
                /* NOTE: these are per-digitizer helpers */
                uint32_t eventIndex = 0, decodeChannels = 0, j = 0, k = 0;
                caen::BasicEvent basicEvent;
                caen::BasicDPPEvent basicDPPEvent;
                caen::BasicDPPWaveforms basicDPPWaveforms;
                uint32_t charge = 0, timestamp = 0, channel = 0;
                uint64_t globaltime = 0, now = 0, count = 0;
                uint16_t *samples = NULL;
                LocalStats stats;
                stats.bytesRead = 0;
                stats.eventsFound = 0; 
                stats.eventsUnpacked = 0;
                stats.eventsDecoded = 0;
                stats.eventsSent = 0;

                /* NOTE: use time since epoch with millisecond resolution to
                 * keep timestamps unique */
                globaltime = getTimeMsecs();

                std::cout << "Read at most " << digitizer.caenGetPrivReadoutBuffer().size << "b data from " << digitizer.name() << std::endl;
                digitizer.caenReadData(digitizer.caenGetPrivReadoutBuffer());
                stats.bytesRead = digitizer.caenGetPrivReadoutBuffer().dataSize;
                std::cout << "Read " << stats.bytesRead << "b of acquired data" << std::endl;

                /* NOTE: check and skip if there's no actual events to handle */
                if (digitizer.caenIsDPPFirmware()) {
                    std::cout << "Unpack aggregated DPP events from " << digitizer.name() << std::endl;
                    digitizer.caenGetDPPEvents(digitizer.caenGetPrivReadoutBuffer(), digitizer.caenGetPrivDPPEvents());
                    for (channel = 0; channel < MAX_CHANNELS; channel++) {
                        stats.eventsFound += digitizer.caenGetPrivDPPEvents().nEvents[channel];
                    }
                    stats.eventsUnpacked += stats.eventsFound;
                    if (stats.eventsFound < 1)
                        continue;
                    std::cout << "Unpacked " << stats.eventsUnpacked << " DPP events from all channels." << std::endl;
                } else {
                    stats.eventsFound += digitizer.caenGetNumEvents(digitizer.caenGetPrivReadoutBuffer());
                    std::cout << "Acquired data from " << digitizer.name() << " contains " << stats.eventsFound << " event(s)." << std::endl;
                    if (stats.eventsFound < 1) {
                        std::cout << "No events found - no further handling." << std::endl;
                        throttleDown = std::min((uint32_t)2000, 2*throttleDown + 100);
                        continue;
                    }
                }

                if (conf.sendEventEnabled) {
                    CommHelper *digitizerComm = commHelpers[digitizer.name()];
                    /* Reset send buffer each time to prevent any stale data */
                    memset(digitizerComm->sendBuf, 0, MAXBUFSIZE);
                    /* TODO: add check to make sure sendBuf always fits eventData */
                    digitizerComm->eventData = Data::setupEventData((void *)digitizerComm->sendBuf, MAXBUFSIZE, stats.eventsFound, 0);
                    std::cout << "Prepared eventData " << digitizerComm->eventData << " from sendBuf " << (void *)digitizerComm->sendBuf << std::endl;
                    digitizerComm->metadata = digitizerComm->eventData->metadata;
                    /* NOTE: safe copy with explicit string termination */
                    strncpy(digitizerComm->eventData->metadata->digitizerModel, digitizer.model().c_str(), MAXMODELSIZE);
                    digitizerComm->eventData->metadata->digitizerModel[MAXMODELSIZE-1] = '\0';
                    digitizerComm->eventData->metadata->digitizerID = std::stoi(digitizer.serial());
                    digitizerComm->eventData->metadata->globalTime = globaltime;
                    std::cout << "Prepared eventData has " << digitizerComm->eventData->listEventsLength << " listEvents " << std::endl;
                }

                if (digitizer.caenIsDPPFirmware()) {
                    eventIndex = 0;
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

                            std::cout << digitizer.name() << " channel " << channel << " event " << j << " charge " << charge << " at global time " << globaltime << " and local time " << timestamp << std::endl;

                            if (conf.channelDumpEnabled) {
                                /* NOTE: write in requested format */
                                channelChargeWriters = charge_writer_map[digitizer.name()];
                                /* NOTE: we use explicit "\n" rather than std::endl.
                                 *       This is in order to avoid automatic
                                 *       ofstream flush after each line.
                                 */
                                channelChargeWriters[channel] << std::setw(16) << timestamp << " " << digitizer.serial() << " " << std::setw(8) << channel << " " << std::setw(8) << charge << "\n";
                            }
                            
                            /* Only try to decode waveforms if digitizer is actually
                             * configured to record them in the first place. */
                            if (digitizer.caenHasDPPWaveformsEnabled()) {
                                try {
                                    digitizer.caenDecodeDPPWaveforms(digitizer.caenGetPrivDPPEvents(), channel, j, digitizer.caenGetPrivDPPWaveforms());

                                    std::cout << "Decoded " << digitizer.caenDumpPrivDPPWaveforms() << " DPP event waveforms from event " << j << " on channel " << channel << " from " << digitizer.name() << std::endl;
                                    stats.eventsDecoded += 1;
                                    if (conf.channelDumpEnabled and conf.waveDumpEnabled) {
                                        channelWaveWriters = wave_writer_map[digitizer.name()];
                                        /* NOTE: we don't want to muck with underlying
                                         * event type here, so we rely on the wrapped
                                         * extraction and pull out
                                         * values from the resulting
                                         * BasicDPPWaveforms. */
                                        basicDPPWaveforms = digitizer.caenExtractBasicDPPWaveforms(digitizer.caenGetPrivDPPWaveforms());
                                        for(k=0; k<basicDPPWaveforms.Ns; k++) {
                                            channelWaveWriters[channel] << basicDPPWaveforms.Trace1[k];                 /* samples */
                                            channelWaveWriters[channel] << " " << 2000 + 200 * basicDPPWaveforms.DTrace1[k];  /* gate    */
                                            channelWaveWriters[channel] << " " << 1000 + 200 *basicDPPWaveforms.DTrace2[k];  /* trigger */
                                            if (basicDPPWaveforms.DTrace3 != NULL)
                                                channelWaveWriters[channel] << " " << 500 + 200 * basicDPPWaveforms.DTrace3[k];   /* trg hold off */
                                            if (basicDPPWaveforms.DTrace4 != NULL)
                                                channelWaveWriters[channel] << " " << 100 + 200 * basicDPPWaveforms.DTrace4[k];  /* overthreshold */
                                            /* NOTE: we use explicit "\n" rather than std::endl.
                                             *       This is in order to avoid automatic
                                             *       ofstream flush after each line.
                                             */
                                            channelWaveWriters[channel] << "\n";
                                        }
                                    }                                    
                                } catch(std::exception& e) {
                                    std::cerr << "failed to decode waveforms for event " << j << " on channel " << channel << " from " << digitizer.name() << " : " << e.what() << std::endl;
                                }
                            }                            

                            if (conf.sendEventEnabled) {
                                CommHelper *digitizerComm = commHelpers[digitizer.name()];
                                //std::cout << "Filling event at " << globaltime << " from " << digitizer.name() << " channel " << channel << " localtime " << timestamp << " charge " << charge << std::endl;
                                digitizerComm->eventData->listEvents[eventIndex].localTime = timestamp;
                                digitizerComm->eventData->listEvents[eventIndex].extendTime = 0;
                                digitizerComm->eventData->listEvents[eventIndex].adcValue = charge;
                                digitizerComm->eventData->listEvents[eventIndex].channel = channel;
                            }
                            eventIndex += 1;
                        }
                    }
                } else { 
                    /* Handle the non-DPP case */
                    for (eventIndex=0; eventIndex < stats.eventsFound; eventIndex++) {
                        digitizer.caenGetEventInfo(digitizer.caenGetPrivReadoutBuffer(), eventIndex);
                        std::cout << "Unpacked event " << digitizer.caenGetPrivEventInfo().EventCounter << "  of " << stats.eventsFound << " events from " << digitizer.name() << std::endl;
                        stats.eventsUnpacked += 1;
                        digitizer.caenDecodeEvent(digitizer.caenGetPrivEventInfo(), digitizer.caenGetPrivEvent());
                        std::cout << "Decoded event " << digitizer.caenGetPrivEventInfo().EventCounter << "  of " << stats.eventsFound << " events from " << digitizer.name() << std::endl;
                        stats.eventsDecoded += 1;
                        /* TODO: where do we get real channel from in these events?!? */
                        /*       it looks like decoded events is really a matrix
                         *       of channel events a bit like in the DPPEvents case */
                        channel = 4242;
                        basicEvent = digitizer.caenExtractBasicEvent(digitizer.caenGetPrivEventInfo(), digitizer.caenGetPrivEvent(), channel, eventIndex);
                        timestamp = basicEvent.timestamp;
                        count = basicEvent.count;
                        samples = basicEvent.samples;
                        if (conf.sendEventEnabled) {
                            CommHelper *digitizerComm = commHelpers[digitizer.name()];
                            //std::cout << "Filling event at " << globaltime << " from " << digitizer.name() << " channel " << channel << " localtime " << timestamp << " sample count " << count << std::endl;
                            digitizerComm->eventData->waveformEvents[eventIndex].localTime = timestamp;
                            digitizerComm->eventData->waveformEvents[eventIndex].waveformLength = count;
                            memcpy(digitizerComm->eventData->waveformEvents[eventIndex].waveform, samples, (count * sizeof(samples[0])));
                            digitizerComm->eventData->waveformEvents[eventIndex].channel = channel;
                        }   
                    }
                }

                /* Pack and send out UDP */
                if (conf.sendEventEnabled) {
                    CommHelper *digitizerComm = commHelpers[digitizer.name()];
                    std::cout << "Packing events at " << globaltime << " from " << digitizer.name() << std::endl;
                    digitizerComm->packedEvents = Data::packEventData(digitizerComm->eventData, stats.eventsFound, 0);
                    /* Send data to preconfigured receiver */
                    std::cout << "Sending " << stats.eventsUnpacked << " packed events of " << digitizerComm->packedEvents.dataSize << "b at " << globaltime << " from " << digitizer.name() << " to " << conf.address << ":" << conf.port << std::endl;
                    digitizerComm->socket->send_to(boost::asio::buffer((char*)(digitizerComm->packedEvents.data), digitizerComm->packedEvents.dataSize), digitizerComm->remoteEndpoint);
                    stats.eventsSent += stats.eventsUnpacked;
                }

                /* Update total stats atomically */
                now = getTimeMsecs();
                totals.runMilliseconds = now - acquisitionStart;
                totals.bytesRead += stats.bytesRead;
                totals.eventsFound += stats.eventsFound;
                totals.eventsUnpacked += stats.eventsUnpacked;
                totals.eventsDecoded += stats.eventsDecoded;
                totals.eventsSent += stats.eventsSent;

                /* No throttling if handling succeeded this far */
                throttleDown = 0;
            }
            

            std::cout << "= Accumulated Stats =" << std::endl;
            std::cout << "Runtime in seconds: " << totals.runMilliseconds / 1000.0 << std::endl;
            std::cout << "Bytes read: " << totals.bytesRead << std::endl;
            std::cout << "Aggregated events found: " << totals.eventsFound << std::endl;
            std::cout << "Individual events unpacked: " << totals.eventsUnpacked << std::endl;
            std::cout << "Individual events decoded: " << totals.eventsDecoded << std::endl;
            std::cout << "Individual events sent: " << totals.eventsSent << std::endl;

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

    acquisitionStopped = getTimeMsecs();
    std::cout << "Acquisition loop ran for " << (acquisitionStopped - acquisitionStart) / 1000.0 << "s." << std::endl;

    /* Clean up after all digitizers: buffers, etc. */
    std::cout << "Clean up after " << digitizers.size() << " digitizer(s)." << std::endl;
    for (Digitizer& digitizer: digitizers)
    {
        if (conf.channelDumpEnabled) {
            std::cout << "Closing channel charge dump files for " << digitizer.name() << std::endl;
            channelChargeWriters = charge_writer_map[digitizer.name()];
            for (int channel=0; channel<MAX_CHANNELS; channel++) {
                channelChargeWriters[channel].close();
            }
            charge_writer_map.erase(digitizer.name());
            delete[] channelChargeWriters;
        }

        if (conf.sendEventEnabled) {
            std::cout << "Closing comm helpers for " << digitizer.name() << std::endl;
            delete commHelpers[digitizer.name()];
        }

        if (digitizer.caenIsDPPFirmware()) {
            if (digitizer.caenHasDPPWaveformsEnabled()) {
                if (conf.channelDumpEnabled && conf.waveDumpEnabled) {
                    std::cout << "Closing channel wave dump files for " << digitizer.name() << std::endl;
                    channelWaveWriters = wave_writer_map[digitizer.name()];
                    for (int channel=0; channel<MAX_CHANNELS; channel++) {
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
