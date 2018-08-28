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
 * and send it out on UDP or dump to a file.
 *
 */

#include <iostream>
#include <chrono>
#include <thread>
#include <boost/program_options.hpp>
#include "interrupt.hpp"
#include "Digitizer.hpp"
#include "Configuration.hpp"
#include "DataHandler.hpp"
#include "DataWriter.hpp"
#include "DataWriterHDF5.hpp"
#include "DataWriterText.hpp"
#include "DataWriterNetwork.hpp"
#include "FileID.hpp"
#include "Timer.hpp"

namespace po = boost::program_options;

struct
{
    bool  textout = false;
    bool  hdf5out = false;
    bool  nullout = false;
    long  events  = -1;
    float time    = -1.0f;
    float split   = -1.0f;
    float stats   = -1.0f;
    int   verbose =  1;
    std::string* path = nullptr;
    std::string* basename = nullptr;
    std::string* network = nullptr;
    std::string* port = nullptr;
    std::string* outConfigFile = nullptr;
    std::vector<std::string> configFile;
} conf;

std::atomic<long> eventsFound{0};

inline static void printStats(const std::vector<Digitizer>& digitizers)
{
    for (const Digitizer& digitizer: digitizers)
    {
        std::cout << digitizer.name() << ": ";
        if (digitizer.active)
        {
            const Digitizer::Stats& stats = digitizer.getStats();
            std::cout << PRINTD(stats.eventsFound) << " events found, " <<
            PRINTD(stats.bytesRead) << " bytes read." << std::endl;
        } else {
            std::cout << "DEAD!" << std::endl;
        }
    }
}

