//
// Created by troels on 9/15/17.
//

#ifndef JADAQ_CONFIGURATION_HPP
#define JADAQ_CONFIGURATION_HPP

#include <fstream>
#include "ini_parser.hpp"
#include <boost/property_tree/info_parser.hpp>
#include "Digitizer.hpp"

namespace pt = boost::property_tree;

class Configuration
{
private:
    pt::ptree ptree;
    std::vector<Digitizer> digitizers;
    void populatePtree();
    void apply();
public:
    explicit Configuration(std::vector<Digitizer>&& digitizers_);
    explicit Configuration(const std::vector<Digitizer>& digitizers_);
    explicit Configuration(std::ifstream& file);
    std::vector<Digitizer> getDigitizers();
    void write(std::ofstream& file);
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
