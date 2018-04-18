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
#include "trace.hpp"
#include "DataHandlerNetworkSend.hpp"
#include "DataHandlerText.hpp"
#include "uuid.hpp"

#define NOTHREADS
#define IDLESLEEP (10)

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
    bool sendListEventEnabled;
    bool sendWaveformEventEnabled;
    uint32_t stopAfterEvents;
    uint32_t stopAfterSeconds;
    uint32_t workerThreads;
};

/* Use atomic to make sure updates from different threads won't cause races */
struct TotalStats {
    std::atomic<uint64_t> runStartTime;
    std::atomic<uint32_t> bytesRead;
    std::atomic<uint32_t> eventsFound;
    std::atomic<uint32_t> eventsUnpacked;
    std::atomic<uint32_t> eventsDecoded;
    std::atomic<uint32_t> eventsSent;
};

/* Per-digitizer thread helpers */
struct ThreadHelper {
    /* TODO: move thread pool etc here as well? */
    std::atomic<bool> ready;
};

/* For channel dump and communication */
typedef std::map<const std::string, std::ofstream *> ofstream_map;
typedef std::map<const std::string, ThreadHelper *> thread_helper_map;

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

void showTotals(TotalStats &totals) {
    STAT(uint64_t runtimeMsecs = DataHandler::getTimeMsecs() - totals.runStartTime;)
    STAT(std::cout << "= Accumulated Stats =" << std::endl;)
    STAT(std::cout << "Runtime in seconds: " << runtimeMsecs / 1000.0 << std::endl;)
    STAT(std::cout << "Bytes read: " << totals.bytesRead << std::endl;)
    STAT(std::cout << "Aggregated events found: " << totals.eventsFound << std::endl;)
    STAT(std::cout << "Individual events unpacked: " << totals.eventsUnpacked << std::endl;)
    STAT(std::cout << "Individual events decoded: " << totals.eventsDecoded << std::endl;)
    STAT(std::cout << "Individual events sent: " << totals.eventsSent << std::endl;)
}

