//
// Created by troels on 9/15/17.
//

#include "Configuration.hpp"
#include <regex>

Configuration::Configuration(std::ifstream& file)
{
    pt::ini_parser::read_ini(file, ptree);
}

Configuration::Configuration(std::vector<Digitizer>&& digitizers_)
        : digitizers(digitizers_)
{ populatePtree(); }

Configuration::Configuration(const std::vector<Digitizer>& digitizers_)
        : digitizers(digitizers_)
{ populatePtree(); }

std::vector<Digitizer> Configuration::getDigitizers()
{

    return std::vector<Digitizer>();
}

void Configuration::write(std::ofstream& file)
{
    for (Digitizer& digitizer: digitizers)
    {
        file << '[' << digitizer.name() << "]\n";
    }
}

void Configuration::populatePtree()
{
    //TODO
}

void Configuration::apply()
{
    for (auto& section : ptree)
    {
        std::string name = section.first;
        const pt::ptree& conf = section.second;
        // TODO
    }
}

static std::pair<int,int> range(std::string s)
{
    std::regex single("^(\\d+)$");
    std::regex range("^(\\d+)-(\\d+)$");
    std::smatch match;
    if (std::regex_search(s,match,single))
    {
        int i = std::stoi(match[1]);
        return std::make_pair(i,i);
    }
    else if (std::regex_search(s,match,range))
    {
        return std::make_pair(std::stoi(match[1]),std::stoi(match[2]));
    }
    throw std::invalid_argument{"Not a valid range"};
};
