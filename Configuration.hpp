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
 * Read and write digitizer configuration ini files using boost property
 * tree.
 *
 */

#ifndef JADAQ_CONFIGURATION_HPP
#define JADAQ_CONFIGURATION_HPP

#include <fstream>
#include "ini_parser.hpp"
#include "Digitizer.hpp"

namespace pt = boost::property_tree;

class Configuration
{
private:
    pt::ptree in;
    std::vector<Digitizer> digitizers;
    pt::ptree readBack();
    void apply();
public:
    explicit Configuration(std::ifstream& file);
    std::vector<Digitizer> getDigitizers();
    void write(std::ofstream& file);
    void writeInput(std::ofstream& file);
    class Range
    {
    private:
        int first;
        int last;
    public:
        Range() : first(0), last(-1) {}
        Range(int first_, int last_) : first(first_), last(last_) {}
        explicit Range(std::string);
        int begin() { return first; }
        int end() { return last+1; }
        friend std::string to_string(const Configuration::Range& range);
    };
};

std::string to_string(const Configuration::Range& range);

#endif //JADAQ_CONFIGURATION_HPP
