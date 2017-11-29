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
#include <sstream>
#include <fstream>
#include <string>
#include <ctime>

#include "H5Cpp.h"
using namespace H5;


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
        std::cout << "Usage: " << argv[0] << " [<config_file>] [<output_file>]" << std::endl;
        std::cout << "Reads in a partial/full configuration in <config_file> " << std::endl;
        std::cout << "and configures the hdf5writer accordingly. Then dumps " << std::endl;
        std::cout << "received data as HDF5 into <output_file>. " << std::endl;
        return -1;
    }

    /* Data format version - please increment on versiondata on layout changes */
    uint32_t versiondata[3] = {1, 0, 0};
    hsize_t versiondims[1] = {3};

    /* Helpers */
    uint32_t eventsReceived = 0, eventsWritten = 0;

    uint32_t eventIndex = 0, i = 0, j = 0;
    /* Dummy data */
    std::string digitizer, digitizerModel, digitizerID;
    uint32_t channel = 0, charge = 0 , globaltime = 0, localtime = 0;
    
    /* Path helpers */
    std::string configFileName;
    std::string outputFileName = "out.h5";

    /* Act on command-line arguments */
    if (argc > 1) {
        configFileName = std::string(argv[1]);
        std::cout << "Reading hdf5writer configuration from: " << configFileName << std::endl;
    } else {
        std::cout << "Using default hdf5writer configuration." << std::endl;
    }
    if (argc > 2) {
        outputFileName = std::string(argv[2]);
        std::cout << "Writing hdf5 formatted data to: " << outputFileName << std::endl;
    } else {
        std::cout << "Using default hdf5 output location: " << outputFileName << std::endl;
    }

    /* Prepare and start event handling */
    std::cout << "Setup hdf5writer" << std::endl;

    /* TODO: setup UDP listener */
    
    /* Prepare output file  and data sets */
    bool createOutfile = true, createGroup = true, createDataset = true;
    const H5std_string outname(outputFileName);
    H5std_string groupname, datasetname, versionname = H5std_string("version");
    std::stringstream nest;
    H5File *outfile = NULL;
    Group *rootgroup = NULL, *digitizergroup = NULL, *globaltimegroup = NULL;
    DataSet *dataset = NULL;
    DataSpace versionspace;
    Attribute *versionattr = NULL;
    try {
        /* TODO: should we truncate or just keep adding? */
        if (createOutfile) {
            std::cout << "Creating new outfile " << outname << std::endl;
            outfile = new H5File(outname, H5F_ACC_TRUNC);
        } else {    
            std::cout << "Opening existing outfile " << outname << std::endl;
            outfile = new H5File(outname, H5F_ACC_RDWR);
        }
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
        versionattr = new Attribute(rootgroup->createAttribute(versionname, PredType::STD_I32BE, versionspace));
        versionattr->write(PredType::NATIVE_INT, versiondata);
        delete versionattr;
        delete rootgroup;
    } catch(FileIException error) {
        std::cerr << "ERROR: could not open root group" << " : " << std::endl;
        error.printError();
    }

    /* TODO: switch to real data format */
    uint32_t data[EVENTFIELDS];
    const int RANK = 1;
    hsize_t dimsf[1];
    dimsf[0] = EVENTFIELDS;
    DataSpace dataspace(RANK, dimsf);
    IntType datatype(PredType::NATIVE_INT);
    datatype.setOrder(H5T_ORDER_LE);

    /* Turn off the auto-printing of errors - manual print instead. */
    Exception::dontPrint();

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
            /* TODO: actualy receive data here - fake for now */
            digitizerModel = "V1740D";
            digitizerID = "137";
            digitizer = "V1740D_137";

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
            globaltime = std::time(nullptr);
            eventsReceived = 1 + globaltime % 3;
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

            /* Loop over received events and create a dataset for each
             * of them. */
            for (i=0; i < eventsReceived; i++) {
                /* Create a new dataset named after event index under
                 * globaltime group if it doesn't exist already. */
                nest.str("");
                nest.clear();
                nest << "event-" << i;
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
                    dataset = new DataSet(globaltimegroup->createDataSet(datasetname, datatype, dataspace));
                    createDataset = false;
                }
            
                /* Fake event for now */
                channel = (i % 2) * 31;
                localtime = (globaltime + i) & 0xFFFF;
                charge = 242 + (localtime+i*13) % 100;
                std::cout << "Saving data from " << digitizer << " channel " << channel << " localtime " << localtime << " charge " << charge << std::endl;
                data[0] = channel;
                data[1] = localtime;
                data[2] = charge;
                dataset->write(data, PredType::NATIVE_INT);

                if (dataset != NULL)
                    delete dataset;
            }
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

    /* Stop file writer */
    std::cout << "Stop file writer" << std::endl;

    /* TODO: close UDP listener */

    /* Close output file */
    if (outfile != NULL) {
        std::cout << "Close outfile: " << outname << std::endl;
        delete outfile;
    }
    
    /* Clean up after all */
    std::cout << "Clean up after file writer" << std::endl;


    std::cout << "Shutting down." << std::endl;

    return 0;
}
