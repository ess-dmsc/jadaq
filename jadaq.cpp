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

#include "Configuration.hpp"
#include "DataHandler.hpp"
#include "DataWriter.hpp"
#include "DataWriterNetwork.hpp"
#include "Digitizer.hpp"
//#include "Timer.hpp"
#include "interrupt.hpp"
#include <boost/program_options.hpp>
#include <chrono>
#include <iostream>
#include <queue>
#include <thread>
#include "xtrace.h"
#include "timer.h"

// #undef TRC_MASK
// #define TRC_MASK TRC_M_ALL
// #undef TRC_LEVEL
// #define TRC_LEVEL TRC_L_DEB

namespace po = boost::program_options;

struct {
  bool nullout = false;
  long events = -1;
  unsigned int time = 0xffffff; // many seconds
  unsigned int stats = 0xffffff; // many seconds
  int verbose = 1;
  std::string *path = nullptr;
  std::string *basename = nullptr;
  std::string *network = nullptr;
  std::string *port = nullptr;
  std::string *outConfigFile = nullptr;
  std::vector<std::string> configFile;
} conf;

struct {
  bool timeout{false};
  std::vector<Digitizer> * digarr;
} application_control;

static void printStats(const std::vector<Digitizer> &digitizers) {
  long eventsFound = 0;
  long bytesRead = 0;
  std::cout << std::setw(15) << "DIGITIZER"
            << "       " << PRINTHS(eventsFound, "Events")
            << PRINTHS(bytesRead, "Bytes") << std::endl;
  for (const Digitizer &digitizer : digitizers) {
    std::cout << std::setw(15) << digitizer.name() << ": ";
    if (digitizer.active) {
      std::cout << "ALIVE! ";
    } else {
      std::cout << "DEAD!! ";
    }
    const Digitizer::Stats &stats = digitizer.getStats();
    std::cout << PRINTD(stats.eventsFound) << PRINTD(stats.bytesRead)
              << std::endl;
    eventsFound += stats.eventsFound;
    bytesRead += stats.bytesRead;
  }
  std::cout << std::setw(15) << "TOTAL"
            << ":        " << PRINTD(eventsFound) << PRINTD(bytesRead)
            << std::endl
            << std::endl;
}



void service_thread() {
  XTRACE(MAIN, INF, "Starting service thread");
  Timer stoptimer;
  Timer stattimer;

  while (1) {
    if (stoptimer.timeus() >= conf.time * 1000000) {
      application_control.timeout = true;
      return;
    }

    if (stattimer.timeus() >= conf.stats * 1000000) {
      printStats(*application_control.digarr);
      stattimer.reset();
    }
    usleep(1000);
  }

}

