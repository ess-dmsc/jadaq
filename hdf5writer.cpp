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

#include <getopt.h>
#include <signal.h>
#include <chrono>
#include <thread>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <string>
#include <ctime>

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>

#include "H5Cpp.h"

using boost::asio::ip::udp;

using namespace H5;

#include "DataFormat.hpp"


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
    std::cout << "Usage: " << name << " [<options>] [<output_file>]" << std::endl;
    std::cout << "Where <options> can be:" << std::endl;
    std::cout << "--address / -a ADDRESS   the UDP network address to listen on (default is " << UDP_LISTEN_ALL << ")." << std::endl;
    std::cout << "--port / -p PORT         the UDP network port to listen on (default is " << DEFAULT_UDP_PORT << ")." << std::endl;
    std::cout << std::endl << "Listens for events on the network and dumps " << std::endl;
    std::cout << "received data as HDF5 into <output_file>." << std::endl;
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
    /* Listen on all network addresses by default */
    std::string address = UDP_LISTEN_ALL, port = DEFAULT_UDP_PORT;
    std::string outputFileName = "out.h5";

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
            exit(0);
            break;
        }
    }

    /* Check and act on positional command-line arguments */
    if (argc - optind > 1) {
        usageHelp(argv[0]);
        exit(1);
    }
    if(argc - optind > 0) {
        outputFileName = std::string(argv[optind]);
    }

    std::cout << "Listening for UDP packages on: " << address << ":" << port << std::endl;
    std::cout << "Writing hdf5 formatted data to: " << outputFileName << std::endl;

    /* Data format version - please increment on versiondata on layout changes */
    uint16_t versiondata[VERSIONPARTS] = VERSION;
    hsize_t versiondims[1] = {VERSIONPARTS};

    /* Helpers */
    uint32_t listEventsReceived = 0, waveformEventsReceived = 0, eventsWritten = 0;
    std::string flavor;

    uint32_t eventIndex = 0;
    /* Dummy data */
    std::string digitizer, digitizerModel;
    uint16_t digitizerID = 0;
    uint32_t channel = 0, charge = 0, localtime = 0;
    uint64_t globaltime = 0;

    /* Listening helpers */
    boost::asio::io_service io_service;
    udp::endpoint receiver_endpoint;
    udp::socket *socket = NULL;
    /* NOTE: use a static buffer of MAXBUFSIZE bytes for receiving */
    char recv_buf[MAXBUFSIZE];
    udp::endpoint remote_endpoint;
    boost::system::error_code error;
    Data::PackedEvents packedEvents;
    packedEvents.data = &recv_buf;
    packedEvents.size = MAXBUFSIZE;
    packedEvents.dataSize = 0;
    Data::EventData *eventData;
    Data::Meta *metadata;
    Data::List::Element *listEvent;
    Data::Waveform::Element *waveformEvent;
    uint32_t offset = 0;
    const H5std_string MEMBER1( "localTime_name" );
    const H5std_string MEMBER2( "adcValue_name" );
    const H5std_string MEMBER3( "extendTime_name" );
    const H5std_string MEMBER4( "channel_name" );
    const H5std_string MEMBER5( "__pad_name" );
    CompType listEventType(sizeof(Data::List::Element));
    listEventType.insertMember(MEMBER1, HOFFSET(Data::List::Element, localTime), PredType::STD_U32LE);
    listEventType.insertMember(MEMBER2, HOFFSET(Data::List::Element, adcValue), PredType::STD_U32LE); 
    listEventType.insertMember(MEMBER3, HOFFSET(Data::List::Element, extendTime), PredType::STD_U16LE);
    listEventType.insertMember(MEMBER4, HOFFSET(Data::List::Element, channel), PredType::STD_U16LE);
    listEventType.insertMember(MEMBER5, HOFFSET(Data::List::Element, __pad), PredType::STD_U32LE);

    /* Prepare and start event handling */
    std::cout << "Setup hdf5writer" << std::endl;

    /* Prepare output file  and data sets */
    bool createOutfile = true, createGroup = true, createDataset = true;
    const H5std_string outname(outputFileName);
    H5std_string groupname, datasetname, versionname = H5std_string("version");
    std::stringstream nest;
    H5File *outfile = NULL;
    unsigned int h5flags = 0;
    /* NOTE: we enable single writer / multiple reader (SWMR) if hdf5 is
     * recent enough to support it (version 1.10+). It should be noted
     * that the SWMR user guide explicitly points out that adding groups
     * or datasets is NOT supported with SWMR, so it may still not work
     * for our purpose, and at least readers may have to regularly
     * reopen the file to detect the addition of data. 
     * It does seem to work at least for the simple hdf5monitor.py
     * included here, however.
     * We explicitly flush data to disk after each accumulated event
     * package to make sure updates get written regularly.
     */
