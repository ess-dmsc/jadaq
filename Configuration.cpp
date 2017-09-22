//
// Created by troels on 9/15/17.
//

#include "Configuration.hpp"
#include <regex>
#include <iostream>

Configuration::Configuration(std::ifstream& file)
{
    pt::ini_parser::read_ini(file, in);
    apply();
}

std::vector<Digitizer> Configuration::getDigitizers()
{

    return std::vector<Digitizer>();
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

pt::ptree Configuration::readBack()
{
    pt::ptree out;
    for (Digitizer& digitizer: digitizers)
    {
        pt::ptree dPtree;
        dPtree.put("USB", digitizer.usb());
        if (digitizer.vme())
        {
            dPtree.put("VME", digitizer.vme());
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

void Configuration::apply()
{
    for (auto& section : in)
    {
        std::string name = section.first;
        const pt::ptree& conf = section.second;
        if (conf.empty())
            continue;
        int usb;
        uint32_t vme = 0;
        try {
            usb = conf.get<int>("USB");
        } catch (pt::ptree_error& e)
        {
            std::cerr << '[' << name << ']' <<" does not contain USB number. REQUIRED" << std::endl;
            throw;
        }
        try { vme = conf.get<int>("VME"); } catch (...) {}
        try {
            digitizers.push_back(Digitizer(usb,vme));
        } catch (caen::Error& e)
        {
            std::cerr << "Unable to open digitizer [" << name << ']' << usb << ',' << vme << std::endl;
        }



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
