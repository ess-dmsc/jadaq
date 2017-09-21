//
// Created by troels on 9/15/17.
//

#include "Configuration.hpp"
#include <regex>
#include <iostream>

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
    pt::write_ini(file,ptree);
}

void Configuration::populatePtree()
{
    for (Digitizer& digitizer: digitizers)
    {
        pt::ptree dPtree;
         for (FunctionID id = functionIDbegin(); id < functionIDend(); ++id)
        {
            if (!needIndex(id))
            {
                try {
                    dPtree.put(std::to_string(id), digitizer.get(id));
                } catch (caen::Error& e)
                {
                    //Nothing to do
                }
            }
            else
            {
                pt::ptree fPtree;
                bool valid = false;
                for (int i = 0; i < digitizer.caen()->channels(); ++i)
                {
                    try {
                        fPtree.put(std::to_string(i), digitizer.get(id,i));
                        valid = true;
                    } catch (caen::Error& e)
                    {
                        //std::cout << "Tried to call get" << functionName(id) << " Caught: " << e.what() << std::endl;
                        break; //first time we fail we give up
                    }
                }
                if (valid)
                {
                    dPtree.put_child(std::to_string(id), fPtree);
                }
            }
        }
        ptree.put_child(digitizer.name(),dPtree);
    }
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
