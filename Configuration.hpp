//
// Created by troels on 9/15/17.
//

#ifndef JADAQ_CONFIGURATION_HPP
#define JADAQ_CONFIGURATION_HPP

#include <fstream>
#include <boost/property_tree/ini_parser.hpp>
#include "Digitizer.hpp"

namespace pt = boost::property_tree;

class Configuration {
private:
    pt::ptree ptree;
public:
    Configuration(std::ifstream& file);
    std::vector<Digitizer> getDigitizers();
};


#endif //JADAQ_CONFIGURATION_HPP
