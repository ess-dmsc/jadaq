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
 * Class for handling file id's for file splitting
 *
 */

#ifndef JADAQ_FILEID_HPP
#define JADAQ_FILEID_HPP

#include <iomanip>
#include <sstream>

class FileID {
private:
  int id;
  int width;

public:
  FileID() : id(0), width(5) {}
  FileID(int id_, int width_) : id(id_), width(width_) {}
  FileID &operator++() {
    ++id;
    return *this;
  }
  std::string toString() const {
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(width) << id;
    return ss.str();
  }
};

static inline std::string to_string(const FileID &id) { return id.toString(); }
static inline std::ostream &operator<<(std::ostream &os, const FileID &id) {
  os << id.toString();
  return os;
}

#endif // JADAQ_FILEID_HPP
