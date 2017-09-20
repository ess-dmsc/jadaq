//
// Created by troels on 9/15/17.
//

#ifndef JADAQ_CONFIGURATION_HPP
#define JADAQ_CONFIGURATION_HPP

#include <fstream>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/info_parser.hpp>
#include "Digitizer.hpp"

namespace pt = boost::property_tree;

class Configuration {
private:
    pt::ptree ptree;
    std::vector<Digitizer> digitizers;
    void populatePtree();
    void apply();
public:
    Configuration(std::vector<Digitizer>&& digitizers_);
    Configuration(const std::vector<Digitizer>& digitizers_);
    Configuration(std::ifstream& file);
    std::vector<Digitizer> getDigitizers();
    void write(std::ofstream& file);
};


#endif //JADAQ_CONFIGURATION_HPP
