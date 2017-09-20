//
// Created by troels on 9/15/17.
//

#include "Configuration.hpp"

Configuration::Configuration(std::ifstream& file)
{
    pt::ini_parser::read_ini(file, ptree);
}

std::vector<Digitizer> Configuration::getDigitizers()
{
    for (auto& section : ptree)
    {
        std::string name = section.first;
        const pt::ptree& conf = section.second;

    }

    return std::vector<Digitizer>();
}
