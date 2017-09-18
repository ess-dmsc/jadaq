//
// Created by troels on 9/15/17.
//

#include "Configuration.hpp"

Configuration::Configuration(std::ifstream& file)
{
    pt::ini_parser::read_ini(file, ptree);
}
