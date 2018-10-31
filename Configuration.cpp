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
 * Read and write digitizer configuration ini files using boost property
 * tree.
 *
 */

#include "Configuration.hpp"
#include "StringConversion.hpp"
#include <cstdint>
#include <iostream>
#include <regex>

Configuration::Configuration(std::ifstream &file, bool verbose) {
  setVerbose(verbose);
  pt::ini_parser::read_ini(file, in);
  apply();
}

std::vector<Digitizer> &Configuration::getDigitizers() { return digitizers; }

/*
 * Read configuration from digitizers and write it on stream
 */
void Configuration::write(std::ofstream &file) {
  pt::write_ini(file, readBack());
}

/*
 * Write the parsed tree back on stream
 */
void Configuration::writeInput(std::ofstream &file) { pt::write_ini(file, in); }

std::string to_string(const Configuration::Range &range) {
  std::stringstream ss;
  ss << range.first << '-' << range.last;
  return ss.str();
}

static pt::ptree rangeNode(Digitizer &digitizer, FunctionID id, int begin,
                           int end, bool verbose) {
  pt::ptree ptree;
  std::string prev;
  try {
    prev = digitizer.get(id, begin);
  } catch (caen::Error &e) {
    if (verbose) {
      std::cerr << "WARNING: " << digitizer.name()
                << " could not read configuration value [" << begin << "] for "
                << to_string(id) << ": " << e.what() << std::endl;
    }
    return ptree;
  }
  for (int i = begin + 1; i < end; ++i) {
    try {
      std::string cur(digitizer.get(id, i));
      if (cur != prev) {
        ptree.put(to_string(Configuration::Range(begin, i - 1)), prev);
        begin = i;
        prev = cur;
      } else {
        continue;
      }
    } catch (caen::Error &e) {
      if (verbose) {
        std::cerr << "WARNING: " << digitizer.name()
                  << " could not read configuration range [" << i << ":" << end
                  << "] for " << to_string(id) << ": " << e.what() << std::endl;
      }

      ptree.put(to_string(Configuration::Range(begin, i - 1)), prev);
      return ptree;
    } catch (std::runtime_error &e) {
      if (verbose) {
        // Internal helper init probably failed so we just skip it
        std::cerr << "WARNING: " << digitizer.name()
                  << " could not handle configuration range  [" << i << ":"
                  << end << "] for " << to_string(id) << ": " << e.what()
                  << std::endl;
      }
      return ptree;
    }
  }
  // We only get here if values are valid until end
  ptree.put(to_string(Configuration::Range(begin, end - 1)), prev);
  return ptree;
}

pt::ptree Configuration::readBack() {
  pt::ptree out;
  for (Digitizer &digitizer : digitizers) {
    pt::ptree dPtree;
    switch (digitizer.linkType) {
    case CAEN_DGTZ_USB:
      dPtree.put("USB", digitizer.linkNum);
      break;
    case CAEN_DGTZ_OpticalLink:
      dPtree.put("OPTICAL", digitizer.linkNum);
      break;
    default:
      std::cerr << "ERROR: Unsupported Link Type: " << digitizer.linkType
                << std::endl;
    }
    dPtree.put("VME", hex_string(digitizer.VMEBaseAddress));
    dPtree.put("CONET", digitizer.conetNode);

    for (FunctionID id = functionIDbegin(); id < functionIDend(); ++id) {
      if (!takeIndex(id)) {
        try {
          dPtree.put(to_string(id), digitizer.get(id));
        } catch (caen::Error &e) {
          // Function not supported so we just skip it
          if (getVerbose()) {
            std::cerr << "WARNING: " << digitizer.name()
                      << " could not read configuration for " << to_string(id)
                      << ": " << e.what() << std::endl;
          }
        } catch (std::runtime_error &e) {
          // Internal helper init probably failed so we just skip it
          if (getVerbose()) {
            std::cerr << "WARNING: " << digitizer.name()
                      << " could not handle configuration for " << to_string(id)
                      << ": " << e.what() << std::endl;
          }
        }
      } else {
        pt::ptree fPtree =
            rangeNode(digitizer, id, 0, digitizer.channels(), getVerbose());
        if (!fPtree.empty()) {
          dPtree.put_child(to_string(id), fPtree);
        }
      }
    }
    for (uint32_t reg : digitizer.getRegisters()) {
      dPtree.put(to_string(Register) + "[" + hex_string(reg) + "]",
                 digitizer.get(Register, reg));
    }
    out.put_child(digitizer.name(), dPtree);
  }
  return out;
}

