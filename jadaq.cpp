#include <iostream>
#include <CAENDigitizerType.h>
#include "caen.hpp"
#include <CAENComm.h>

int main() {
    caen::Digitizer* digitizer = caen::Digitizer::USB(0);
    digitizer->reset();
    std::cout << "I am talking to a " << digitizer->modelName() << ", serial: " << digitizer->serialNumber() << std::endl;
    std::cout << "ROC_FW " << digitizer->ROCfirmwareRel() << std::endl;
    std::cout << "AMC_FW " << digitizer->AMCfirmwareRel() << std::endl;
    std::cout << "DPPFirmware type" << digitizer->getDPPFirmwareType() << std::endl;
    std::cout << "ADCbits " << digitizer->ADCbits() << std::endl;
    std::cout << "Channels " << digitizer->channels() << std::endl;
    digitizer->setRecordLength(192*16);
    std::cout << "Record length:  " << digitizer->getRecordLength() << std::endl;
    digitizer->setGroupEnableMask(0x1);
    std::cout << "Group mask:  0x" << std::hex << digitizer->getGroupEnableMask() << std::dec << std::endl;
    digitizer->setMaxNumEventsBLT(1);
    std::cout << "Max Events:  " << digitizer->getMaxNumEventsBLT() << std::endl;
    digitizer->writeRegister(0x8004,1<<3);
    std::cout << "Board Config: 0x" << std::hex << digitizer->readRegister(0x8000) << std::dec << std::endl;
    digitizer->setDecimationFactor(1);
    std::cout << "Decimation Factor: " << digitizer->getDecimationFactor() << std::endl;
    digitizer->setPostTriggerSize(50);
    std::cout << "Post trigger size: " << digitizer->getPostTriggerSize() << std::endl;
    std::cout << "IO level: " << (digitizer->getIOlevel()?"TTL":"NIM") << std::endl;
    std::cout << "Acquisition mode: " << digitizer->getAcquisitionMode() << std::endl;
    digitizer->setExternalTriggerMode(CAEN_DGTZ_TRGMODE_DISABLED);
    std::cout << "External trigger mode: " << digitizer->getExternalTriggerMode() << std::endl;

    for (uint32_t group = 0; group < 4; ++group)
    {
        std::cout << "Group " << group << " DC offset: " << digitizer->getGroupDCOffset(group) << std::endl;
        digitizer->setGroupSelfTrigger(group, CAEN_DGTZ_TRGMODE_ACQ_ONLY);
        std::cout << "Group " << group << " self trigger: " << digitizer->getGroupSelfTrigger(group) << std::endl;
        digitizer->setGroupTriggerThreshold(group,100);
        std::cout << "Group " << group << " trigger treshold: " << digitizer->getGroupTriggerThreshold(group) << std::endl;
        group?digitizer->setChannelGroupMask(group,0xff):digitizer->setChannelGroupMask(group,0xff);
        std::cout << "Group " << group << " channel mask: 0x" << std::hex << digitizer->getChannelGroupMask(group) << std::dec << std::endl;
        std::cout << "Group " << group << " trigger polarity: " << digitizer->getTriggerPolarity(group) << std::endl;
    }

    std::cout << "-----------------------------------------------------------------" << std::endl;

    caen::ReadoutBuffer buffer = digitizer->mallocReadoutBuffer();
    std::cout << "Buffer size: " << buffer.size << std::endl;
    digitizer->startAcquisition();
    std::cout << "Started acquisition" << std::endl;
    for (int i = 0; i < 1; ++i)
    {
        digitizer->sendSWtrigger();
        std::cout << "Sent trigger" << std::endl;
        digitizer->readData(buffer, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT);
        std::cout << "Read data" << std::endl;
        std::cout << "Buffer header 0: " << std::hex << *((uint32_t *) buffer.data) << std::dec <<std::endl;
        std::cout << "Buffer header 2: " << std::hex << *(((uint32_t *) buffer.data) + 2) << std::dec << std::endl;
        //uint32_t nEvents = digitizer->getNumEvents(buffer);
        uint32_t nEvents = 1;
        std::cout << "Got " << nEvents << " events." << std::endl;
        std::cout << "Buffer size after: " << buffer.size << "\tBuffer address: " << (void*) buffer.data << std::endl;
        CAEN_DGTZ_UINT16_EVENT_t* event = (CAEN_DGTZ_UINT16_EVENT_t*)digitizer->mallocEvent();
        for (uint32_t e = 0; e < nEvents; ++e)
        {
            caen::EventInfo info = digitizer->getEventInfo(buffer,e);
            std::cout << "event info " << e << ": " <<
                      "\tEventSize: " << info.EventSize <<
                      "\tBoardId: " << info.BoardId <<
                      "\tPattern: " << info.Pattern <<
                      "\tChannelMask: " << info.ChannelMask <<
                      "\tEventCounter: " << info.EventCounter <<
                      "\tTriggerTimeTag: " << info.TriggerTimeTag <<
                      "\tdata: " << (void*)info.data << std::endl;
            digitizer->decodeEvent(info,event);
            for (size_t ch = 0; ch < MAX_UINT16_CHANNEL_SIZE; ++ch)
            {
                std::cout << "\t chSize[" << ch << "]: " << event->ChSize[ch] << " - " << event->DataChannel[ch] <<  std::endl << "\t\t";
                for (size_t che = 0; che < event->ChSize[ch]; ++che )
                {
                    std::cout << event->DataChannel[ch][che] << ",";
                }
                std::cout << std::endl;
            }

        }
        digitizer->freeEvent(event);
    }
    digitizer->stopAcquisition();
    std::cout << "Stopped acquisition" << std::endl;
    delete(digitizer);


    return 0;
}