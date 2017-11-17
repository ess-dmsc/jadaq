/* Helpers to use conf file on format used by CAEN demos */

#ifndef JADAQ_CAENCONF_HPP
#define JADAQ_CAENCONF_HPP

#include <iostream>
#include <fstream>
//#include <CAENDigitizer.h>
#include "caen.hpp"

#define ACQMODE_LIST    0
#define ACQMODE_MIXED   1

#define ENABLE_TEST_PULSE 1

#define CONNECTION_TYPE_USB    0
#define CONNECTION_TYPE_OPT    1
#define CONNECTION_TYPE_AUTO   255

/* Data structures */

typedef struct 
{
        uint32_t ConnectionType;
        uint32_t ConnectionLinkNum;
        uint32_t ConnectionConetNode;
        uint32_t ConnectionVMEBaseAddress;
        uint32_t RecordLength;   
        uint32_t PreTrigger;     
        uint32_t ActiveChannel;
        uint32_t GateWidth[8];
        uint32_t PreGate;
        uint32_t ChargeSensitivity;
        uint32_t FixedBaseline;
        uint32_t BaselineMode;
        uint32_t TrgMode;
        uint32_t TrgSmoothing;
        uint32_t TrgHoldOff;
        uint32_t TriggerThreshold[64];
        uint32_t DCoffset[8];
        uint32_t AcqMode;
        uint32_t NevAggr;
        uint32_t PulsePol;
        uint32_t EnChargePed;
        uint32_t SaveList;
        uint32_t DisTrigHist;
        uint32_t DisSelfTrigger;
        uint32_t EnTestPulses;
        uint32_t TestPulsesRate;
        uint32_t DefaultTriggerThr;
        uint32_t EnableExtendedTimeStamp;
        uint64_t ChannelTriggerMask;
} BoardParameters;

/* Helper functions to load, set and dump digitizer configuration */
void set_default_parameters(BoardParameters *params);
int load_configuration_from_file(char * fname, BoardParameters *params);
int setup_parameters(BoardParameters *params, char *fname);
int configure_digitizer(int handle, caen::Digitizer *digitizer, BoardParameters *params);
int dump_configuration(int handle, char *fname);

#endif //JADAQ_CAENCONF_HPP
