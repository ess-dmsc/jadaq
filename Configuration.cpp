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
#include <regex>
#include <iostream>
#include <cstdint>

Configuration::Configuration(std::ifstream& file)
{
    pt::ini_parser::read_ini(file, in);
    apply();
}

std::vector<Digitizer> Configuration::getDigitizers()
{
    return digitizers;
}

/*
 * Read configuration from digitizers and write it on stream
 */
void Configuration::write(std::ofstream& file)
{
    pt::write_ini(file,readBack());
}

/*
 * Write the parsed tree back on stream
 */
void Configuration::writeInput(std::ofstream& file)
{
    pt::write_ini(file,in);
}

std::string to_string(const Configuration::Range& range)
{
    std::stringstream ss;
    ss << range.first << '-' << range.last;
    return ss.str();
}


static pt::ptree rangeNode(Digitizer& digitizer, FunctionID id, int begin, int end)
{
    pt::ptree ptree;
    std::string prev;
    try {
        prev = digitizer.get(id,begin);
    } catch (caen::Error& e)
    {
        std::cerr << "WARNING: " << digitizer.name() << " could not read configuration value [" << begin << "] for " <<
                  to_string(id) << ": " << e.what() << std::endl;
        return ptree;
    }
    for (int i = begin+1; i < end; ++i)
    {
        try {
            std::string cur(digitizer.get(id,i));
            if (cur != prev)
            {
                ptree.put(to_string(Configuration::Range(begin,i-1)), prev);
                begin = i;
                prev = cur;
            } else
            {
                continue;
            }
        } catch (caen::Error& e)
        {
            std::cerr << "WARNING: " << digitizer.name() << " could not read configuration range [" << i << ":" << end << "] for " << to_string(id) << ": " << e.what() << std::endl;
            ptree.put(to_string(Configuration::Range(begin,i-1)),prev);
            return ptree;
        } catch (std::runtime_error& e)
        {
            //Internal helper init probably failed so we just skip it
            std::cerr << "WARNING: " << digitizer.name() << " could not handle configuration range  [" << i << ":" << end << "] for " << to_string(id) << ": " << e.what() << std::endl;
            return ptree;
        }
    }
    // We only get here if values are valid until end
    ptree.put(to_string(Configuration::Range(begin,end-1)),prev);
    return ptree;
}

pt::ptree Configuration::readBack()
{
    pt::ptree out;
    for (Digitizer& digitizer: digitizers)
    {
        pt::ptree dPtree;
        dPtree.put("USB", digitizer.usb());
        if (digitizer.vme())
        {
            dPtree.put("VME", hex_string(digitizer.vme()));
        }
        for (FunctionID id = functionIDbegin(); id < functionIDend(); ++id)
        {
            if (!needIndex(id))
            {
                try {
                    dPtree.put(to_string(id), digitizer.get(id));
                } catch (caen::Error& e)
                {
                    //Function not supported so we just skip it
                    std::cerr << "WARNING: " << digitizer.name() << " could not read configuration for " << to_string(id) << ": " << e.what() << std::endl;
                } catch (std::runtime_error& e)
                {
                    //Internal helper init probably failed so we just skip it
                    std::cerr << "WARNING: " << digitizer.name() << " could not handle configuration for " << to_string(id) << ": " << e.what() << std::endl;
                }
            }
            else
            {
                pt::ptree fPtree = rangeNode(digitizer,id,0,digitizer.caen()->channels());
                if (!fPtree.empty())
                {
                    dPtree.put_child(to_string(id), fPtree);
                }
            }
        }
        out.put_child(digitizer.name(),dPtree);
    }
    return out;
}

static void configure(Digitizer& digitizer, pt::ptree& conf)
{
    for (auto& setting : conf)
    {
        FunctionID fid = functionID(setting.first);
        if (setting.second.empty()) //Setting without channel/group
        {
            try { digitizer.set(fid,setting.second.data()); }
            catch (caen::Error& e)
            {
                std::cerr << "ERROR: " << digitizer.name() << " could not set" << to_string(fid) << '(' << setting.second.data() <<
                          ") " << e.what() << std::endl;
                throw;
            }
        }
        else //Setting with channel/group
        {
            for (auto& rangeSetting: setting.second)
            {
                assert(rangeSetting.second.empty());
                Configuration::Range range{rangeSetting.first};
                for (int i = range.begin(); i != range.end(); ++i)
                {
                    try { digitizer.set(fid, i, rangeSetting.second.data()); }
                    catch (caen::Error& e)
                    {
                        std::cerr << "ERROR: " << digitizer.name() << " could not set" << to_string(fid) <<
                                  '(' << i << ", " << rangeSetting.second.data() << ") " << e.what() << std::endl;
                        throw;
                    }
                }
            }
        }
    }
}

void Configuration::apply()
{
    for (auto& section : in)
    {
        std::string name = section.first;
        pt::ptree conf(section.second); //create local a copy we can modify
        if (conf.empty()) {
            continue; //Skip top level keys i.e. not in a [section]
        }
        int usb;
        uint32_t vme = 0;
        try {
            usb = conf.get<int>("USB");
            conf.erase("USB");
        } catch (pt::ptree_error& e)
        {
            std::cerr << "ERROR: [" << name << ']' <<" does not contain USB number. REQUIRED" << std::endl;
            continue;
        }
        vme = conf.get<uint32_t>("VME",0);
        conf.erase("VME");
        Digitizer* digitizer = nullptr;
        try {
            digitizers.push_back(Digitizer(usb,vme));
            digitizer = &*digitizers.rbegin();
        } catch (caen::Error& e)
        {
            std::cerr << "ERROR: Unable to open digitizer [" << name << ']' << std::endl;
            continue;
        }
        configure(*digitizer,conf);
    }
}

Configuration::Range::Range(std::string s) {
    std::regex single("^(\\d+)$");
    std::regex range("^(\\d+)-(\\d+)$");
    std::smatch match;
    if (std::regex_search(s, match, single)) {
        first = last = std::stoi(match[1]);
    } else if (std::regex_search(s, match, range)) {
        first = std::stoi(match[1]);
        last = std::stoi(match[2]);
    } else {
        throw std::invalid_argument{"Not a valid range"};
    }
}
