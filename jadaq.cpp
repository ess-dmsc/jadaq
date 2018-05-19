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

#include <boost/program_options.hpp>
#include <iostream>
#include "interrupt.hpp"
#include "Digitizer.hpp"
#include "Configuration.hpp"
#include "DataHandler.hpp"
#include "DataHandlerHDF5.hpp"
#include "DataHandlerText.hpp"

namespace po = boost::program_options;

struct
{
    bool  textout = false;
    bool  hdf5out = false;
    bool  nullout = false;
    bool  sort    = false;
    int   verbose =  1;
    int   events  = -1;
    float time    = -1.0;
    std::string* outConfigFile = nullptr;
    std::vector<std::string> configFile;
} conf;

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
                ("sort,s", po::bool_switch(&conf.sort), "Sort output before writing to file (only valid for file output).")
                ("text,T", po::bool_switch(&conf.textout), "Output to text file.")
                ("hdf5,H", po::bool_switch(&conf.hdf5out), "Output to hdf5 file.")
                ("config_out", po::value<std::string>()->value_name("<file>"), "Read back device(s) configuration and write to <file>")
                ("config", po::value<std::vector<std::string> >(), "Configuration file");
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
        conf.events = vm["events"].as<int>();
        conf.time   = vm["time"].as<float>();

        // We will use the Null data handlere if no other is selected
        conf.nullout = (!conf.textout && !conf.hdf5out);
    }
    catch (const po::error &error)
    {
        std::cerr << error.what() << '\n';
        throw;
    }

    // get a unique run ID
    uuid runID;

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
    Configuration configuration(configFile, false);
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

    // Extract a vector of all configured digitizers
    std::vector<Digitizer> digitizers;
    digitizers = configuration.getDigitizers();
    if (conf.verbose)
    {
        std::cout << "Setup " << digitizers.size() << " digitizer(s):" << std::endl;
        for (Digitizer &digitizer: digitizers)
        {
            std::cout << "\t" << digitizer.name() << std::endl;
        }
    }

    // TODO: move DataHandler creation to factory method in DataHandlerGeneric
    DataHandler<Data::ListElement422>* dataHandler;
    if (conf.hdf5out)
    {
        if (conf.sort)
            dataHandler = new DataHandlerHDF5<jadaq::set, Data::ListElement422>(runID);
        else
            dataHandler = new DataHandlerHDF5<jadaq::vector, Data::ListElement422>(runID);
    }
    else if (conf.textout)
    {
        if (conf.sort)
            dataHandler = new DataHandlerText<jadaq::set, Data::ListElement422>(runID);
        else
            dataHandler = new DataHandlerText<jadaq::vector, Data::ListElement422>(runID);
    }
    else
    {
        std::cerr << "No valid data handler specified." << std::endl;
        return -1;
    }
    for (Digitizer& digitizer: digitizers) {
        if (conf.verbose)
        {
            std::cout << "Start acquisition on digitizer " << digitizer.name() << std::endl;
        }
        digitizer.initialize(dataHandler);
        digitizer.startAcquisition();
    }

    /* Set up interrupt handler and start handling acquired data */
    setup_interrupt_handler();
    if (conf.verbose)
    {
        std::cout << "Running acquisition loop - Ctrl-C to interrupt" << std::endl;
    }
    while(true)
    {
        for (Digitizer& digitizer: digitizers) {
            try {
                digitizer.acquisition();
            } catch(std::exception& e) {
                std::cerr << "ERROR: unexpected exception during acquisition: " << e.what() << std::endl;
                throw;
            }
        }
        if(interrupt)
        {
            std::cout << "caught interrupt - stop file writer and clean up." << std::endl;
            break;
        }
    }
    for (Digitizer& digitizer: digitizers)
    {
        if (conf.verbose)
        {
            std::cout << "Stop acquisition on digitizer " << digitizer.name() << std::endl;
        }
        digitizer.stopAcquisition();
    }
    /* Clean up after all digitizers: buffers, etc. */
    for (Digitizer& digitizer: digitizers)
    {
        digitizer.close();
    }

    if (conf.verbose)
    {
        std::cout << "Acquisition complete - shutting down." << std::endl;
    }
    return 0;
}
