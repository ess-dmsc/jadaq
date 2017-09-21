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
            ptree.put(to_string(Configuration::Range(begin,i-1)),prev);
            return ptree;
        }
    }
    // We only get here if values are valid until end
    ptree.put(to_string(Configuration::Range(begin,end-1)),prev);
    return ptree;
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
                pt::ptree fPtree = rangeNode(digitizer,id,0,digitizer.caen()->channels());
                if (!fPtree.empty())
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

Configuration::Range::Range(std::string s)
{
    std::regex single("^(\\d+)$");
    std::regex range("^(\\d+)-(\\d+)$");
    std::smatch match;
    if (std::regex_search(s,match,single))
    {
        first = last = std::stoi(match[1]);
    }
    else if (std::regex_search(s,match,range))
    {
        first = std::stoi(match[1]);
        last = std::stoi(match[2]);
    }
    throw std::invalid_argument{"Not a valid range"};

}
