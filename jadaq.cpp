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
#include "DataWriterHDF5.hpp"
#include "DataWriterNetwork.hpp"
#include "DataWriterText.hpp"
#include "Digitizer.hpp"
//#include "Timer.hpp"
#include "interrupt.hpp"
#include <boost/program_options.hpp>
#include <chrono>
#include <iostream>
#include <queue>
#include <thread>
#include "runno.hpp"
#include "xtrace.h"
#include "timer.h"

// #undef TRC_MASK
// #define TRC_MASK TRC_M_ALL
// #undef TRC_LEVEL
// #define TRC_LEVEL TRC_L_INF

namespace po = boost::program_options;

struct {
  bool textout = false;
  bool hdf5out = false;
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

static void printStats(const std::vector<Digitizer> &digitizers, uint32_t elapsedms) {
  static uint64_t oldevents=0;
  static uint64_t oldbytes=0;
  static uint64_t oldreadouts=0;
  uint64_t eventsFound = 0;
  uint64_t bytesRead = 0;
  uint64_t readouts = 0;
  printf("   DIGITIZER                        Events                Bytes                Readouts\n");
  for (const Digitizer &digitizer : digitizers) {
    const Digitizer::Stats &stats = digitizer.getStats();
    printf("     %-10s: %6s            %15" PRIu64 "           %15" PRIu64 "           %15" PRIu64 "\n",
           digitizer.name().c_str(), digitizer.active ? "ALIVE!" : "DEAD!",
           stats.eventsFound, stats.bytesRead, stats.readouts);
    eventsFound += stats.eventsFound;
    bytesRead += stats.bytesRead;
    readouts += stats.readouts;
  }
  printf("     Total                         %15" PRIu64 "           %15" PRIu64 "           %15" PRIu64"\n",
         eventsFound, bytesRead, readouts);
  printf("     Total Rates                   %15ld/s         %15ld/s         %15ld/s\n\n",
         (eventsFound - oldevents)*1000/elapsedms,
         (bytesRead - oldbytes)*1000/elapsedms,
         (readouts - oldreadouts)*1000/elapsedms);
  oldevents = eventsFound;
  oldbytes = bytesRead;
  oldreadouts = oldreadouts;
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
      printStats(*application_control.digarr, stattimer.timeus()/1000);
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
        "hdf5,H", po::bool_switch(&conf.hdf5out), "Output to hdf5 file.")(
        "stats",
        po::value<float>()->value_name("<seconds>")->default_value(conf.stats),
        "Print statistics every <seconds> seconds")(
        "path,p",
        po::value<std::string>()->value_name("<path>")->default_value("."),
        "Store data and other run information in local <path>.")(
        "basename,b",
        po::value<std::string>()->value_name("<name>")->default_value("jadaq-"),
        "Use <name> as the basename for file output.")(
        "network,N", po::value<std::string>()->value_name("<address>"),
        "Send data over network - address to bind to.")(
        "port,P", po::value<std::string>()->value_name("<port>")->default_value("9000"),
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
    }
    // else {
    //   conf.network = new std::string("127.0.0.1");
    //   conf.port = new std::string(vm["port"].as<std::string>());
    // }
    // We will use the Null data handlere if no other is selected
    conf.nullout = (!conf.hdf5out && (conf.network == nullptr));

  } catch (const po::error &error) {
    std::cerr << error.what() << '\n';
    throw;
  }

  // prepare a run number
  runno runNumber;

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

  // read in run number stored in path (if any)
  if (runNumber.readFromPath(*conf.path)){
    XTRACE(MAIN, DEB, "Found run number %d at path '%s'", runNumber.value(), (*conf.path).c_str());
  } else {
    XTRACE(MAIN, WAR, "No run number found at path '%s' (will be set to zero)", (*conf.path).c_str());
  }
  // copy over configuration file
  std::stringstream dstName;
  dstName << *conf.path << *conf.basename << runNumber.toString() << ".cfg";
  std::ifstream  src(configFileName, std::ios::binary);
  std::ofstream  dst(dstName.str(), std::ios::binary);
  dst << src.rdbuf();
  if (!dst){
    std::cerr << "Error: could not copy config file to '" << *conf.path << "' -- please check the output path argument!" << std::endl;
    return -1;
  }
  // write out next run number to file
  runno nextRun(runNumber.value()+1);
  nextRun.writeToPath(*conf.path);

  XTRACE(MAIN, ALW, "Starting run %s", runNumber.toString().c_str());


  XTRACE(MAIN, INF, "getDigitizers()");
  std::vector<Digitizer> &digitizers = configuration.getDigitizers();
  XTRACE(MAIN, INF, "Setup %d digitizer(s):", digitizers.size());

  for (Digitizer &digitizer : digitizers) {
      XTRACE(MAIN, INF, "digitizer: %s", digitizer.name().c_str());
  }

  // TODO: move DataHandler creation to factory method in DataHandlerGeneric
  DataWriter dataWriter;

  if (conf.hdf5out) {
    XTRACE(MAIN, NOTE, "Creating DataWriter for HDF5");
    dataWriter = new DataWriterHDF5(*conf.path, *conf.basename,runNumber.toString().c_str());
  } else if (conf.network != nullptr) {
    XTRACE(MAIN, NOTE, "Creating DataWriter for UDP");
    dataWriter = new DataWriterNetwork(*conf.network, *conf.port, runNumber.value());
  } else if (conf.nullout) {
    XTRACE(MAIN, WAR, "Creating (dummy) DataWriter for to /dev/null");
    dataWriter = new DataWriterNull();
  } else {
    std::cerr << "No valid data handler." << std::endl;
    return -1;
  }
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

  Timer acquisitionTimer;
  uint64_t eventsFound = 0;
  uint64_t readouts = 0;
  SteadyTimer readoutTimer;
  while (true) {
    eventsFound = 0;
    for (Digitizer &digitizer : digitizers) {
      if (digitizer.active) {
        try {
          /* wait a certain amount of milliseconds between acquisition attempts to avoid
           potential hickups on the link */
          // NOTE: introduced to address issue #18, value determined experimentally
          // TODO: make this value configurable
          int gracePeriod = 50 - readoutTimer.elapsedms();
          if (gracePeriod > 0) std::this_thread::sleep_for(std::chrono::milliseconds(gracePeriod));
          readoutTimer.reset();
          digitizer.acquisition();
        } catch (caen::Error &e) {
          XTRACE(MAIN, ERR, "ERROR: unexpected exception during acquisition: %s (%d)", e.what(), e.code());
          digitizer.active = false;
        }
      }
      eventsFound += digitizer.getStats().eventsFound;
      readouts += digitizer.getStats().readouts;
    }
    if (interrupt) {
      XTRACE(MAIN, ALW, "Caught interrupt - stop acquisition and clean up.");
      break;
    }
    if (application_control.timeout) {
      XTRACE(MAIN, ALW, "Time out - stop acquisition and clean up.");
      break;
    }
    if (conf.events >= 0 && eventsFound >= static_cast<uint64_t>(conf.events)) {
      XTRACE(MAIN, ALW, "Collected requested events - stop acquisition and clean up.");
      break;
    }
  }

  auto elapsed = acquisitionTimer.timeus();
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

  XTRACE(MAIN, ALW, "Acquisition ran for %.2f seconds.", elapsed/1000000.0);
  XTRACE(MAIN, ALW, "Collecting %u events.", eventsFound);
  XTRACE(MAIN, ALW, "Resulting in a collection rate of %.2f kHz.", eventsFound / (elapsed / 1000.0));
  XTRACE(MAIN, ALW, "Total number of readout attempts: %u.", eventsFound);
  return 0;
}
