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

#include "runno.hpp"
#include <fstream>

bool runno::readFromPath(std::string pathname){
  // load runno from file 'run.no' inside specified path
  if (!pathname.empty() && *pathname.rbegin() != '/')
    pathname += '/';
  std::string fname = pathname+"run.no";
  std::ifstream file(fname.c_str(), std::ifstream::in);
  std::string result;
  if (file.is_open()) {
    std::getline(file, result);
    if (file.fail()) {
      throw std::runtime_error("Error reading run number from file " + fname);
    }
    // update value
    val = std::stoi(result);
  } else {
    // could not open file
    return false;
  }
  return true;
}

void runno::writeToPath(std::string pathname){
  // writes runno value to 'run.no' file in the specified path
  if (!pathname.empty() && *pathname.rbegin() != '/')
    pathname += '/';
  std::string fname = pathname+"run.no";
  std::ofstream file(fname.c_str(), std::ifstream::out);
  if (!file.is_open())
    throw std::runtime_error("Unable to open file " + fname + " for writing run number");
  file << val << std::endl;
  if (file.fail())
    throw std::runtime_error("Error writing run number to file " + fname);
}

