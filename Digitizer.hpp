//
// Created by troels on 9/19/17.
//

#ifndef JADAQ_DIGITIZER_HPP
#define JADAQ_DIGITIZER_HPP


#include "caen.hpp"

class Digitizer
{
private:
    caen::Digitizer* digitizer;
public:
    Digitizer(caen::Digitizer* digitizer_) : digitizer(digitizer_) {}
};


#endif //JADAQ_DIGITIZER_HPP
