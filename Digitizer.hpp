//
// Created by troels on 9/19/17.
//

#ifndef JADAQ_DIGITIZER_HPP
#define JADAQ_DIGITIZER_HPP

#include "FunctionID.hpp"
#include "caen.hpp"

#include <string>
#include <unordered_map>

class Digitizer
{
private:
    caen::Digitizer* digitizer;
    int usb_;
    uint32_t vme_;
public:
    Digitizer(int usb, uint32_t vme) : digitizer(caen::Digitizer::USB(usb,vme)), usb_(usb), vme_(vme) {}
    const std::string name() { return digitizer->modelName() + "_" + std::to_string(digitizer->serialNumber()); }
    const int usb() { return usb_; }
    const int vme() { return vme_; }
    void set(FunctionID functionID, std::string value);
    void set(FunctionID functionID, int index, std::string value);
    std::string get(FunctionID functionID);
    std::string get(FunctionID functionID, int index);
    caen::Digitizer* caen() { return digitizer; }
};


#endif //JADAQ_DIGITIZER_HPP