int main(int argc, const char *argv[]) {
  try {
    po::options_description desc{"Usage: " + std::string(argv[0]) +
                                 " [<options>]"};
    desc.add_options()("help,h", "Display help information")(
        "verbose,v",
        po::value<int>()->value_name("<level>")->default_value(conf.verbose),
        "Set program verbosity level.")(
        "events,e",
        po::value<int>()->value_name("<count>")->default_value(conf.events),
        "Stop acquisition after collecting <count> events")(
        "time,t",
        po::value<float>()->value_name("<seconds>")->default_value(conf.time),
        "Stop acquisition after <seconds> seconds")(
        "stats",
        po::value<float>()->value_name("<seconds>")->default_value(conf.stats),
        "Print statistics every <seconds> seconds")(
        "path,p",
        po::value<std::string>()->value_name("<path>")->default_value(""),
        "Store data and other run information in local <path>.")(
        "basename,b",
        po::value<std::string>()->value_name("<name>")->default_value("jadaq-"),
        "Use <name> as the basename for file output.")(
        "network,N", po::value<std::string>()->value_name("<address>"),
        "Send data over network - address to bind to.")(
        "port,P", po::value<std::string>()->value_name("<port>")->default_value(
                      Data::defaultDataPort),
        "Network port to bind to if sending over network")(
        "config_out", po::value<std::string>()->value_name("<file>"),
        "Read back device(s) configuration and write to <file>")(
        "config", po::value<std::vector<std::string>>()->value_name("<file>"),
        "Configuration file");
    po::positional_options_description pos;
    pos.add("config", -1);
    po::variables_map vm;
    po::store(
        po::command_line_parser(argc, argv).options(desc).positional(pos).run(),
        vm);
    po::notify(vm);

    if (vm.count("help")) {
      std::cout << desc << std::endl;
      return 0;
    }
    conf.verbose = vm["verbose"].as<int>();
    if (vm.count("config")) {
      conf.configFile = vm["config"].as<std::vector<std::string>>();
      if (conf.configFile.size() != 1) {
        // TODO: Support multiple configurations files
        std::cerr << "More than one configuration file given only the first "
                     "will be used."
                  << std::endl;
      }
    } else {
      std::cerr << "No configuration file given!" << std::endl;
      return -1;
    }
    if (vm.count("config_out")) {
      conf.outConfigFile = new std::string(vm["config_out"].as<std::string>());
    }
    conf.path = new std::string(vm["path"].as<std::string>());
    conf.basename = new std::string(vm["basename"].as<std::string>());
    // add trailing slash to path (if given)
    if (!conf.path->empty() && *conf.path->rbegin() != '/')
      *conf.path += '/';
    conf.events = vm["events"].as<int>();
    conf.time = vm["time"].as<float>();
    conf.stats = vm["stats"].as<float>();
    if (vm.count("network")) {
      conf.network = new std::string(vm["network"].as<std::string>());
      conf.port = new std::string(vm["port"].as<std::string>());
    } else {
      conf.network = (std::string *)"127.0.0.1";
    }
  } catch (const po::error &error) {
    std::cerr << error.what() << '\n';
    throw;
  }

  // get a unique run ID
  /// \todo get rid of this? hardcode for now
  XTRACE(MAIN, ALW, "RunID currently constant 0xdeadbeef");
  uint64_t runID{0xdeadbeef};

  /* Read-in and write resulting digitizer configuration */
  std::string configFileName = conf.configFile[0];
  std::ifstream configFile(configFileName);
  if (!configFile.good()) {
    XTRACE(MAIN, ERR, "Could not open jadaq configuration file: %s", configFileName.c_str());
    return -1;
  }
  XTRACE(MAIN, DEB, "Reading digitizer configuration from %s", configFileName.c_str());
  // NOTE: switch verbose (2nd) arg on here to enable conf warnings
  // TODO: implement a general verbose mode in sted of this
  Configuration configuration(configFile, conf.verbose > 1);
  configFile.close();

  XTRACE(MAIN, INF, "Done reading configuration file");

  if (conf.outConfigFile) {
    std::ofstream outFile(*conf.outConfigFile);
    if (outFile.good()) {
      XTRACE(MAIN, DEB, "Writing current digitizer configuration to %s", (*conf.outConfigFile).c_str());
      configuration.write(outFile);
      outFile.close();
    } else {
      XTRACE(MAIN, ERR, "Unable to open configuration out file: ", (*conf.outConfigFile).c_str());
    }
  }

  XTRACE(MAIN, INF, "getDigitizers()");
  std::vector<Digitizer> &digitizers = configuration.getDigitizers();
  XTRACE(MAIN, INF, "Setup %d digitizer(s):", digitizers.size());

  for (Digitizer &digitizer : digitizers) {
      XTRACE(MAIN, INF, "digitizer: %s", digitizer.name().c_str());
  }

  /// \todo move DataHandler creation to factory method in DataHandlerGeneric
  DataWriter dataWriter;

  XTRACE(MAIN, INF, "Creating DataWriter for UDP");
  dataWriter = new DataWriterNetwork(*conf.network, *conf.port, runID);

  XTRACE(MAIN, INF, "Starting Acquisition");
  for (Digitizer &digitizer : digitizers) {
    XTRACE(MAIN, INF, "Start acquisition on digitizer %s", digitizer.name().c_str());
    digitizer.initialize(dataWriter);
    digitizer.startAcquisition();
    digitizer.active = true;
  }

  /* Set up interrupt handler */
  setup_interrupt_handler();

  /// setup stop timer and stat timer thread
  std::thread support(service_thread);
  support.detach();

  application_control.digarr = &digitizers;


  XTRACE(MAIN, INF, "Running acquisition loop - Ctrl-C to interrupt");

  long acquisitionStart = DataHandler::getTimeMsecs();
  long eventsFound = 0;
  while (true) {
    eventsFound = 0;
    for (Digitizer &digitizer : digitizers) {
      if (digitizer.active) {
        try {
          digitizer.acquisition();
        } catch (caen::Error &e) {
          XTRACE(MAIN, ERR, "ERROR: unexpected exception during acquisition: %s (%d)", e.what(), e.code());
          digitizer.active = false;
        }
      }
      eventsFound += digitizer.getStats().eventsFound;
    }
    if (interrupt) {
      XTRACE(MAIN, ALW, "Caught interrupt - stop acquisition and clean up.");
      break;
    }
    if (application_control.timeout) {
      XTRACE(MAIN, ALW, "Time out - stop acquisition and clean up.");
      break;
    }
    if (conf.events >= 0 && eventsFound >= conf.events) {
      XTRACE(MAIN, ALW, "Collected requested events - stop acquisition and clean up.");
      break;
    }
  }
  /// \todo replace by tsctimer and ustimer
  // for (Timer &timer : timers) {
  //   timer.cancel();
  // }

  long acquisitionStop = DataHandler::getTimeMsecs();
  for (Digitizer &digitizer : digitizers) {
    XTRACE(MAIN, INF, "Stop acquisition on digitizer %s", digitizer.name().c_str());
    digitizer.stopAcquisition();
  }
  XTRACE(MAIN, ALW, "Acquisition complete - shutting down.");
  /* Clean up after all digitizers: buffers, etc. */
  for (Digitizer &digitizer : digitizers) {
    digitizer.close();
  }
  digitizers.clear();

  double runtime = (acquisitionStop - acquisitionStart) / 1000.0;
  XTRACE(MAIN, ALW, "Acquisition ran for %d seconds.", runtime);
  XTRACE(MAIN, ALW, "Collecting %d events.", eventsFound);
  XTRACE(MAIN, ALW, "Resulting in a collection rate of %f kHz.", eventsFound / runtime / 1000.0);
  return 0;
}