void usageHelp(char *name) {
    std::cout << "Usage: " << name << " [<options>] [<jadaq_config_file>]" << std::endl;
    std::cout << "Where <options> can be:" << std::endl;
    std::cout << "--address / -a ADDRESS     optional UDP network address to send to (unset by default)." << std::endl;
    std::cout << "--port / -p PORT           optional UDP network port to send to (default is " << DataHandler::defaultDataPort << ")." << std::endl;
    std::cout << "--confoverride / -c FILE   optional conf overrides on CAEN format." << std::endl;
    std::cout << "--dumpprefix / -d PREFIX   optional prefix for output dump to file." << std::endl;
    std::cout << "--registerdump / -r FILE   optional file to save register dump in." << std::endl;
    std::cout << "--targetevents / -e COUNT  stop acquisition loop after COUNT events." << std::endl;
    std::cout << "--targettime / -t SECS     stop acquisition loop after SECS seconds." << std::endl;
    std::cout << "--workers / -w THREADS     use a thread pool with THREADS workers." << std::endl;
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
    const char* const short_opts = "a:c:d:e:hp:r:s:t:w:";
    const option long_opts[] = {
        {"address", 1, nullptr, 'a'},
        {"confoverride", 1, nullptr, 'c'},
        {"dumpprefix", 1, nullptr, 'd'},
        {"targetevents", 1, nullptr, 'e'},
        {"help", 0, nullptr, 'h'},
        {"port", 1, nullptr, 'p'},
        {"registerdump", 1, nullptr, 'r'},
        {"sendtypes", 1, nullptr, 's'},
        {"targettime", 1, nullptr, 't'},
        {"workers", 1, nullptr, 'w'},
        {nullptr, 0, nullptr, 0}
    };

    /* Default conf values - file writes and UDP send disabled by default */
    RuntimeConf conf;
    conf.address = "";
    conf.port = DataHandler::defaultDataPort;
    conf.configFileName = "";
    conf.overrideFileName = "";
    conf.channelDumpPrefix = "";
    conf.registerDumpFileName = "";
    conf.overrideEnabled = false;
    conf.channelDumpEnabled = false;
    conf.waveDumpEnabled = false;
    conf.registerDumpEnabled = false;
    conf.sendEventEnabled = false;
    conf.sendListEventEnabled = true;
    conf.sendWaveformEventEnabled = false;
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
        case 's':
            conf.sendListEventEnabled = std::string(optarg).find("list") != std::string::npos;
            conf.sendWaveformEventEnabled = std::string(optarg).find("waveform") != std::string::npos;
            break;
        case 't':
            conf.stopAfterSeconds = std::stoi(optarg);
            break;
        case 'w':
            conf.workerThreads = std::stoi(optarg);
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
    if (conf.sendEventEnabled && !conf.sendListEventEnabled && !conf.sendWaveformEventEnabled) {
        std::cerr << "WARNING: you provided send options but neither enabled send for list or waveform events. Disabling send!" << std::endl;
        conf.sendEventEnabled = false;
    }
    
    /* No further command-line arguments */
    if (argc - optind > 1) {
        usageHelp(argv[0]);
        exit(1);
    }
    if (argc - optind > 0) {
        conf.configFileName = argv[optind];        
    }

    /* Total acquisition stats_ */
    TotalStats totals;
    totals.bytesRead = 0;
    totals.eventsFound = 0;
    totals.eventsUnpacked = 0;
    totals.eventsDecoded = 0;
    totals.eventsSent = 0;    

    uuid runID;

    /* Singleton helpers */
    uint64_t acquisitionStart = 0, acquisitionStopped = 0;
    uint64_t runtimeMsecs = 0;
    uint16_t tasksEnqueued = 0;
    boost::asio::io_service threadIOService;
    boost::thread_group threadPool;
    
    /* Active digitizers */
    std::vector<Digitizer> digitizers;
    thread_helper_map threadHelpers;

    /* Read-in and write resulting digitizer configuration */
    std::ifstream configFile(conf.configFileName);
    if (!configFile.good())
    {
        std::cerr << "Could not open jadaq configuration file: " << conf.configFileName << std::endl;
        exit(1);
    } else {
        DEBUG(std::cout << "Reading digitizer configuration from" << conf.configFileName << std::endl;)
        /* NOTE: switch verbose (2nd) arg on here to enable conf warnings */ 
        Configuration configuration(configFile, false);
        std::string outFileName = conf.configFileName+".out";
        std::ofstream outFile(outFileName);
        if (!outFile.good())
        {
            std::cerr << "Unable to open output file: " << outFileName << std::endl;
        } else {
            /* TODO Turn ON/Off with commandline switch */
            DEBUG(std::cout << "Writing current digitizer configuration to " << outFileName << std::endl;)
            configuration.write(outFile);
            outFile.close();
        }
        // Extract a vector of all configured digitizers for later
        digitizers = configuration.getDigitizers();
    }
    configFile.close();

    /* Prepare and start acquisition for all digitizers */
    std::cout << "Setup acquisition from " << digitizers.size() << " digitizer(s)." << std::endl;
    for (Digitizer& digitizer: digitizers)
    {
        ThreadHelper *digitizerThread = new ThreadHelper();
        threadHelpers[digitizer.name()] = digitizerThread;
        digitizerThread->ready = true;
        /* Prepare buffers - must happen AFTER digitizer has been configured! */
        DataHandler* dataHandler;
        if (conf.sendEventEnabled) {
            dataHandler = new DataHandlerNetworkSend(conf.address, conf.port);
        } else
        {
            dataHandler = new DataHandlerText("dump" + digitizer.name() + ".txt");
        }

        digitizer.initialize(runID, dataHandler);
    }

    acquisitionStart = DataHandler::getTimeMsecs();
    totals.runStartTime = acquisitionStart;
    std::cout << "Start acquisition from " << digitizers.size() << " digitizer(s)." << std::endl;
    for (Digitizer& digitizer: digitizers) {
        std::cout << "Start acquisition on digitizer " << digitizer.name() << std::endl;
        digitizer.startAcquisition();
    }

    /* Auto-detect available cores and create a thread for each if
     * workerThreads is not explicitly set. */
    if (conf.workerThreads < 1) {
        conf.workerThreads = boost::thread::hardware_concurrency();
    }
    /* Set up thread pool to distribute tasks to multiple cores */
    std::cout << "Start thread pool of " << conf.workerThreads << " worker(s)." << std::endl;
    /* Use a work wrapper to allow waiting for all pending tasks on exit */
    boost::asio::io_service::work *work = new boost::asio::io_service::work(threadIOService);
    for (int i=0; i<conf.workerThreads; i++) {
        std::cout << "Create thread " << i << " for thread pool." << std::endl;
        threadPool.create_thread(boost::bind(&boost::asio::io_service::run, &threadIOService));
    }

    /* Set up interrupt handler and start handling acquired data */
    setup_interrupt_handler();

    std::cout << "Running acquisition loop - Ctrl-C to interrupt" << std::endl;

    bool keepRunning = true;
    while(keepRunning) {
        /* Continuously acquire and process data:
         *   - read out data
         *   - decode data
         *   - optionally dump data on simple format
         *   - optionally pack and send out data on UDP
         */
        std::cout << "Read out data from " << digitizers.size() << " digitizer(s)." << std::endl;
        /* Read out acquired data for all digitizers */
        tasksEnqueued = 0;
        totals.eventsFound = 0;
        for (Digitizer& digitizer: digitizers) {
            try {
                ThreadHelper *digitizerThread = threadHelpers[digitizer.name()];
                if (digitizerThread->ready) {
                    digitizerThread->ready = false;
                    std::cout << "Enqueuing new digitizer acquisition for " << digitizer.name() << std::endl;
#ifdef NOTHREADS
                    /* Inline digitizerAcquisition without threading */
                    digitizer.acquisition();
                    digitizerThread->ready = true;
#else
                    /* Enqueue digitizerAcquisition for thread handling */
                    threadIOService.post(boost::bind(digitizer.acquisition));
#endif
                    tasksEnqueued++;
                } else {
                    std::cout << "Acquisition still active for digitizer " << digitizer.name() << std::endl;
                }
            } catch(std::exception& e) {
                std::cerr << "ERROR: unexpected exception during acquisition: " << e.what() << std::endl;
                throw;
            }
            
            STAT(totals.eventsFound += digitizer.stats().eventsFound;)
        }
        STAT(if (conf.stopAfterEvents > 0) {
            DEBUG(std::cout << "Handled " << totals.eventsFound << " out of " << conf.stopAfterEvents << " events allowed so far." << std::endl;)
            if (totals.eventsFound >= conf.stopAfterEvents)
            {
                break;
            }
        })
        if (tasksEnqueued < 1) {
            //std::cout << "Digitizer loop idle - throttle down" << std::endl;
            /* NOTE: for running without hogging CPU if nothing to do */
            std::this_thread::sleep_for(std::chrono::milliseconds(IDLESLEEP));
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
        digitizer.stopAcquisition();
    }

    acquisitionStopped = DataHandler::getTimeMsecs();
    std::cout << "Acquisition loop ran for " << (acquisitionStopped - acquisitionStart) / 1000.0 << "s." << std::endl;

    /* Stop and wait for thread pool to complete */
    /* NOTE: this delete forces tasks to finish first */
    delete work;
    std::cout << "Wait for " << conf.workerThreads << " worker thread(s) to finish." << std::endl;
    threadPool.join_all();

    showTotals(totals);

    /* Clean up after all digitizers: buffers, etc. */
    std::cout << "Clean up after " << digitizers.size() << " digitizer(s)." << std::endl;
    for (Digitizer& digitizer: digitizers)
    {
        std::cout << "Closing thread helpers for " << digitizer.name() << std::endl;
        delete threadHelpers[digitizer.name()];
        digitizer.close();

    }

    std::cout << "Acquisition complete - shutting down." << std::endl;

    /* Clean up after run */

    return 0;
}