int main(int argc, const char *argv[])
{

    try
    {
        po::options_description desc{"Usage: " + std::string(argv[0]) + " [<options>]"};
        desc.add_options()
                ("help,h", "Display help information")
                ("verbose,v", po::value<int>()->value_name("<level>")->default_value(conf.verbose), "Set program verbosity level.")
                ("events,e", po::value<int>()->value_name("<count>")->default_value(conf.events), "Stop acquisition after collecting <count> events")
                ("time,t", po::value<float>()->value_name("<seconds>")->default_value(conf.time), "Stop acquisition after <seconds> seconds")
                ("text,T", po::bool_switch(&conf.textout), "Output to text file.")
                ("hdf5,H", po::bool_switch(&conf.hdf5out), "Output to hdf5 file.")
                ("split,s", po::value<float>()->value_name("<seconds>")->default_value(conf.split), "Split output file every <seconds> seconds")
                ("stats", po::value<float>()->value_name("<seconds>")->default_value(conf.stats), "Print statistics every <seconds> seconds")
                ("path,p", po::value<std::string>()->value_name("<path>")->default_value(""), "Store data and other run information in local <path>.")
                ("basename,b", po::value<std::string>()->value_name("<name>")->default_value("jadaq-"), "Use <name> as the basename for file output.")
                ("network,N", po::value<std::string>()->value_name("<address>"), "Send data over network - address to bind to.")
                ("port,P", po::value<std::string>()->value_name("<port>")->default_value(Data::defaultDataPort), "Network port to bind to if sending over network")
                ("config_out", po::value<std::string>()->value_name("<file>"), "Read back device(s) configuration and write to <file>")
                ("config", po::value<std::vector<std::string> >()->value_name("<file>"), "Configuration file");
        po::positional_options_description pos;
        pos.add("config", -1);
        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).options(desc).positional(pos).run(), vm);
        po::notify(vm);

        if (vm.count("help"))
        {
            std::cout << desc << std::endl;
            return 0;
        }
        conf.verbose   = vm["verbose"].as<int>();
        if (vm.count("config"))
        {
            conf.configFile = vm["config"].as<std::vector<std::string>>();
            if (conf.configFile.size() != 1)
            {
                //TODO: Support multiple configurations files
                std::cerr << "More than one configuration file given only the first will be used." << std::endl;
            }
        } else
        {
            std::cerr << "No configuration file given!" << std::endl;
            return -1;
        }
        if (vm.count("config_out"))
        {
            conf.outConfigFile = new std::string(vm["config_out"].as<std::string>());
        }
        conf.path = new std::string(vm["path"].as<std::string>());
        conf.basename = new std::string(vm["basename"].as<std::string>());
        // add trailing slash to path (if given)
        if (!conf.path->empty() && *conf.path->rbegin() != '/')
            *conf.path += '/';
        conf.events = vm["events"].as<int>();
        conf.time   = vm["time"].as<float>();
        conf.split  = vm["split"].as<float>();
        conf.stats  = vm["stats"].as<float>();
        if (vm.count("network"))
        {
            conf.network = new std::string(vm["network"].as<std::string>());
            conf.port = new std::string(vm["port"].as<std::string>());
        }
        // We will use the Null data handlere if no other is selected
        conf.nullout = (!conf.textout && !conf.hdf5out && (conf.network == nullptr));
    }
    catch (const po::error &error)
    {
        std::cerr << error.what() << '\n';
        throw;
    }

    // get a unique run ID
    uuid runID;
    // prepare a run number
    FileID fileID;

    /* Read-in and write resulting digitizer configuration */
    std::string configFileName = conf.configFile[0];
    std::ifstream configFile(configFileName);
    if (!configFile.good())
    {
        std::cerr << "Could not open jadaq configuration file: " << configFileName << std::endl;
        return -1;
    }
    DEBUG(std::cout << "Reading digitizer configuration from" << configFileName << std::endl;)
    // NOTE: switch verbose (2nd) arg on here to enable conf warnings
    // TODO: implement a general verbose mode in sted of this
    Configuration configuration(configFile, conf.verbose > 1);
    configFile.close();

    if (conf.outConfigFile)
    {
        std::ofstream outFile(*conf.outConfigFile);
        if (outFile.good())
        {
            DEBUG(std::cout << "Writing current digitizer configuration to " << *conf.outConfigFile << std::endl;)
            configuration.write(outFile);
            outFile.close();
        } else
        {
            std::cerr << "Unable to open configuration out file: " << *conf.outConfigFile << std::endl;
        }
    }

    std::vector<Digitizer>& digitizers = configuration.getDigitizers();
    if (conf.verbose)
    {
        std::cout << "Setup " << digitizers.size() << " digitizer(s):" << std::endl;
        for (Digitizer &digitizer: digitizers)
        {
            std::cout << "\t" << digitizer.name() << std::endl;
        }
    }

    // TODO: move DataHandler creation to factory method in DataHandlerGeneric
    DataWriter dataWriter;
    if (conf.hdf5out)
    {
        dataWriter = new DataWriterHDF5(*conf.path, *conf.basename, conf.split>0.0f?fileID.toString():"");
    }
    else if (conf.textout)
    {
        dataWriter = new DataWriterText(*conf.path, *conf.basename, conf.split>0.0f?fileID.toString():"");
    }
    else if(conf.network != nullptr)
    {
      dataWriter = new DataWriterNetwork(*conf.network,*conf.port,runID.value());
    }
    else if (conf.nullout)
    {
        dataWriter = new DataWriterNull();
    }
    else
    {
        std::cerr << "No valid data handler." << std::endl;
        return -1;
    }
    for (Digitizer& digitizer: digitizers) {
        if (conf.verbose)
        {
            std::cout << "Start acquisition on digitizer " << digitizer.name() << std::endl;
        }
        digitizer.initialize(dataWriter);
        digitizer.startAcquisition();
        digitizer.active = true;
    }

    /* Set up interrupt handler */
    setup_interrupt_handler();

    // Setup IO service for timer
    std::atomic<bool> timeout{false};
    Timer* runtimer = nullptr;
    if (conf.time > 0.0f)
    {
        runtimer = new Timer{conf.time, [&timeout]() { timeout = true; }};
    }
    Timer* splittimer = nullptr;
    if (conf.split > 0.0f)
    {
        splittimer = new Timer{conf.split, [&dataWriter, &fileID]() { dataWriter.split((++fileID).toString()); }, true};
    }
    Timer* statstimer = nullptr;
    if (conf.stats > 0.0f)
    {
        statstimer = new Timer{conf.stats, [&digitizers]() { printStats(digitizers); }, true};
    }
    if (conf.verbose)
    {
        std::cout << "Running acquisition loop - Ctrl-C to interrupt" << std::endl;
    }
    long acquisitionStart = DataHandler::getTimeMsecs();
    while(true)
    {
        eventsFound = 0;
        for (Digitizer& digitizer: digitizers) {
            if (digitizer.active)
            {
                try { digitizer.acquisition(); }
                catch (std::exception &e)
                {
                    std::cerr << "ERROR: unexpected exception during acquisition: " << e.what() << std::endl;
                    digitizer.active = false;
                }
            }
            eventsFound += digitizer.getStats().eventsFound;
        }
        if (interrupt)
        {
            std::cout << "Caught interrupt - stop acquisition and clean up." << std::endl;
            break;
        }
        if (timeout)
        {
            std::cout << "Time out - stop acquisition and clean up." << std::endl;
            break;
        }
        if (conf.events >= 0 && eventsFound >= conf.events)
        {
            std::cout << "Collected requested events - stop acquisition and clean up." << std::endl;
            break;
        }
    }
    long acquisitionStop = DataHandler::getTimeMsecs();
    for (Digitizer& digitizer: digitizers)
    {
        if (conf.verbose)
        {
            std::cout << "Stop acquisition on digitizer " << digitizer.name() << std::endl;
        }
        digitizer.stopAcquisition();
    }
    if (conf.verbose)
    {
        std::cout << "Acquisition complete - shutting down." << std::endl;
    }
    if (runtimer)
    {
        delete runtimer;
    }
    if (statstimer)
    {
        statstimer->cancel();
        delete statstimer;
    }
    if (splittimer)
    {
        splittimer->cancel();
        delete splittimer;
    }
    /* Clean up after all digitizers: buffers, etc. */
    for (Digitizer& digitizer: digitizers)
    {
        digitizer.close();
    }
    digitizers.clear();
    if (conf.verbose)
    {
        double runtime = (acquisitionStop - acquisitionStart) / 1000.0;
        std::cout << "Acquisition ran for " << runtime << " seconds." << std::endl;
        std::cout << "Collecting " << eventsFound << " events." << std::endl;
        std::cout << "Resulting in a collection rate of " << eventsFound/runtime/1000.0 << " kHz." << std::endl;
    }
    return 0;
}
