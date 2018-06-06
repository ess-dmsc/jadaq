/**
 * jadaq (Just Another DAQ)
 * Copyright (C) 2018  Troels Blum <troels@blum.dk>
 *
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

#include <boost/program_options.hpp>
#include <iostream>
#include "DataHandler.hpp"
#include "NetworkReceive.hpp"
#include "interrupt.hpp"

namespace po = boost::program_options;

struct
{
    bool  textout = false;
    bool  hdf5out = false;
    bool  nullout = false;
    bool  sort    = false;
    int   verbose =  1;
} conf;


int main(int argc, const char *argv[])
{
    std::string address;
    std::string port;
    try
    {
        po::options_description desc{"Usage: " + std::string(argv[0]) + " [<options>]"};
        desc.add_options()
                ("help,h", "Display help information")
                ("address,a", po::value<std::string>()->default_value(NetworkReceive::listenAll)->value_name("<address>"), "Address to bind to. Defaults all network interfaces")
                ("port,p", po::value<std::string>()->value_name("<port>")->default_value(Data::defaultDataPort), "Network port to bind to")
                ("verbose,v", po::value<int>()->value_name("<level>")->default_value(conf.verbose), "Set program verbosity level.")
                ("sort,s", po::bool_switch(&conf.sort), "Sort output before writing to file (only valid for file output).")
                ("text,T", po::bool_switch(&conf.textout), "Output to text file.")
                ("hdf5,H", po::bool_switch(&conf.hdf5out), "Output to hdf5 file.")

                ("backend,b", po::value<std::string>()->value_name("<file type>")->default_value("text"), "Storage back end. [text,hdf5]");

        po::variables_map vm;
        po::store(parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help"))
        {
            std::cout << desc << '\n';
            return 0;
        }
        address = vm["address"].as<std::string>();
        port = vm["port"].as<std::string>();

    }
    catch (const po::error &ex)
    {
        std::cerr << ex.what() << '\n';
    }
    NetworkReceive networkReceive(address,port);

    /* Set up interrupt handler and start handling acquired data */
    setup_interrupt_handler();
    std::cout << "Running file writer loop - Ctrl-C to interrupt" << std::endl;
    networkReceive.run(&interrupt);
    std::cout << "caught interrupt - stop file writer and clean up." << std::endl;
}
