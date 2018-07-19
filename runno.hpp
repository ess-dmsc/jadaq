/**
 * jadaq (Just Another DAQ)
 *
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
 * class for handling a run number and storing it to disk
 *
 */
#ifndef JADAQ_RUNNO_HPP
#define JADAQ_RUNNO_HPP

#include <cstdint>
#include <iostream>
#include <sstream>
#include <iomanip>

class runno {
private:
  uint64_t val;
  uint8_t width;
public:
  runno() : val(0), width(5) {}
  runno(uint64_t v): val(v), width(5) {}
  uint64_t value() const {return val;}
  void setWidth(uint8_t w){width=w;}
  runno& operator=(const runno& that) {val = that.value(); return *this;}
  runno& operator++() { val++; return *this; }
  bool readFromPath(std::string pathname);
  void writeToPath(std::string pathname);
  std::string toString() const {
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(width) << val;
    return ss.str();
  }
};


#endif
