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
 * Write data to plain text file
 *
 */

#ifndef JADAQ_DATAHANDLERTEXT_HPP
#define JADAQ_DATAHANDLERTEXT_HPP

#include "DataFormat.hpp"
#include "container.hpp"
#include <cassert>
#include <fstream>
#include <iomanip>
#include <mutex>

class DataWriterText {
private:
  const std::string &pathname;
  const std::string &basename;

  std::fstream *file = nullptr;
  std::mutex mutex;

  void open(const std::string &id) {
    std::string filename = pathname + basename + id + ".txt";
    file = new std::fstream(filename, std::fstream::out);
    if (!file->is_open()) {
      throw std::runtime_error("Could not open text data file: \"" + filename +
                               "\"");
    }
    *file << "# runID: " << id << std::endl;
  }

  void close() {
    assert(file);
    file->close();
  }

public:
  DataWriterText(const std::string &pathname_, const std::string &basename_,
                 const std::string &&id)
      : pathname(pathname_), basename(basename_) {
    open(id);
  }

  ~DataWriterText() {
    mutex.lock();
    close();
    mutex.unlock();
  }

  void addDigitizer(uint32_t digitizerID) {
    mutex.lock();
    *file << "# digitizerID: " << digitizerID << std::endl;
    mutex.unlock();
  }

  static bool network() { return false; }

  void split(const std::string &id) {
    mutex.lock();
    close();
    open(id);
    mutex.unlock();
  }

  template <typename E>
  void operator()(const jadaq::buffer<E> *buffer, uint32_t digitizer,
                  uint64_t globalTimeStamp) {
    mutex.lock();
    *file << "#" << PRINTH(digitizer) << " ";
    E::headerOn(*file);
    *file << std::endl << "@" << globalTimeStamp << std::endl;
    for (const E &element : *buffer) {
      *file << " " << PRINTD(digitizer) << " " << element << "\n";
    }
    mutex.unlock();
  }
};

#endif // JADAQ_DATAHANDLERTEXT_HPP