#if H5_VERSION_GE(1,10,0)
        h5flags |= H5F_ACC_SWMR_WRITE;
        std::cout << "NOTE: Found a recent HDF version - enabling SWMR." << std::endl;
#else
        std::cout << "WARNING: HDF versions before 1.10 do not support SWMR - disabling." << std::endl;
#endif
    Group *rootgroup = NULL, *digitizergroup = NULL, *globaltimegroup = NULL;
    DataSet *dataset = NULL;
    DataSpace versionspace;
    Attribute *versionattr = NULL;
    /* NOTE: enable H5F_ACC_SWMR_WRITE/READ if available - requires 1.10+ */
    try {
        /* TODO: should we truncate or just keep adding? */
        if (createOutfile) {
            std::cout << "Creating new outfile " << outname << std::endl;
            outfile = new H5File(outname, h5flags|H5F_ACC_TRUNC);
            delete outfile;
        }
        std::cout << "Opening existing outfile " << outname << std::endl;
        outfile = new H5File(outname, h5flags|H5F_ACC_RDWR);
    } catch(FileIException error) {
        std::cerr << "ERROR: could not open/create outfile " << outname << std::endl;
        error.printError();
        exit(1);
    }

    /* Set version info on top level node */
    try {
        std::cout << "Try to open root group" << std::endl;
        rootgroup = new Group(outfile->openGroup("/"));
        versionspace = DataSpace(1, versiondims);
        versionattr = new Attribute(rootgroup->createAttribute(versionname, PredType::STD_U16LE, versionspace));
        versionattr->write(PredType::STD_U16LE, versiondata);
        outfile->flush(H5F_SCOPE_GLOBAL);
        delete versionattr;
        delete rootgroup;
    } catch(FileIException error) {
        std::cerr << "ERROR: could not open root group" << " : " << std::endl;
        error.printError();
    }

    /* TODO: switch to real struct data format in hdf5 */
    uint32_t data[EVENTFIELDS];
    const int RANK = 1;
    hsize_t dimsf[1];
    dimsf[0] = 1;
    DataSpace dataspace(RANK, dimsf);

    /* Turn off the auto-printing of errors - manual print instead. */
    Exception::dontPrint();

    /* Setup UDP listener */
    try {
        udp::endpoint local_end;
        /* Bind to any address if not explicitly provided */
        if (address == UDP_LISTEN_ALL || address == "") { 
            local_end = udp::endpoint(udp::v4(), std::stoi(port));
        } else {
            local_end = udp::endpoint(boost::asio::ip::address::from_string(address), std::stoi(port));
        }
        socket = new udp::socket(io_service, local_end);
    } catch (std::exception& e) {
        std::cerr << "ERROR opening socket on " << address << ":" << port << ": " << e.what() << std::endl;
        exit(1);
    }

    /* Set up interrupt handler and start handling acquired data */
    setup_interrupt_handler();

    std::cout << "Running file writer loop - Ctrl-C to interrupt" << std::endl;

    uint32_t throttleDown = 0;
    bool keepRunning = true;
    while(keepRunning) {
        /* Continuously receive and dump data */
        if (throttleDown > 0) {
            /* NOTE: for running without hogging CPU if nothing to do */
            std::this_thread::sleep_for(std::chrono::milliseconds(throttleDown));
        }
        try {
            std::cout << "Receive data" << std::endl;
            packedEvents.dataSize = socket->receive_from(boost::asio::buffer((char*)(packedEvents.data), MAXBUFSIZE), remote_endpoint, 0, error);
            if (error && error != boost::asio::error::message_size)
                throw boost::system::system_error(error);

            std::cout << "Received " << packedEvents.dataSize << "b of data from " << remote_endpoint.address().to_string() << std::endl;

            std::cout << "Unpack received package of " << packedEvents.dataSize << "b" << std::endl;
            eventData = Data::unpackEventData(packedEvents);
            metadata = eventData->metadata;
            std::cout << "Unpacked event data:" << std::endl;
            std::cout << "checksum: " << eventData->checksum << std::endl;
            std::cout << "listEventsLength: " << eventData->listEventsLength << std::endl;
            std::cout << "waveformEventsLength: " << eventData->waveformEventsLength << std::endl;

            std::cout << "Unpacked metadata:" << std::endl;
            std::cout << "version: " << metadata->version[0] << "." << metadata->version[1] << "." << metadata->version[2] << std::endl;
            std::cout << "digitizerModel: " << metadata->digitizerModel << std::endl;
            std::cout << "digitizerID: " << metadata->digitizerID << std::endl;
            std::cout << "globalTime: " << metadata->globalTime << std::endl;

            if (strcmp(Data::makeVersion(versiondata).c_str(), Data::makeVersion(metadata->version).c_str()) != 0) {
                std::cerr << "ERROR: version mismatch between local and remote data format: " << std::endl;
                std::cout << "local version: " << Data::makeVersion(versiondata) << std::endl;
                std::cout << "remote version: " << Data::makeVersion(metadata->version) << std::endl;
                throw std::runtime_error("version mismatch between local and remote data format");
            }

            digitizerModel = metadata->digitizerModel;
            digitizerID = metadata->digitizerID;
            nest.str("");
            nest.clear();
            nest << digitizerModel << "_" << digitizerID;
            digitizer = nest.str();

            /* Create a new group for the digitizer if it doesn't
             * exist in the output file. */
            groupname = H5std_string(digitizer);
            createGroup = true;
            try {
                std::cout << "Try to open group " << groupname << std::endl;
                digitizergroup = new Group(outfile->openGroup(groupname));
                createGroup = false;
            } catch(FileIException error) {
                if (!createOutfile) {
                    std::cerr << "ERROR: could not open group " << groupname << " : " << std::endl;
                    error.printError();
                } else {
                    //std::cout << "DEBUG: could not open group " << groupname << " : " << std::endl;
                }
            }
            if (createGroup) {
                std::cout << "Create group " << groupname << std::endl;
                digitizergroup = new Group(outfile->createGroup(groupname));
                createGroup = false;
            }

            /* Create a new group for the global time stamp if it
             * doesn't exist in the output file. */
            globaltime = metadata->globalTime;
            nest.str("");
            nest.clear();
            nest << globaltime;
            groupname = H5std_string(nest.str());
            createGroup = true;
            try {
                std::cout << "Try to open group " << groupname << std::endl;
                globaltimegroup = new Group(digitizergroup->openGroup(groupname));
                createGroup = false;
            } catch(GroupIException error) {
                if (!createOutfile) {
                    std::cerr << "ERROR: could not open group " << groupname << " : " << std::endl;
                    error.printError();
                } else {
                    //std::cout << "DEBUG: could not open group " << groupname << " : " << std::endl;
                }
            }
            if (createGroup) {
                std::cout << "Create group " << groupname << std::endl;
                globaltimegroup = new Group(digitizergroup->createGroup(groupname));
                createGroup = false;
            }
            
            delete digitizergroup;

            /* TODO: handle waveformEvents as well*/
            waveformEventsReceived = eventData->waveformEventsLength;

            /* Loop over received events and create a dataset for each
             * of them. */
            listEventsReceived = eventData->listEventsLength;
            flavor = "list";
            for (eventIndex = 0; eventIndex < listEventsReceived; eventIndex++) {
                /* Create a new dataset named after event index under
                 * globaltime group if it doesn't exist already. */
                nest.str("");
                nest.clear();
                nest << flavor << "-" << std::setfill('0') << std::setw(3) << eventIndex;
                datasetname = H5std_string(nest.str());
                createDataset = true;
                try {
                    std::cout << "Try to open dataset " << datasetname << std::endl;
                    dataset = new DataSet(globaltimegroup->openDataSet(datasetname));
                    createDataset = false;
                } catch(GroupIException error) {
                    if (!createOutfile) {
                        std::cerr << "ERROR: could not open dataset " << datasetname << " : file exception : " << std::endl;
                        error.printError();
                    } else {
                        //std::cout << "DEBUG: could not open dataset " << datasetname << " : " << std::endl;
                    }
                }
                if (createDataset) {
                    std::cout << "Create dataset " << datasetname << std::endl;
                    dataset = new DataSet(globaltimegroup->createDataSet(datasetname, listEventType, dataspace));
                    createDataset = false;
                }
            
                /* Read out fields from event for now */
                listEvent = &(eventData->listEvents[eventIndex]);
                channel = listEvent->channel;
                localtime = listEvent->localTime;
                charge = listEvent->adcValue;
                std::cout << "Saving data from " << digitizer << " channel " << channel << " localtime " << localtime << " charge " << charge << std::endl;
                dataset->write(listEvent, listEventType);

                if (dataset != NULL)
                    delete dataset;
            }
            /* Write data to disk after handling each globaltimegroup */
            outfile->flush(H5F_SCOPE_GLOBAL);
            if (globaltimegroup != NULL)
                delete globaltimegroup;
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

    /* Stop file writer and clean up */
    std::cout << "Stop file writer and clean up" << std::endl;

    /* Close UDP listener */
    if (socket != NULL)
        delete socket;

    /* Close output file */
    if (outfile != NULL) {
        std::cout << "Close outfile: " << outname << std::endl;
        delete outfile;
    }

    std::cout << "Shutting down." << std::endl;

    return 0;
}