static void configure(Digitizer &digitizer, pt::ptree &conf, bool verbose) {
  /* NOTE: it seems we need to force stop and reset for all
   * configuration settings to work. Most notably setDCOffset will
   * consitently fail with GenericError if we don't. */
  /* Stop any acquisition first
   * TODO Why is Acquisition running before configuration??
   * */
  digitizer.stopAcquisition();

  /* Reset Digitizer */
  digitizer.reset();

  for (auto &setting : conf) {
    FunctionID fid = functionID(setting.first);
    if (setting.second.empty()) // Setting without channel/group
    {
      try {
        digitizer.set(fid, setting.second.data());
      } catch (caen::Error &e) {
        std::cerr << "ERROR: " << digitizer.name() << " could not set"
                  << to_string(fid) << '(' << setting.second.data() << ") "
                  << e.what() << std::endl;
        throw;
      } catch (std::runtime_error &e) {
        if (verbose) {
          std::cout << "WARNING: " << digitizer.name()
                    << " ignoring attempt to set " << to_string(fid) << ": "
                    << e.what() << std::endl;
        }
      }
    } else // Setting with channel/group
    {
      for (auto &rangeSetting : setting.second) {
        assert(rangeSetting.second.empty());
        Configuration::Range range{rangeSetting.first};
        for (int i = range.begin(); i != range.end(); ++i) {
          try {
            digitizer.set(fid, i, rangeSetting.second.data());
          } catch (caen::Error &e) {
            std::cerr << "ERROR: " << digitizer.name() << " could not set"
                      << to_string(fid) << '(' << i << ", "
                      << rangeSetting.second.data() << ") " << e.what()
                      << std::endl;
            throw;
          } catch (std::runtime_error &e) {
            if (verbose) {
              std::cout << "WARNING: " << digitizer.name()
                        << " ignoring attempt to set " << to_string(fid) << ": "
                        << e.what() << std::endl;
            }
          }
        }
      }
    }
  }
}

void Configuration::apply() {
  for (auto &section : in) {
    std::string name = section.first;
    pt::ptree conf(section.second); // create local a copy we can modify
    if (conf.empty()) {
      continue; // Skip top level keys i.e. not in a [section]
    }
    int usb = -1;
    int optical = -1;
    uint32_t vme = 0;
    int conet = 0;
    usb = conf.get<int>("USB", -1);
    conf.erase("USB");
    optical = conf.get<int>("OPTICAL", -1);
    conf.erase("OPTICAL");
    vme = conf.get<uint32_t>("VME", 0);
    conf.erase("VME");
    conet = conf.get<int>("CONET", 0);
    conf.erase("CONET");
    Digitizer *digitizer = nullptr;
    if (usb < 0 && optical < 0) {
      std::cerr << "ERROR: [" << name << ']'
                << " contains neither USB nor OPTICAL number. One is REQUIRED."
                << std::endl;
      continue;
    } else if (usb >= 0 && optical >= 0) {
      std::cerr << "ERROR: [" << name << ']'
                << " contains both USB and OPTICAL number. Omly one is VALID"
                << std::endl;
      continue;
    }
    try {
      if (optical >= 0) {
        digitizers.emplace_back(CAEN_DGTZ_OpticalLink, optical, conet, vme);
      } else {
        digitizers.emplace_back(CAEN_DGTZ_USB, usb, conet, vme);
      }
      digitizer = &*digitizers.rbegin();
    } catch (caen::Error &e) {
      std::cerr << "ERROR: Unable to open digitizer [" << name
                << "]: " << e.what() << std::endl;
      throw;
    }
    configure(*digitizer, conf, getVerbose());
  }
}

Configuration::Range::Range(std::string s) {

  std::regex single("^(\\d+)$");
  std::regex hexsingle("^0[xX]([a-fA-F0-9]+)$");
  std::regex range("^(\\d+)-(\\d+)$");
  std::regex hexrange("^0[xX]([a-fA-F0-9]+)-0[xX]([a-fA-F0-9]+)$");
  std::smatch match;
  if (std::regex_search(s, match, single)) {
    first = last = std::stoi(match[1]);
  } else if (std::regex_search(s, match, hexsingle)) {
    first = last = stou(match[1], 0, 16);
  } else if (std::regex_search(s, match, range)) {
    first = std::stoi(match[1]);
    last = std::stoi(match[2]);
  } else if (std::regex_search(s, match, hexrange)) {
    first = stou(match[1], 0, 16);
    last = stou(match[2], 0, 16);
  } else {
    std::cerr << "Range helper found invalid range: " << s << std::endl;
    throw std::invalid_argument{"Not a valid range"};
  }
}
