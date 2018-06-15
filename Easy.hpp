//
// Created by troels on 6/13/18.
//

#ifndef JADAQ_EASY_HPP
#define JADAQ_EASY_HPP


#include <string>
#include <vector>
#include <boost/any.hpp>
#include <map>
#include <assert.h>
#include <iostream>

/**
 * @class EasyBase
 * @brief For user-friendly configuration of various bit masks.
 *
 * Base class to handle all the translation between named variables
 * and bit masks. All actual such mask helpers should just inherit
 * from it and implment the specific variable names and specifications.
 *
 * @param EasyBase::mask
 * Initialize from bit mask
 */
class EasyBase
{
protected:
    /* We save variables in a standard map using boost::any values
     * to allow simple variable lookup based on string name but
     * variable value types. The map also allows easy iteration
     * through all variable names and values. */
    typedef std::map<const std::string, boost::any> var_map;
    var_map variables;
    typedef std::pair<std::string, std::pair<uint8_t, uint8_t>> layout_entry;
    typedef std::vector<layout_entry> var_layout;
    var_layout layout;
    /* We save original mask if constructed from one, so that we can
     * use it to only change the relevant mask bit defined and not
     * touch any reserved bits that may or may not need to remain
     * untouched.
     */
    uint32_t origMask = 0x0;
    /**
     * @brief unpack at most 8 bits from 32 bit mask
     * @param mask:
     * bit mask
     * @param bits:
     * number of bits to unpack
     * @param offset:
     * offset to the bits to unpack, i.e. how many bits to right-shift
     */
    const uint8_t unpackBits(const uint32_t mask, const uint8_t bits, const uint8_t offset) const
    {
        assert(bits <= 8);
        assert(bits + offset <= 32);
        return (const uint8_t)((mask >> offset) & ((1 << bits) - 1));
    }
    /**
     * @brief pack at most 8 bits into 32 bit mask
     * @param value:
     * value to pack
     * @param bits:
     * number of bits to pack
     * @param offset:
     * offset to the packed bits, i.e. how many bits to left-shift
     */
    const uint32_t packBits(const uint8_t value, const uint8_t bits, const uint8_t offset) const
    {
        assert(bits <= 8);
        assert(bits + offset <= 32);
        return (const uint32_t)((value & ((1 << bits) - 1)) << offset);
    }

    /* force small integers to full unsigned int for proper printing */
    template<typename T>
    const std::string ui_to_string(const T v) const
    {
        std::stringstream ss;
        ss << unsigned(v);
        return ss.str();
    }
    /* Shared helpers since one constructor cannot reuse the other */
    virtual void initLayout() {
        /* Default layout helper - do nothing */
    };
    virtual void autoInit(const uint32_t mask) {
        uint8_t val;
        for (const auto &keyVal : layout) {
            val = (unpackBits(mask, keyVal.second.first, keyVal.second.second));
            variables[keyVal.first] = (const uint8_t)val;
        }
        /* Make sure parsing is correct */
        if (mask != toBits()) {
            std::cerr << "WARNING: mismatch between mask " << mask << " and parsed value " << toBits() << " in autoInit of " << getClassName() << std::endl;
            std::cerr << variables.size() << " variables, " << layout.size() << " layouts" << std::endl;
            throw std::runtime_error(std::string("autoInit failed for: ")+getClassName());
        }
    };
    void construct() {
        /* Default construct helper - just call initLayout */
        initLayout();
    };
    virtual void constructFromMask(const uint32_t mask) {
        /* Generic construct helper - using possibly overriden
         * layout helper to unpack mask. */
        /* NOTE: a number of masks carry values in the reserved parts.
         *       It's not clear if we break things if we blindly truncate
         *       those during set. So we save the original mask when
         *       constructed from mask and only pack our defined
         *       valid bits into that saved mask during toBits().
         */
        origMask = mask;
        initLayout();
        autoInit(mask);
    };
public:
    virtual const std::string getClassName() const { return "EasyBase"; };
    EasyBase()
    {
        /* Default constructor - do nothing */
        construct();
    }
    EasyBase(const uint32_t mask)
    {
        /* Construct from bit mask - override in children if needed */
        constructFromMask(mask);
    }
    ~EasyBase()
    {
        /* Do nothing */
    }
    /* NOTE: variable get/set helpers restricted to only declared names */
    virtual const uint8_t getValue(const std::string name) const
    {
        if (variables.find(name) != variables.end()) {
            return boost::any_cast<uint8_t>(variables.at(name));
        }
        std::cerr << "No such variable: " << name << std::endl;
        throw std::invalid_argument(std::string("No such variable: ")+name);
    }
    virtual void setValue(const std::string name, uint8_t val)
    {
        /* TODO: add value check or enforce based on bits in layout? */
        if (variables.find(name) != variables.end()) {
            variables[name] = (const uint8_t)val;
        }
        std::cerr << "No such variable: " << name << std::endl;
        throw std::invalid_argument(std::string("No such variable: ")+name);
    }
    /* Convert to low-level bit mask in line with docs */
    virtual const uint32_t toBits() const
    {
        /* Use saved mask from init if available - it's 0 otherwise */
        uint32_t mask = origMask;
        /* NOTE: we must use variables.at() rather than variables[] here
         * to avoid 'argument discards qualifiers' error during compile */
        for (const auto &keyVal : layout) {
            mask |= packBits(boost::any_cast<uint8_t>(variables.at(keyVal.first)), keyVal.second.first, keyVal.second.second);
        }
        return mask;
    }
    /* Convert to (constant) configuration doc string */
    virtual const std::string toConfHelpString(const std::string name, bool header=false) const
    {
        std::stringstream ss;
        if (header) {
            ss << "### Format for " << name << " is:" << std::endl;
        }
        ss << '{';
        int i = 0;
        /* NOTE: map does not preserve insert order so we use layout
         * for ordering. */
        for (const auto &keyVal : layout) {
            /* NOTE: some registers have forced values that we need
             * to keep track of but not show user. */
            if ((keyVal.first).substr(0, 12) == "__reserved__") {
                continue;
            }
            // No comma before first entry
            if (i > 0) {
                ss << ",";
            }
            ss << keyVal.first;
            i++;
        }
        ss << '}';
        if (header) {
            ss << std::endl;
        }
        return ss.str();
    }
    /* Convert to (constant) configuration string */
    virtual const std::string toConfValueString() const
    {
        /* NOTE: map does not preserve insert order so we use layout
         * for ordering. */
        std::stringstream ss;
        ss << '{';
        int i = 0;
        for (const auto &keyVal : layout) {
            /* NOTE: some registers have forced values that we need
             * to keep track of but not show user. */
            if ((keyVal.first).substr(0, 12) == "__reserved__") {
                continue;
            }
            /* NOTE: we must use variables.at rather than variables[] here
             * to avoid 'argument discards qualifiers' compile problems */
            uint8_t val = boost::any_cast<uint8_t>(variables.at(keyVal.first));
            // No comma before first entry
            if (i > 0) {
                ss << ",";
            }
            ss << ui_to_string(val);
            i++;
        }
        ss<< '}';
        return ss.str();
    }
    /* Convert to (constant) configuration string */
    virtual const std::string toConfString() const
    {
        std::stringstream ss;
        ss << toConfValueString() << " # " << toConfHelpString(getClassName());
        return ss.str();
    }
}; // class EasyBase


/**
 * @class EasyBoardConfiguration
 * @brief For user-friendly configuration of Board Configuration mask.
 *
 * This register contains general settings for the board
 * configuration.
 *
 * @param EasyBoardConfiguration::triggerOverlapSetting
 * Trigger Overlap Setting (default value is 0).\n
 * When two acquisition windows are overlapped, the second trigger
 * can be either accepted or rejected. Options are:\n
 * 0 = Trigger Overlapping Not Allowed (no trigger is accepted until
 * the current acquisition window is finished);\n
 * 1 = Trigger Overlapping Allowed (the current acquisition window
 * is prematurely closed by the arrival of a new trigger).\n
 * NOTE: it is suggested to keep this bit cleared in case of using a
 * DPP firmware.
 * @param EasyBoardConfiguration::testPatternEnable
 * Test Pattern Enable (default value is 0).\n
 * This bit enables a triangular (0<-->3FFF) test wave to be
 * provided at the ADCs input for debug purposes. Options are:\n
 * 0 = disabled\n
 * 1 = enabled.
 * @param EasyBoardConfiguration::selfTriggerPolarity
 * Self-trigger Polarity (default value is 0).\n
 * Options are:\n
 * 0 = Positive (the self-trigger is generated upon the input pulse
 * overthreshold)\n
 * 1 = Negative (the self-trigger is generated upon the input pulse
 * underthreshold).
 */
class EasyBoardConfiguration : public EasyBase
{
protected:
    /* Shared base since one constructor cannot reuse the other */
    /*
     * EasyBoardConfiguration fields:
     * trigger overlap setting in [1], test pattern enable in [3],
     * (reserved forced 1) in [4], self-trigger polarity in [6].
     */
    virtual void initLayout() override
    {
        layout = {
                {"triggerOverlapSetting", {(const uint8_t)1, (const uint8_t)1}},
                {"testPatternEnable", {(const uint8_t)1, (const uint8_t)3}},
                {"__reserved__0_", {(const uint8_t)1, (const uint8_t)4}},
                {"selfTriggerPolarity", {(const uint8_t)1, (const uint8_t)6}}
        };
    }
    /* NOTE: use inherited generic constructFromMask(mask) */
    void construct(const uint8_t triggerOverlapSetting, const uint8_t testPatternEnable, const uint8_t selfTriggerPolarity) {
        initLayout();
        variables = {
                {"triggerOverlapSetting", (const uint8_t)(triggerOverlapSetting & 0x1)},
                {"testPatternEnable", (const uint8_t)(testPatternEnable & 0x1)},
                {"__reserved__0_", (const uint8_t)(0x1)},
                {"selfTriggerPolarity", (const uint8_t)(selfTriggerPolarity & 0x1)}
        };
    };
public:
    virtual const std::string getClassName() const override { return "EasyBoardConfiguration"; };
    /* Construct using default values from docs */
    EasyBoardConfiguration(const uint8_t triggerOverlapSetting, const uint8_t testPatternEnable, const uint8_t selfTriggerPolarity)
    {
        construct(triggerOverlapSetting, testPatternEnable, selfTriggerPolarity);
    }
    /* Construct from low-level bit mask in line with docs */
    EasyBoardConfiguration(const uint32_t mask)
    {
        constructFromMask(mask);
    }
}; // class EasyBoardConfiguration


/**
 * @class EasyDPPBoardConfiguration
 * @brief For user-friendly configuration of DPP Board Configuration mask.
 *
 * This register contains general settings for the DPP board
 * configuration.
 *
 * @param EasyDPPBoardConfiguration::individualTrigger
 * Individual trigger: must be 1
 * @param EasyDPPBoardConfiguration::analogProbe
 * Analog Probe: Selects which signal is associated to the Analog
 * trace in the readout data. Options are:\n
 * 00: Input\n
 * 01: Smoothed Input\n
 * 10: Baseline\n
 * 11: Reserved.
 * @param EasyDPPBoardConfiguration::waveformRecording
 * Waveform Recording: enables the data recording of the
 * waveform. The user must define the number of samples to be saved
 * in the Record Length 0x1n24 register. Options are:\n
 * 0: disabled\n
 * 1: enabled.
 * @param EasyDPPBoardConfiguration::extrasRecording
 * Extras Recording: when enabled the EXTRAS word is saved into the
 * event data. Refer to the ”Channel Aggregate Data Format” chapter
 * of the DPP User Manual for more details about the EXTRAS
 * word. Options are:\n
 * 0: disabled\n
 * 1: enabled.
 * @param EasyDPPBoardConfiguration::timeStampRecording
 * Time Stamp Recording: must be 1
 * @param EasyDPPBoardConfiguration::chargeRecording
 * Charge Recording: must be 1
 * @param EasyDPPBoardConfiguration::externalTriggerMode
 * External Trigger mode. The external trigger mode on TRG-IN
 * connector can be used according to the following options:\n
 * 00: Trigger\n
 * 01: Veto\n
 * 10: An -Veto\n
 * 11: Reserved.
 */
class EasyDPPBoardConfiguration : public EasyBase
{
protected:
    /* Shared base since one constructor cannot reuse the other */
    /*
     * EasyDPPBoardConfiguration fields:
     * (reserved forced 1) in [4], individual trigger in [8],
     * analog probe in [12:13], waveform recording in [16],
     * extras recording in [17], time stamp recording in [18],
     * charge recording in [19], external trigger mode in [20:21].
     */
    virtual void initLayout() override
    {
        layout = {
                {"__reserved__0_", {(const uint8_t)1, (const uint8_t)4}},
                {"individualTrigger", {(const uint8_t)1, (const uint8_t)8}},
                {"analogProbe", {(const uint8_t)2, (const uint8_t)12}},
                {"waveformRecording", {(const uint8_t)1, (const uint8_t)16}},
                {"extrasRecording", {(const uint8_t)1, (const uint8_t)17}},
                {"timeStampRecording", {(const uint8_t)1, (const uint8_t)18}},
                {"chargeRecording", {(const uint8_t)1, (const uint8_t)19}},
                {"externalTriggerMode", {(const uint8_t)2, (const uint8_t)20}}
        };
    }
    /* NOTE: use inherited generic constructFromMask(mask) */
    void construct(const uint8_t individualTrigger, const uint8_t analogProbe, const uint8_t waveformRecording, const uint8_t extrasRecording, const uint8_t timeStampRecording, const uint8_t chargeRecording, const uint8_t externalTriggerMode) {
        initLayout();
        variables = {
                {"__reserved__0_", (const uint8_t)(0x1)},
                {"individualTrigger", (const uint8_t)(individualTrigger & 0x1)},
                {"analogProbe", (const uint8_t)(analogProbe & 0x3)},
                {"waveformRecording", (const uint8_t)(waveformRecording & 0x1)},
                {"extrasRecording", (const uint8_t)(extrasRecording & 0x1)},
                {"timeStampRecording", (const uint8_t)(timeStampRecording & 0x1)},
                {"chargeRecording", (const uint8_t)(chargeRecording & 0x1)},
                {"externalTriggerMode", (const uint8_t)(externalTriggerMode & 0x3)}
        };
    };
public:
    virtual const std::string getClassName() const override { return "EasyDPPBoardConfiguration"; };
    /* Construct using default values from docs */
    EasyDPPBoardConfiguration(const uint8_t individualTrigger, const uint8_t analogProbe, const uint8_t waveformRecording, const uint8_t extrasRecording, const uint8_t timeStampRecording, const uint8_t chargeRecording, const uint8_t externalTriggerMode)
    {
        construct(individualTrigger, analogProbe, waveformRecording, extrasRecording, timeStampRecording, chargeRecording, externalTriggerMode);
    }
    /* Construct from low-level bit mask in line with docs */
    EasyDPPBoardConfiguration(const uint32_t mask)
    {
        constructFromMask(mask);
    }
}; // class EasyDPPBoardConfiguration


/**
 * @class EasyAcquisitionControl
 * @brief For user-friendly configuration of Acquisition Control mask.
 *
 * This register manages the acquisition settings.
 *
 * @param EasyAcquisitionControl::startStopMode
 * Start/Stop Mode Selection (default value is 00). Options are:\n
 * 00 = SW CONTROLLED. Start/stop of the run takes place on software
 * command by setting/resetting bit[2] of this register\n
 * 01 = S-IN/GPI CONTROLLED (S-IN for VME, GPI for Desktop/NIM). If
 * the acquisition is armed (i.e. bit[2] = 1), then the acquisition
 * starts when S-IN/GPI is asserted and stops when S-IN/GPI returns
 * inactive. If bit[2] = 0, the acquisition is always off\n
 * 10 = FIRST TRIGGER CONTROLLED. If the acquisition is armed
 * (i.e. bit[2] = 1), then the run starts on the first trigger pulse
 * (rising edge on TRG-IN); this pulse is not used as input trigger,
 * while actual triggers start from the second pulse. The stop of
 * Run must be SW controlled (i.e. bit[2] = 0)\n
 * 11 = LVDS CONTROLLED (VME only). It is like option 01 but using
 * LVDS (RUN) instead of S-IN.\n
 * The LVDS can be set using registers 0x811C and 0x81A0.
 * @param EasyAcquisitionControl::acquisitionStartArm
 * Acquisition Start/Arm (default value is 0).\n
 * When bits[1:0] = 00, this bit acts as a Run Start/Stop. When
 * bits[1:0] = 01, 10, 11, this bit arms the acquisition and the
 * actual Start/Stop is controlled by an external signal. Options
 * are:\n
 * 0 = Acquisition STOP (if bits[1:0]=00); Acquisition DISARMED (others)\n
 * 1 = Acquisition RUN (if bits[1:0]=00); Acquisition ARMED (others).
 * @param EasyAcquisitionControl::triggerCountingMode
 * Trigger Counting Mode (default value is 0). Through this bit it
 * is possible to count the reading requests from channels to mother
 * board. The reading requests may come from the following options:\n
 * 0 = accepted triggers from combination of channels\n
 * 1 = triggers from combination of channels, in addition to TRG-IN
 * and SW TRG.
 * @param EasyAcquisitionControl::memoryFullModeSelection
 * Memory Full Mode Selection (default value is 0). Options are:\n
 * 0 = NORMAL. The board is full whenever all buffers are full\n
 * 1 = ONE BUFFER FREE. The board is full whenever Nb-1 buffers are
 * full, where Nb is the overall number of buffers in which the
 * channel memory is divided.
 * @param EasyAcquisitionControl::pLLRefererenceClockSource
 * PLL Reference Clock Source (Desktop/NIM only). Default value is 0.\n
 * Options are:\n
 * 0 = internal oscillator (50 MHz)\n
 * 1 = external clock from front panel CLK-IN connector.\n
 * NOTE: this bit is reserved in case of VME boards.
 * @param EasyAcquisitionControl::lVDSIOBusyEnable
 * LVDS I/O Busy Enable (VME only). Default value is 0.\n
 * The LVDS I/Os can be programmed to accept a Busy signal as input,
 * or to propagate it as output. Options are:\n
 * 0 = disabled\n
 * 1 = enabled.\n
 * NOTE: this bit is supported only by VME boards and meaningful
 * only if the LVDS new features are enabled (bit[8]=1 of register
 * 0x811C). Register 0x81A0 should also be configured for nBusy/nVeto.
 * @param EasyAcquisitionControl::lVDSVetoEnable
 * LVDS I/O Veto Enable (VME only). Default value is 0.\n
 * The LVDS I/Os can be programmed to accept a Veto signal as input,
 * or to transfer it as output. Options are:\n
 * 0 = disabled (default)\n
 * 1 = enabled.\n
 * NOTE: this bit is supported only by VME boards and meaningful
 * only if the LVDS new features are enabled (bit[8]=1 of register
 * 0x811C). Register 0x81A0 should also be configured for nBusy/nVeto.
 * @param EasyAcquisitionControl::lVDSIORunInEnable
 * LVDS I/O RunIn Enable Mode (VME only). Default value is 0.\n
 * The LVDS I/Os can be programmed to accept a RunIn signal as
 * input, or to transfer it as output. Options are:\n
 * 0 = starts on RunIn level (default)\n
 * 1 = starts on RunIn rising edge.\n
 * NOTE: this bit is supported only by VME boards and meaningful
 * only if the LVDS new features are enabled (bit[8]=1 of register
 * 0x811C). Register 0x81A0 must also be configured for nBusy/nVeto.
 */
class EasyAcquisitionControl : public EasyBase
{
protected:
    /* Shared base since one constructor cannot reuse the other */
    /*
     * EasyAcquisitionControl fields:
     * start/stop mode [0:1], acquisition start/arm in [2],
     * trigger counting mode in [3], memory full mode selection in [5],
     * PLL reference clock source in [6], LVDS I/O busy enable in [8],
     * LVDS veto enable in [9], LVDS I/O RunIn enable in [11].
     */
    virtual void initLayout() override
    {
        layout = {{"startStopMode", {(const uint8_t)2, (const uint8_t)0}},
                  {"acquisitionStartArm", {(const uint8_t)1, (const uint8_t)2}},
                  {"triggerCountingMode", {(const uint8_t)1, (const uint8_t)3}},
                  {"memoryFullModeSelection", {(const uint8_t)1, (const uint8_t)5}},
                  {"pLLRefererenceClockSource", {(const uint8_t)1, (const uint8_t)6}},
                  {"lVDSIOBusyEnable", {(const uint8_t)1, (const uint8_t)8}},
                  {"lVDSVetoEnable", {(const uint8_t)1, (const uint8_t)9}},
                  {"lVDSIORunInEnable", {(const uint8_t)1, (const uint8_t)11}}
        };
    }
    /* NOTE: use inherited generic constructFromMask(mask) */
    void construct(const uint8_t startStopMode, const uint8_t acquisitionStartArm, const uint8_t triggerCountingMode, const uint8_t memoryFullModeSelection, const uint8_t pLLRefererenceClockSource, const uint8_t lVDSIOBusyEnable, const uint8_t lVDSVetoEnable, const uint8_t lVDSIORunInEnable) {
        initLayout();
        variables = {
                {"startStopMode", (const uint8_t)(startStopMode & 0x3)},
                {"acquisitionStartArm", (const uint8_t)(acquisitionStartArm & 0x1)},
                {"triggerCountingMode", (const uint8_t)(triggerCountingMode & 0x1)},
                {"memoryFullModeSelection", (const uint8_t)(memoryFullModeSelection & 0x1)},
                {"pLLRefererenceClockSource", (const uint8_t)(pLLRefererenceClockSource & 0x1)},
                {"lVDSIOBusyEnable", (const uint8_t)(lVDSIOBusyEnable & 0x1)},
                {"lVDSVetoEnable", (const uint8_t)(lVDSVetoEnable & 0x1)},
                {"lVDSIORunInEnable", (const uint8_t)(lVDSIORunInEnable & 0x1)}
        };
    };
public:
    virtual const std::string getClassName() const override { return "EasyAcquisitionControl"; };
    /* Construct using default values from docs */
    EasyAcquisitionControl(const uint8_t startStopMode, const uint8_t acquisitionStartArm, const uint8_t triggerCountingMode, const uint8_t memoryFullModeSelection, const uint8_t pLLRefererenceClockSource, const uint8_t lVDSIOBusyEnable, const uint8_t lVDSVetoEnable, const uint8_t lVDSIORunInEnable)
    {
        construct(startStopMode, acquisitionStartArm, triggerCountingMode, memoryFullModeSelection, pLLRefererenceClockSource, lVDSIOBusyEnable, lVDSVetoEnable, lVDSIORunInEnable);
    }
    /* Construct from low-level bit mask in line with docs */
    EasyAcquisitionControl(const uint32_t mask)
    {
        constructFromMask(mask);
    }
}; // class EasyAcquisitionControl


/**
 * @class EasyDPPAcquisitionControl
 * @brief For user-friendly configuration of Acquisition Control mask.
 *
 * This register manages the acquisition settings.
 *
 * @param EasyDPPAcquisitionControl::startStopMode
 * Start/Stop Mode Selection (default value is 00). Options are:\n
 * 00 = SW CONTROLLED. Start/stop of the run takes place on software
 * command by setting/resetting bit[2] of this register\n
 * 01 = S-IN/GPI CONTROLLED (S-IN for VME, GPI for Desktop/NIM). If
 * the acquisition is armed (i.e. bit[2] = 1), then the acquisition
 * starts when S-IN/GPI is asserted and stops when S-IN/GPI returns
 * inactive. If bit[2] = 0, the acquisition is always off\n
 * 10 = FIRST TRIGGER CONTROLLED. If the acquisition is armed
 * (i.e. bit[2] = 1), then the run starts on the first trigger pulse
 * (rising edge on TRG-IN); this pulse is not used as input trigger,
 * while actual triggers start from the second pulse. The stop of
 * Run must be SW controlled (i.e. bit[2] = 0)\n
 * 11 = LVDS CONTROLLED (VME only). It is like option 01 but using
 * LVDS (RUN) instead of S-IN.\n
 * The LVDS can be set using registers 0x811C and 0x81A0.
 * @param EasyDPPAcquisitionControl::acquisitionStartArm
 * Acquisition Start/Arm (default value is 0).\n
 * When bits[1:0] = 00, this bit acts as a Run Start/Stop. When
 * bits[1:0] = 01, 10, 11, this bit arms the acquisition and the
 * actual Start/Stop is controlled by an external signal. Options
 * are:\n
 * 0 = Acquisition STOP (if bits[1:0]=00); Acquisition DISARMED (others)\n
 * 1 = Acquisition RUN (if bits[1:0]=00); Acquisition ARMED (others).
 * @param EasyDPPAcquisitionControl::triggerCountingMode
 * Trigger Counting Mode (default value is 0). Through this bit it
 * is possible to count the reading requests from channels to mother
 * board. The reading requests may come from the following options:\n
 * 0 = accepted triggers from combination of channels\n
 * 1 = triggers from combination of channels, in addition to TRG-IN
 * and SW TRG.
 * @param EasyDPPAcquisitionControl::pLLRefererenceClockSource
 * PLL Reference Clock Source (Desktop/NIM only). Default value is 0.\n
 * Options are:\n
 * 0 = internal oscillator (50 MHz)\n
 * 1 = external clock from front panel CLK-IN connector.\n
 * NOTE: this bit is reserved in case of VME boards.
 * @param EasyDPPAcquisitionControl::lVDSIOBusyEnable
 * LVDS I/O Busy Enable (VME only). Default value is 0.\n
 * The LVDS I/Os can be programmed to accept a Busy signal as input,
 * or to propagate it as output. Options are:\n
 * 0 = disabled\n
 * 1 = enabled.\n
 * NOTE: this bit is supported only by VME boards and meaningful
 * only if the LVDS new features are enabled (bit[8]=1 of register
 * 0x811C). Register 0x81A0 should also be configured for nBusy/nVeto.
 * @param EasyDPPAcquisitionControl::lVDSVetoEnable
 * LVDS I/O Veto Enable (VME only). Default value is 0.\n
 * The LVDS I/Os can be programmed to accept a Veto signal as input,
 * or to transfer it as output. Options are:\n
 * 0 = disabled (default)\n
 * 1 = enabled.\n
 * NOTE: this bit is supported only by VME boards and meaningful
 * only if the LVDS new features are enabled (bit[8]=1 of register
 * 0x811C). Register 0x81A0 should also be configured for nBusy/nVeto.
 * @param EasyDPPAcquisitionControl::lVDSIORunInEnable
 * LVDS I/O RunIn Enable Mode (VME only). Default value is 0.\n
 * The LVDS I/Os can be programmed to accept a RunIn signal as
 * input, or to transfer it as output. Options are:\n
 * 0 = starts on RunIn level (default)\n
 * 1 = starts on RunIn rising edge.\n
 * NOTE: this bit is supported only by VME boards and meaningful
 * only if the LVDS new features are enabled (bit[8]=1 of register
 * 0x811C). Register 0x81A0 must also be configured for nBusy/nVeto.
 */
class EasyDPPAcquisitionControl : public EasyBase
{
protected:
    /* Shared base since one constructor cannot reuse the other */
    /*
     * EasyDPPAcquisitionControl fields:
     * start/stop mode [0:1], acquisition start/arm in [2],
     * trigger counting mode in [3], PLL reference clock
     * source in [6], LVDS I/O busy enable in [8],
     * LVDS veto enable in [9], LVDS I/O RunIn enable in [11].
     */
    virtual void initLayout() override
    {
        layout = {{"startStopMode", {(const uint8_t)2, (const uint8_t)0}},
                  {"acquisitionStartArm", {(const uint8_t)1, (const uint8_t)2}},
                  {"triggerCountingMode", {(const uint8_t)1, (const uint8_t)3}},
                  {"pLLRefererenceClockSource", {(const uint8_t)1, (const uint8_t)6}},
                  {"lVDSIOBusyEnable", {(const uint8_t)1, (const uint8_t)8}},
                  {"lVDSVetoEnable", {(const uint8_t)1, (const uint8_t)9}},
                  {"lVDSIORunInEnable", {(const uint8_t)1, (const uint8_t)11}}
        };
    }
    /* NOTE: use inherited generic constructFromMask(mask) */
    void construct(const uint8_t startStopMode, const uint8_t acquisitionStartArm, const uint8_t triggerCountingMode, const uint8_t pLLRefererenceClockSource, const uint8_t lVDSIOBusyEnable, const uint8_t lVDSVetoEnable, const uint8_t lVDSIORunInEnable) {
        initLayout();
        variables = {
                {"startStopMode", (const uint8_t)(startStopMode & 0x3)},
                {"acquisitionStartArm", (const uint8_t)(acquisitionStartArm & 0x1)},
                {"triggerCountingMode", (const uint8_t)(triggerCountingMode & 0x1)},
                {"pLLRefererenceClockSource", (const uint8_t)(pLLRefererenceClockSource & 0x1)},
                {"lVDSIOBusyEnable", (const uint8_t)(lVDSIOBusyEnable & 0x1)},
                {"lVDSVetoEnable", (const uint8_t)(lVDSVetoEnable & 0x1)},
                {"lVDSIORunInEnable", (const uint8_t)(lVDSIORunInEnable & 0x1)}
        };
    };
public:
    virtual const std::string getClassName() const override { return "EasyDPPAcquisitionControl"; };
    /* Construct using default values from docs */
    EasyDPPAcquisitionControl(const uint8_t startStopMode, const uint8_t acquisitionStartArm, const uint8_t triggerCountingMode, const uint8_t pLLRefererenceClockSource, const uint8_t lVDSIOBusyEnable, const uint8_t lVDSVetoEnable, const uint8_t lVDSIORunInEnable)
    {
        construct(startStopMode, acquisitionStartArm, triggerCountingMode, pLLRefererenceClockSource, lVDSIOBusyEnable, lVDSVetoEnable, lVDSIORunInEnable);
    }
    /* Construct from low-level bit mask in line with docs */
    EasyDPPAcquisitionControl(const uint32_t mask)
    {
        constructFromMask(mask);
    }
}; // class EasyDPPAcquisitionControl


/**
 * @class EasyAcquisitionStatus
 * @brief For user-friendly configuration of Acquisition Status mask.
 *
 * This register monitors a set of conditions related to the
 * acquisition status.
 *
 * @param EasyAcquisitionStatus::acquisitionStatus
 * Acquisition Status. It reflects the status of the acquisition and
 * drivers the front panel ’RUN’ LED. Options are:\n
 * 0 = acquisition is stopped (’RUN’ is off)\n
 * 1 = acquisition is running (’RUN’ lits).
 * @param EasyAcquisitionStatus::eventReady
 * Event Ready. Indicates if any events are available for
 * readout. Options are:\n
 * 0 = no event is available for readout\n
 * 1 = at least one event is available for readout.\n
 * NOTE: the status of this bit must be considered when managing the
 * readout from the digitizer.
 * @param EasyAcquisitionStatus::eventFull
 * Event Full. Indicates if at least one channel has reached the
 * FULL condition. Options are:\n
 * 0 = no channel has reached the FULL condition\n
 * 1 = the maximum number of events to be read is reached.
 * @param EasyAcquisitionStatus::clockSource
 * Clock Source. Indicates the clock source status. Options are:\n
 * 0 = internal (PLL uses the internal 50 MHz oscillator as reference)\n
 * 1 = external (PLL uses the external clock on CLK-IN connector as
 * reference).
 * @param EasyAcquisitionStatus::pLLBypassMode
 * PLL Bypass Mode. This bit drives the front panel 'PLL BYPS' LED.\n
 * Options are:\n
 * 0 = PLL bypass mode is not ac ve ('PLL BYPS' is off)\n
 * 1 = PLL bypass mode is ac ve and the VCXO frequency directly
 * drives the clock distribution chain ('PLL BYPS' lits).\n
 * WARNING: before operating in PLL Bypass Mode, it is recommended
 * to contact CAEN for feasibility.
 * @param EasyAcquisitionStatus::pLLUnlockDetect
 * PLL Unlock Detect. This bit flags a PLL unlock condition. Options
 * are:\n
 * 0 = PLL has had an unlock condition since the last register read
 * access\n
 * 1 = PLL has not had any unlock condition since the last register
 * read access.\n
 * NOTE: flag can be restored to 1 via read access to register 0xEF04.
 * @param EasyAcquisitionStatus::boardReady
 * Board Ready. This flag indicates if the board is ready for
 * acquisition (PLL and ADCs are correctly synchronised). Options
 * are:\n
 * 0 = board is not ready to start the acquisition\n
 * 1 = board is ready to start the acquisition.\n
 * NOTE: this bit should be checked a er software reset to ensure
 * that the board will enter immediately in run mode a er the RUN
 * mode setting; otherwise, a latency between RUN mode setting and
 * Acquisition start might occur.
 * @param EasyAcquisitionStatus::s_IN
 * S-IN (VME boards) or GPI (DT/NIM boards) Status. Reads the
 * current logical level on S-IN (GPI) front panel connector.
 * @param EasyAcquisitionStatus::tRG_IN
 * TRG-IN Status. Reads the current logical level on TRG-IN front
 * panel connector.
 */
class EasyAcquisitionStatus : public EasyBase
{
protected:
    /* Shared base since one constructor cannot reuse the other */
    /*
     * EasyAcquisitionStatus fields:
     * acquisition status [2], event ready [3], event full in [4],
     * clock source in [5], PLL bypass mode in [6],
     * PLL unlock detect in [7], board ready in [8], S-In in [15],
     * TRG-IN in [16].
     */
    virtual void initLayout() override
    {
        layout = {
                {"acquisitionStatus", {(const uint8_t)1, (const uint8_t)2}},
                {"eventReady", {(const uint8_t)1, (const uint8_t)3}},
                {"eventFull", {(const uint8_t)1, (const uint8_t)4}},
                {"clockSource", {(const uint8_t)1, (const uint8_t)5}},
                {"pLLBypassMode", {(const uint8_t)1, (const uint8_t)6}},
                {"pLLUnlockDetect", {(const uint8_t)1, (const uint8_t)7}},
                {"boardReady", {(const uint8_t)1, (const uint8_t)8}},
                {"s_IN", {(const uint8_t)1, (const uint8_t)15}},
                {"tRG_IN", {(const uint8_t)1, (const uint8_t)16}}
        };
    }
    /* NOTE: use inherited generic constructFromMask(mask) */
    void construct(const uint8_t acquisitionStatus, const uint8_t eventReady, const uint8_t eventFull, const uint8_t clockSource, const uint8_t pLLBypassMode, const uint8_t pLLUnlockDetect, const uint8_t boardReady, const uint8_t s_IN, const uint8_t tRG_IN) {
        initLayout();
        variables = {
                {"acquisitionStatus", (const uint8_t)(acquisitionStatus & 0x1)},
                {"eventReady", (const uint8_t)(eventReady & 0x1)},
                {"eventFull", (const uint8_t)(eventFull & 0x1)},
                {"clockSource", (const uint8_t)(clockSource & 0x1)},
                {"pLLBypassMode", (const uint8_t)(pLLBypassMode & 0x1)},
                {"pLLUnlockDetect", (const uint8_t)(pLLUnlockDetect & 0x1)},
                {"boardReady", (const uint8_t)(boardReady & 0x1)},
                {"s_IN", (const uint8_t)(s_IN & 0x1)},
                {"tRG_IN", (const uint8_t)(tRG_IN & 0x1)}
        };
    };
public:
    virtual const std::string getClassName() const override { return "EasyAcquisitionStatus"; };
    /* Construct using default values from docs */
    EasyAcquisitionStatus(const uint8_t acquisitionStatus, const uint8_t eventReady, const uint8_t eventFull, const uint8_t clockSource, const uint8_t pLLBypassMode, const uint8_t pLLUnlockDetect, const uint8_t boardReady, const uint8_t s_IN, const uint8_t tRG_IN)
    {
        construct(acquisitionStatus, eventReady, eventFull, clockSource, pLLBypassMode, pLLUnlockDetect, boardReady, s_IN, tRG_IN);
    }
    /* Construct from low-level bit mask in line with docs */
    EasyAcquisitionStatus(const uint32_t mask)
    {
        constructFromMask(mask);
    }
}; // class EasyAcquisitionStatus


/**
 * @class EasyDPPAcquisitionStatus
 * @brief For user-friendly configuration of Acquisition Status mask.
 *
 * This register monitors a set of conditions related to the
 * acquisition status.
 *
 * @param EasyDPPAcquisitionStatus::acquisitionStatus
 * Acquisition Status. It reflects the status of the acquisition and
 * drivers the front panel ’RUN’ LED. Options are:\n
 * 0 = acquisition is stopped (’RUN’ is off)\n
 * 1 = acquisition is running (’RUN’ lits).
 * @param EasyDPPAcquisitionStatus::eventReady
 * Event Ready. Indicates if any events are available for
 * readout. Options are:\n
 * 0 = no event is available for readout\n
 * 1 = at least one event is available for readout.\n
 * NOTE: the status of this bit must be considered when managing the
 * readout from the digitizer.
 * @param EasyDPPAcquisitionStatus::eventFull
 * Event Full. Indicates if at least one channel has reached the
 * FULL condition. Options are:\n
 * 0 = no channel has reached the FULL condition\n
 * 1 = the maximum number of events to be read is reached.
 * @param EasyDPPAcquisitionStatus::clockSource
 * Clock Source. Indicates the clock source status. Options are:\n
 * 0 = internal (PLL uses the internal 50 MHz oscillator as reference)\n
 * 1 = external (PLL uses the external clock on CLK-IN connector as
 * reference).
 * @param EasyDPPAcquisitionStatus::pLLUnlockDetect
 * PLL Unlock Detect. This bit flags a PLL unlock condition. Options
 * are:\n
 * 0 = PLL has had an unlock condition since the last register read
 * access\n
 * 1 = PLL has not had any unlock condition since the last register
 * read access.\n
 * NOTE: flag can be restored to 1 via read access to register 0xEF04.
 * @param EasyDPPAcquisitionStatus::boardReady
 * Board Ready. This flag indicates if the board is ready for
 * acquisition (PLL and ADCs are correctly synchronised). Options
 * are:\n
 * 0 = board is not ready to start the acquisition\n
 * 1 = board is ready to start the acquisition.\n
 * NOTE: this bit should be checked a er software reset to ensure
 * that the board will enter immediately in run mode a er the RUN
 * mode setting; otherwise, a latency between RUN mode setting and
 * Acquisition start might occur.
 * @param EasyDPPAcquisitionStatus::s_IN
 * S-IN (VME boards) or GPI (DT/NIM boards) Status. Reads the
 * current logical level on S-IN (GPI) front panel connector.
 * @param EasyDPPAcquisitionStatus::tRG_IN
 * TRG-IN Status. Reads the current logical level on TRG-IN front
 * panel connector.
 */
class EasyDPPAcquisitionStatus : public EasyBase
{
protected:
    /* Shared base since one constructor cannot reuse the other */
    /*
     * EasyDPPAcquisitionStatus fields:
     * acquisition status [2], event ready [3], event full in [4],
     * clock source in [5], PLL unlock detect in [7], board ready in [8],
     * S-In in [15], TRG-IN in [16].
     */
    virtual void initLayout() override
    {
        layout = {
                {"acquisitionStatus", {(const uint8_t)1, (const uint8_t)2}},
                {"eventReady", {(const uint8_t)1, (const uint8_t)3}},
                {"eventFull", {(const uint8_t)1, (const uint8_t)4}},
                {"clockSource", {(const uint8_t)1, (const uint8_t)5}},
                {"pLLUnlockDetect", {(const uint8_t)1, (const uint8_t)7}},
                {"boardReady", {(const uint8_t)1, (const uint8_t)8}},
                {"s_IN", {(const uint8_t)1, (const uint8_t)15}},
                {"tRG_IN", {(const uint8_t)1, (const uint8_t)16}}
        };
    }
    /* NOTE: use inherited generic constructFromMask(mask) */
    void construct(const uint8_t acquisitionStatus, const uint8_t eventReady, const uint8_t eventFull, const uint8_t clockSource, const uint8_t pLLUnlockDetect, const uint8_t boardReady, const uint8_t s_IN, const uint8_t tRG_IN) {
        initLayout();
        variables = {
                {"acquisitionStatus", (const uint8_t)(acquisitionStatus & 0x1)},
                {"eventReady", (const uint8_t)(eventReady & 0x1)},
                {"eventFull", (const uint8_t)(eventFull & 0x1)},
                {"clockSource", (const uint8_t)(clockSource & 0x1)},
                {"pLLUnlockDetect", (const uint8_t)(pLLUnlockDetect & 0x1)},
                {"boardReady", (const uint8_t)(boardReady & 0x1)},
                {"s_IN", (const uint8_t)(s_IN & 0x1)},
                {"tRG_IN", (const uint8_t)(tRG_IN & 0x1)}
        };
    };
public:
    virtual const std::string getClassName() const override { return "EasyDPPAcquisitionStatus"; };
    /* Construct using default values from docs */
    EasyDPPAcquisitionStatus(const uint8_t acquisitionStatus, const uint8_t eventReady, const uint8_t eventFull, const uint8_t clockSource, const uint8_t pLLUnlockDetect, const uint8_t boardReady, const uint8_t s_IN, const uint8_t tRG_IN)
    {
        construct(acquisitionStatus, eventReady, eventFull, clockSource, pLLUnlockDetect, boardReady, s_IN, tRG_IN);
    }
    /* Construct from low-level bit mask in line with docs */
    EasyDPPAcquisitionStatus(const uint32_t mask)
    {
        constructFromMask(mask);
    }
}; // class EasyDPPAcquisitionStatus


/**
 * @class EasyGlobalTriggerMask
 * @brief For user-friendly configuration of Global Trigger Mask.
 *
 * This register sets which signal can contribute to the global
 * trigger generation.
 *
 * @param EasyGlobalTriggerMask::groupTriggerMask
 * Bit n corresponds to the trigger request from group n that
 * participates to the global trigger generation (n = 0,...,3 for DT
 * and NIM; n = 0,...,7 for VME boards). Options are:\n
 * 0 = trigger request is not sensed for global trigger generation\n
 * 1 = trigger request participates in the global trigger generation.\n
 * NOTE: in case of DT and NIMboards, only bits[3:0] are meaningful,
 * while bits[7:4] are reserved.
 * @param EasyGlobalTriggerMask::majorityCoincidenceWindow
 * Majority Coincidence Window. Sets the me window (in units of the
 * trigger clock) for the majority coincidence. Majority level must
 * be set different from 0 through bits[26:24].
 * @param EasyGlobalTriggerMask::majorityLevel
 * Majority Level. Sets the majority level for the global trigger
 * generation. For a level m, the trigger fires when at least m+1 of
 * the enabled trigger requests (bits[7:0] or [3:0]) are
 * over-threshold inside the majority coincidence window
 * (bits[23:20]).\n
 * NOTE: the majority level must be smaller than the number of
 * trigger requests enabled via bits[7:0] mask (or [3:0]).
 * @param EasyGlobalTriggerMask::lVDSTrigger
 * LVDS Trigger (VME boards only). When enabled, the trigger from
 * LVDS I/O participates to the global trigger generation (in logic
 * OR). Options are:\n
 * 0 = disabled\n
 * 1 = enabled.
 * @param EasyGlobalTriggerMask::externalTrigger
 * External Trigger (default value is 1). When enabled, the external
 * trigger on TRG-IN participates to the global trigger generation
 * in logic OR with the other enabled signals. Options are:\n
 * 0 = disabled\n
 * 1 = enabled.
 * @param EasyGlobalTriggerMask::softwareTrigger
 * Software Trigger (default value is 1). When enabled, the software
 * trigger participates to the global trigger signal generation in
 * logic OR with the other enabled signals. Options are:\n
 * 0 = disabled\n
 * 1 = enabled.
 */
class EasyGlobalTriggerMask : public EasyBase
{
protected:
    /* Shared base since one constructor cannot reuse the other */
    /*
     * EasyGlobalTriggerMask fields:
     * groupTriggerMask in [0:7], majorityCoincidenceWindow in [20:23],
     * majorityLevel in [24:26], LVDS trigger in [29],
     * external trigger in [30], software trigger in [31].
     */
    virtual void initLayout() override
    {
        layout = {
                {"groupTriggerMask", {(const uint8_t)8, (const uint8_t)0}},
                {"majorityCoincidenceWindow", {(const uint8_t)4, (const uint8_t)20}},
                {"majorityLevel", {(const uint8_t)3, (const uint8_t)24}},
                {"lVDSTrigger", {(const uint8_t)1, (const uint8_t)29}},
                {"externalTrigger", {(const uint8_t)1, (const uint8_t)30}},
                {"softwareTrigger", {(const uint8_t)1, (const uint8_t)31}}
        };
    }
    /* NOTE: use inherited generic constructFromMask(mask) */
    void construct(const uint8_t groupTriggerMask, const uint8_t majorityCoincidenceWindow, const uint8_t majorityLevel, const uint8_t lVDSTrigger, const uint8_t externalTrigger, const uint8_t softwareTrigger) {
        initLayout();
        variables = {
                {"groupTriggerMask", (const uint8_t)(groupTriggerMask & 0xFF)},
                {"majorityCoincidenceWindow", (const uint8_t)(majorityCoincidenceWindow & 0xF)},
                {"majorityLevel", (const uint8_t)(majorityLevel & 0x7)},
                {"lVDSTrigger", (const uint8_t)(lVDSTrigger & 0x1)},
                {"externalTrigger", (const uint8_t)(externalTrigger & 0x1)},
                {"softwareTrigger", (const uint8_t)(softwareTrigger & 0x1)}
        };
    };
public:
    virtual const std::string getClassName() const override { return "EasyGlobalTriggerMask"; };
    /* Construct using default values from docs */
    EasyGlobalTriggerMask(const uint8_t groupTriggerMask, const uint8_t majorityCoincidenceWindow, const uint8_t majorityLevel, const uint8_t lVDSTrigger, const uint8_t externalTrigger, const uint8_t softwareTrigger)
    {
        construct(groupTriggerMask, majorityCoincidenceWindow, majorityLevel, lVDSTrigger, externalTrigger, softwareTrigger);
    }
    /* Construct from low-level bit mask in line with docs */
    EasyGlobalTriggerMask(const uint32_t mask)
    {
        constructFromMask(mask);
    }
}; // class EasyGlobalTriggerMask


/**
 * @class EasyDPPGlobalTriggerMask
 * @brief For user-friendly configuration of Global Trigger Mask.
 *
 * This register sets which signal can contribute to the global
 * trigger generation.
 *
 * @param EasyDPPGlobalTriggerMask::lVDSTrigger
 * LVDS Trigger (VME boards only). When enabled, the trigger from
 * LVDS I/O participates to the global trigger generation (in logic
 * OR). Options are:\n
 * 0 = disabled\n
 * 1 = enabled.
 * @param EasyDPPGlobalTriggerMask::externalTrigger
 * External Trigger (default value is 1). When enabled, the external
 * trigger on TRG-IN participates to the global trigger generation
 * in logic OR with the other enabled signals. Options are:\n
 * 0 = disabled\n
 * 1 = enabled.
 * @param EasyDPPGlobalTriggerMask::softwareTrigger
 * Software Trigger (default value is 1). When enabled, the software
 * trigger participates to the global trigger signal generation in
 * logic OR with the other enabled signals. Options are:\n
 * 0 = disabled\n
 * 1 = enabled.
 */
class EasyDPPGlobalTriggerMask : public EasyBase
{
protected:
    /* Shared base since one constructor cannot reuse the other */
    /*
     * EasyDPPGlobalTriggerMask fields:
     * LVDS trigger in [29], external trigger in [30],
     * software trigger in [31].
     */
    virtual void initLayout() override
    {
        layout = {
                {"lVDSTrigger", {(const uint8_t)1, (const uint8_t)29}},
                {"externalTrigger", {(const uint8_t)1, (const uint8_t)30}},
                {"softwareTrigger", {(const uint8_t)1, (const uint8_t)31}}
        };
    }
    /* NOTE: use inherited generic constructFromMask(mask) */
    void construct(const uint8_t lVDSTrigger, const uint8_t externalTrigger, const uint8_t softwareTrigger) {
        initLayout();
        variables = {
                {"lVDSTrigger", (const uint8_t)(lVDSTrigger & 0x1)},
                {"externalTrigger", (const uint8_t)(externalTrigger & 0x1)},
                {"softwareTrigger", (const uint8_t)(softwareTrigger & 0x1)}
        };
    };
public:
    virtual const std::string getClassName() const override { return "EasyDPPGlobalTriggerMask"; };
    /* Construct using default values from docs */
    EasyDPPGlobalTriggerMask(const uint8_t lVDSTrigger, const uint8_t externalTrigger, const uint8_t softwareTrigger)
    {
        construct(lVDSTrigger, externalTrigger, softwareTrigger);
    }
    /* Construct from low-level bit mask in line with docs */
    EasyDPPGlobalTriggerMask(const uint32_t mask)
    {
        constructFromMask(mask);
    }
}; // class EasyDPPGlobalTriggerMask


/**
 * @class EasyFrontPanelTRGOUTEnableMask
 * @brief For user-friendly configuration of Front Panel TRG-OUT Enable Mask.
 *
 * This register sets which signal can contribute to
 * generate the signal on the front panel TRG-OUT LEMO connector
 * (GPO in case of DT and NIM boards).
 *
 * @param EasyFrontPanelTRGOUTEnableMask::groupTriggerMask
 * This mask sets the trigger requests participating in the TRG-OUT
 * (GPO). Bit n corresponds to the trigger request from group n.\n
 * Options are:\n
 * 0 = Trigger request does not participate to the TRG-OUT (GPO) signal\n
 * 1 = Trigger request participates to the TRG-OUT (GPO) signal.\n
 * NOTE: in case of DT and NIM boards, only bits[3:0] are meaningful
 * while bis[7:4] are reserved.
 * @param EasyFrontPanelTRGOUTEnableMask::tRGOUTGenerationLogic
 * TRG-OUT (GPO) Generation Logic. The enabled trigger requests
 * (bits [7:0] or [3:0]) can be combined to generate the TRG-OUT
 * (GPO) signal. Options are:\n
 * 00 = OR\n
 * 01 = AND\n
 * 10 = Majority\n
 * 11 = reserved.
 * @param EasyFrontPanelTRGOUTEnableMask::majorityLevel
 * Majority Level. Sets the majority level for the TRG-OUT (GPO)
 * signal generation. Allowed level values are between 0 and 7 for
 * VME boards, and between 0 and 3 for DT and NIM boards. For a
 * level m, the trigger fires when at least m+1 of the trigger
 * requests are generated by the enabled channels (bits [7:0] or [3:0]).
 * @param EasyFrontPanelTRGOUTEnableMask::lVDSTriggerEnable
 * LVDS Trigger Enable (VME boards only). If the LVDS I/Os are
 * programmed as outputs, they can participate in the TRG-OUT (GPO)
 * signal generation. They are in logic OR with the other enabled
 * signals. Options are:\n
 * 0 = disabled\n
 * 1 = enabled.
 * @param EasyFrontPanelTRGOUTEnableMask::externalTrigger
 * External Trigger. When enabled, the external trigger on TRG-IN
 * can participate in the TRG-OUT (GPO) signal generation in logic
 * OR with the other enabled signals. Options are:\n
 * 0 = disabled\n
 * 1 = enabled.
 * @param EasyFrontPanelTRGOUTEnableMask::softwareTrigger
 * Software Trigger. When enabled, the software trigger can
 * participate in the TRG-OUT (GPO) signal generation in logic OR
 * with the other enabled signals. Options are:\n
 * 0 = disabled\n
 * 1 = enabled.
 */
class EasyFrontPanelTRGOUTEnableMask : public EasyBase
{
protected:
    /* Shared base since one constructor cannot reuse the other */
    /*
     * EasyFrontPanelTRGOUTEnableMask fields:
     * group Trigger mask in [0:7], TRG-OUT generation logic in [8:9],
     * majority level in [10:12], LVDS trigger enable in [29],
     * external trigger in [30], software trigger in [31].
     */
    virtual void initLayout() override
    {
        layout = {
                {"groupTriggerMask", {(const uint8_t)8, (const uint8_t)0}},
                {"tRGOUTGenerationLogic", {(const uint8_t)2, (const uint8_t)8}},
                {"majorityLevel", {(const uint8_t)3, (const uint8_t)10}},
                {"lVDSTriggerEnable", {(const uint8_t)1, (const uint8_t)29}},
                {"externalTrigger", {(const uint8_t)1, (const uint8_t)30}},
                {"softwareTrigger", {(const uint8_t)1, (const uint8_t)31}}
        };
    }
    /* NOTE: use inherited generic constructFromMask(mask) */
    void construct(const uint8_t groupTriggerMask, const uint8_t tRGOUTGenerationLogic, const uint8_t majorityLevel, const uint8_t lVDSTriggerEnable, const uint8_t externalTrigger, const uint8_t softwareTrigger) {
        initLayout();
        variables = {
                {"groupTriggerMask", (const uint8_t)(groupTriggerMask & 0xFF)},
                {"tRGOUTGenerationLogic", (const uint8_t)(tRGOUTGenerationLogic & 0x3)},
                {"majorityLevel", (const uint8_t)(majorityLevel & 0x7)},
                {"lVDSTriggerEnable", (const uint8_t)(lVDSTriggerEnable & 0x1)},
                {"externalTrigger", (const uint8_t)(externalTrigger & 0x1)},
                {"softwareTrigger", (const uint8_t)(softwareTrigger & 0x1)}
        };
    };
public:
    virtual const std::string getClassName() const override { return "EasyFrontPanelTRGOUTEnableMask"; };
    /* Construct using default values from docs */
    EasyFrontPanelTRGOUTEnableMask(const uint8_t groupTriggerMask, const uint8_t tRGOUTGenerationLogic, const uint8_t majorityLevel, const uint8_t lVDSTriggerEnable, const uint8_t externalTrigger, const uint8_t softwareTrigger)
    {
        construct(groupTriggerMask, tRGOUTGenerationLogic, majorityLevel, lVDSTriggerEnable, externalTrigger, softwareTrigger);
    }
    /* Construct from low-level bit mask in line with docs */
    EasyFrontPanelTRGOUTEnableMask(const uint32_t mask)
    {
        constructFromMask(mask);
    }
}; // class EasyFrontPanelTRGOUTEnableMask


/**
 * @class EasyDPPFrontPanelTRGOUTEnableMask
 * @brief For user-friendly configuration of Front Panel TRG-OUT Enable Mask.
 *
 * This register sets which signal can contribute to
 * generate the signal on the front panel TRG-OUT LEMO connector
 * (GPO in case of DT and NIM boards).
 *
 * @param EasyDPPFrontPanelTRGOUTEnableMask::lVDSTriggerEnable
 * LVDS Trigger Enable (VME boards only). If the LVDS I/Os are
 * programmed as outputs, they can participate in the TRG-OUT (GPO)
 * signal generation. They are in logic OR with the other enabled
 * signals. Options are:\n
 * 0 = disabled\n
 * 1 = enabled.
 * @param EasyDPPFrontPanelTRGOUTEnableMask::externalTrigger
 * External Trigger. When enabled, the external trigger on TRG-IN
 * can participate in the TRG-OUT (GPO) signal generation in logic
 * OR with the other enabled signals. Options are:\n
 * 0 = disabled\n
 * 1 = enabled.
 * @param EasyDPPFrontPanelTRGOUTEnableMask::softwareTrigger
 * Software Trigger. When enabled, the software trigger can
 * participate in the TRG-OUT (GPO) signal generation in logic OR
 * with the other enabled signals. Options are:\n
 * 0 = disabled\n
 * 1 = enabled.
 */
class EasyDPPFrontPanelTRGOUTEnableMask : public EasyBase
{
protected:
    /* Shared base since one constructor cannot reuse the other */
    /*
     * EasyDPPFrontPanelTRGOUTEnableMask fields:
     * LVDS trigger enable in [29], external trigger in [30],
     * software trigger in [31].
     */
    virtual void initLayout() override
    {
        layout = {
                {"lVDSTriggerEnable", {(const uint8_t)1, (const uint8_t)29}},
                {"externalTrigger", {(const uint8_t)1, (const uint8_t)30}},
                {"softwareTrigger", {(const uint8_t)1, (const uint8_t)31}}
        };
    }
    /* NOTE: use inherited generic constructFromMask(mask) */
    void construct(const uint8_t lVDSTriggerEnable, const uint8_t externalTrigger, const uint8_t softwareTrigger) {
        initLayout();
        variables = {
                {"lVDSTriggerEnable", (const uint8_t)(lVDSTriggerEnable & 0x1)},
                {"externalTrigger", (const uint8_t)(externalTrigger & 0x1)},
                {"softwareTrigger", (const uint8_t)(softwareTrigger & 0x1)}
        };
    };
public:
    virtual const std::string getClassName() const override { return "EasyDPPFrontPanelTRGOUTEnableMask"; };
    /* Construct using default values from docs */
    EasyDPPFrontPanelTRGOUTEnableMask(const uint8_t lVDSTriggerEnable, const uint8_t externalTrigger, const uint8_t softwareTrigger)
    {
        construct(lVDSTriggerEnable, externalTrigger, softwareTrigger);
    }
    /* Construct from low-level bit mask in line with docs */
    EasyDPPFrontPanelTRGOUTEnableMask(const uint32_t mask)
    {
        constructFromMask(mask);
    }
}; // class EasyDPPFrontPanelTRGOUTEnableMask


/**
 * @class EasyFrontPanelIOControl
 * @brief For user-friendly configuration of Acquisition Control mask.
 *
 * This register manages the front panel I/O connectors. Default
 * value is 0x000000.
 *
 * @param EasyFrontPanelIOControl::lEMOIOElectricalLevel
 * LEMO I/Os Electrical Level. This bit sets the electrical level of
 * the front panel LEMO connectors: TRG-IN, TRG-OUT (GPO in case of
 * DT and NIM boards), S-IN (GPI in case of DT and NIM
 * boards). Options are:\n
 * 0 = NIM I/O levels\n
 * 1 = TTL I/O levels.
 * @param EasyFrontPanelIOControl::tRGOUTEnable
 * TRG-OUT Enable (VME boards only). Enables the TRG-OUT LEMO front
 * panel connector. Options are:\n
 * 0 = enabled (default)\n
 * 1 = high impedance.\n
 * NOTE: this bit is reserved in case of DT and NIM boards.
 * @param EasyFrontPanelIOControl::lVDSIODirectionFirst
 * LVDS I/O [3:0] Direction (VME boards only). Sets the direction of
 * the signals on the first 4-pin group of the LVDS I/O
 * connector. Options are:\n
 * 0 = input\n
 * 1 = output.\n
 * NOTE: this bit is reserved in case of DT and NIM boards.
 * @param EasyFrontPanelIOControl::lVDSIODirectionSecond
 * LVDS I/O [7:4] Direction (VME boards only). Sets the direction of
 * the second 4-pin group of the LVDS I/O connector. Options are:\n
 * 0 = input\n
 * 1 = output.\n
 * NOTE: this bit is reserved in case of DT and NIM boards.
 * @param EasyFrontPanelIOControl::lVDSIODirectionThird
 * LVDS I/O [11:8] Direction (VME boards only). Sets the direction of
 * the third 4-pin group of the LVDS I/O connector. Options are:\n
 * 0 = input\n
 * 1 = output.\n
 * NOTE: this bit is reserved in case of DT and NIM boards.
 * @param EasyFrontPanelIOControl::lVDSIODirectionFourth
 * LVDS I/O [15:12] Direction (VME boards only). Sets the direction of
 * the fourth 4-pin group of the LVDS I/O connector. Options are:\n
 * 0 = input\n
 * 1 = output.\n
 * NOTE: this bit is reserved in case of DT and NIM boards.
 * @param EasyFrontPanelIOControl::lVDSIOSignalConfiguration
 * LVDS I/O Signal Configuration (VME boards and LVDS I/O old
 * features only). This configuration must be enabled through bit[8]
 * set to 0. Options are:\n
 * 00 = general purpose I/O\n
 * 01 = programmed I/O\n
 * 10 = pattern mode: LVDS signals are input and their value is
 * written into the header PATTERN field\n
 * 11 = reserved.\n
 * NOTE: these bits are reserved in case of DT and NIM boards.
 * @param EasyFrontPanelIOControl::lVDSIONewFeaturesSelection
 * LVDS I/O New Features Selection (VME boards only). Options are:\n
 * 0 = LVDS old features\n
 * 1 = LVDS new features.\n
 * The new features options can be configured through register
 * 0x81A0. Please, refer to the User Manual for all details.\n
 * NOTE: LVDS I/O New Features option is valid from motherboard
 * firmware revision 3.8 on.\n
 * NOTE: this bit is reserved in case of DT and NIM boards.
 * @param EasyFrontPanelIOControl::lVDSIOPatternLatchMode
 * LVDS I/Os Pattern Latch Mode (VME boards only). Options are:\n
 * 0 = Pattern (i.e. 16-pin LVDS status) is latched when the
 * (internal) global trigger is sent to channels, in consequence of
 * an external trigger. It accounts for post-trigger settings and
 * input latching delays\n
 * 1 = Pattern (i.e. 16-pin LVDS status) is latched when an external
 * trigger arrives.\n
 * NOTE: this bit is reserved in case of DT and NIM boards.
 * @param EasyFrontPanelIOControl::tRGINControl
 * TRG-IN control. The board trigger logic can be synchronized
 * either with the edge of the TRG-IN signal, or with its whole
 * duration.\n
 * Note: this bit must be used in conjunction with bit[11] =
 * 0. Options are:\n
 * 0 = trigger is synchronized with the edge of the TRG-IN signal\n
 * 1 = trigger is synchronized with the whole duration of the TRG-IN
 * signal.
 * @param EasyFrontPanelIOControl::tRGINMezzanines
 * TRG-IN to Mezzanines (channels). Options are:\n
 * 0 = TRG-IN signal is processed by the motherboard and sent to
 * mezzanine (default). The trigger logic is then synchronized with
 * TRG-IN\n
 * 1 = TRG-IN is directly sent to the mezzanines with no mother
 * board processing nor delay. This option can be useful when TRG-IN
 * is used to veto the acquisition.\n
 * NOTE: if this bit is set to 1, then bit[10] is ignored.
 * @param EasyFrontPanelIOControl::forceTRGOUT
 * Force TRG-OUT (GPO). This bit can force TRG-OUT (GPO in case of
 * DT and NIM boards) test logical level if bit[15] = 1. Options
 * are:\n
 * 0 = Force TRG-OUT (GPO) to 0\n
 * 1 = Force TRG-OUT (GPO) to 1.
 * @param EasyFrontPanelIOControl::tRGOUTMode
 * TRG-OUT (GPO) Mode. Options are:\n
 * 0 = TRG-OUT (GPO) is an internal signal (according to bits[17:16])\n
 * 1= TRG-OUT (GPO) is a test logic level set via bit[14].
 * @param EasyFrontPanelIOControl::tRGOUTModeSelection
 * TRG-OUT (GPO) Mode Selection. Options are:\n
 * 00 = Trigger: TRG-OUT/GPO propagates the internal trigger sources
 * according to register 0x8110\n
 * 01 = Motherboard Probes: TRG-OUT/GPO is used to propagate signals
 * of the motherboards according to bits[19:18]\n
 * 10 = Channel Probes: TRG-OUT/GPO is used to propagate signals of
 * the mezzanines (Channel Signal Virtual Probe)\n
 * 11 = S-IN (GPI) propagation.
 * @param EasyFrontPanelIOControl::motherboardVirtualProbeSelection
 * Motherboard Virtual Probe Selection (to be propagated on TRG-
 * OUT/GPO). Options are:\n
 * 00 = RUN/delayedRUN: this is the RUN in case of ROC FPGA firmware
 * rel. less than 4.12. This probe can be selected according to
 * bit[20].\n
 * 01 = CLKOUT: this clock is synchronous with the sampling clock of
 * the ADC and this option can be used to align the phase of the
 * clocks in different boards\n
 * 10 = CLK Phase\n
 * 11 = BUSY/UNLOCK: this is the board BUSY in case of ROC FPGA
 * firmware rel. 4.5 or lower. This probe can be selected according
 * to bit[20].
 * @param EasyFrontPanelIOControl::motherboardVirtualProbePropagation
 * According to bits[19:18], this bit selects the probe to be
 * propagated on TRG-OUT . If bits[19:18] = 00, then bit[20] options
 * are:\n
 * 0 = RUN, the signal is active when the acquisition is running and
 * it is synchonized with the start run. This option must be used to
 * synchronize the start/stop of the acquisition through the
 * TRG-OUT->TR-IN or TRG-OUT->S-IN (GPI) daisy chain.\n
 * 1 = delayedRUN. This option can be used to debug the
 * synchronization when the start/stop is propagated through the
 * LVDS I/O (VME boards). If bits[19:18] = 11, then bit[20] options
 * are:\n
 * 0 = Board BUSY\n
 * 1 = PLL Lock Loss.\n
 * NOTE: this bit is reserved in case of ROC FPGA firmware rel. 4.5
 * or lower.\n
 * NOTE: this bit corresponds to BUSY/UNLOCK for ROC FPGA firmware
 * rel. less than 4.12.
 * @param EasyFrontPanelIOControl::patternConfiguration
 * Pattern Configuration. Configures the information given by the
 * 16-bit PATTERN field in the header of the event format (VME
 * only). Option are:\n
 * 00 = PATTERN: 16-bit pattern latched on the 16 LVDS signals as
 * one trigger arrives (default)\n
 * Other options are reserved.
 */
class EasyFrontPanelIOControl : public EasyBase
{
protected:
    /* Shared base since one constructor cannot reuse the other */
    /*
     * EasyFrontPanelIOControl fields:
     * LEMO I/O electrical level [0], TRG-OUT enable [1],
     * LVDS I/O 1st Direction in [2], LVDS I/O 2nd Direction in [3],
     * LVDS I/O 3rd Direction in [4], LVDS I/O 4th Direction in [5],
     * LVDS I/O signal configuration [6:7],
     * LVDS I/O new features selection in [8],
     * LVDS I/Os pattern latch mode in [9],
     * TRG-IN control in [10], TRG-IN to mezzanines in [11],
     * force TRG-OUT in [14], TRG-OUT mode in [15],
     * TRG-OUT mode selection in [16:17],
     * motherboard virtual probe selection in [18:19],
     * motherboard virtual probe propagation in [20],
     * pattern configuration in [21:22]
     */
    virtual void initLayout() override
    {
        layout = {{"lEMOIOElectricalLevel", {(const uint8_t)1, (const uint8_t)0}},
                  {"tRGOUTEnable", {(const uint8_t)1, (const uint8_t)1}},
                  {"lVDSIODirectionFirst", {(const uint8_t)1, (const uint8_t)2}},
                  {"lVDSIODirectionSecond", {(const uint8_t)1, (const uint8_t)3}},
                  {"lVDSIODirectionThird", {(const uint8_t)1, (const uint8_t)4}},
                  {"lVDSIODirectionFourth", {(const uint8_t)1, (const uint8_t)5}},
                  {"lVDSIOSignalConfiguration", {(const uint8_t)2, (const uint8_t)6}},
                  {"lVDSIONewFeaturesSelection", {(const uint8_t)1, (const uint8_t)8}},
                  {"lVDSIOPatternLatchMode", {(const uint8_t)1, (const uint8_t)9}},
                  {"tRGINControl", {(const uint8_t)1, (const uint8_t)10}},
                  {"tRGINMezzanines", {(const uint8_t)1, (const uint8_t)11}},
                  {"forceTRGOUT", {(const uint8_t)1, (const uint8_t)14}},
                  {"tRGOUTMode", {(const uint8_t)1, (const uint8_t)15}},
                  {"tRGOUTModeSelection", {(const uint8_t)2, (const uint8_t)16}},
                  {"motherboardVirtualProbeSelection", {(const uint8_t)2, (const uint8_t)18}},
                  {"motherboardVirtualProbePropagation", {(const uint8_t)1, (const uint8_t)20}},
                  {"patternConfiguration", {(const uint8_t)2, (const uint8_t)21}}
        };
    }
    /* NOTE: use inherited generic constructFromMask(mask) */
    void construct(const uint8_t lEMOIOElectricalLevel, const uint8_t tRGOUTEnable, const uint8_t lVDSIODirectionFirst, const uint8_t lVDSIODirectionSecond, const uint8_t lVDSIODirectionThird, const uint8_t lVDSIODirectionFourth, const uint8_t lVDSIOSignalConfiguration, const uint8_t lVDSIONewFeaturesSelection, const uint8_t lVDSIOPatternLatchMode, const uint8_t tRGINControl, const uint8_t tRGINMezzanines, const uint8_t forceTRGOUT, const uint8_t tRGOUTMode, const uint8_t tRGOUTModeSelection, const uint8_t motherboardVirtualProbeSelection, const uint8_t motherboardVirtualProbePropagation, const uint8_t patternConfiguration) {
        initLayout();
        variables = {
                {"lEMOIOElectricalLevel", (const uint8_t)(lEMOIOElectricalLevel & 0x1)},
                {"tRGOUTEnable", (const uint8_t)(tRGOUTEnable & 0x1)},
                {"lVDSIODirectionFirst", (const uint8_t)(lVDSIODirectionFirst & 0x1)},
                {"lVDSIODirectionSecond", (const uint8_t)(lVDSIODirectionSecond & 0x1)},
                {"lVDSIODirectionThird", (const uint8_t)(lVDSIODirectionThird & 0x1)},
                {"lVDSIODirectionFourth", (const uint8_t)(lVDSIODirectionFourth & 0x1)},
                {"lVDSIOSignalConfiguration", (const uint8_t)(lVDSIOSignalConfiguration & 0x3)},
                {"lVDSIONewFeaturesSelection", (const uint8_t)(lVDSIONewFeaturesSelection & 0x1)},
                {"lVDSIOPatternLatchMode", (const uint8_t)(lVDSIOPatternLatchMode & 0x1)},
                {"tRGINControl", (const uint8_t)(tRGINControl & 0x1)},
                {"tRGINMezzanines", (const uint8_t)(tRGINMezzanines & 0x1)},
                {"forceTRGOUT", (const uint8_t)(forceTRGOUT & 0x1)},
                {"tRGOUTMode", (const uint8_t)(tRGOUTMode & 0x1)},
                {"tRGOUTModeSelection", (const uint8_t)(tRGOUTModeSelection & 0x3)},
                {"motherboardVirtualProbeSelection", (const uint8_t)(motherboardVirtualProbeSelection & 0x3)},
                {"motherboardVirtualProbePropagation", (const uint8_t)(motherboardVirtualProbePropagation & 0x1)},
                {"patternConfiguration", (const uint8_t)(patternConfiguration & 0x3)}
        };
    };
public:
    virtual const std::string getClassName() const override { return "EasyFrontPanelIOControl"; };
    /* Construct using default values from docs */
    EasyFrontPanelIOControl(const uint8_t lEMOIOElectricalLevel, const uint8_t tRGOUTEnable, const uint8_t lVDSIODirectionFirst, const uint8_t lVDSIODirectionSecond, const uint8_t lVDSIODirectionThird, const uint8_t lVDSIODirectionFourth, const uint8_t lVDSIOSignalConfiguration, const uint8_t lVDSIONewFeaturesSelection, const uint8_t lVDSIOPatternLatchMode, const uint8_t tRGINControl, const uint8_t tRGINMezzanines, const uint8_t forceTRGOUT, const uint8_t tRGOUTMode, const uint8_t tRGOUTModeSelection, const uint8_t motherboardVirtualProbeSelection, const uint8_t motherboardVirtualProbePropagation, const uint8_t patternConfiguration)
    {
        construct(lEMOIOElectricalLevel, tRGOUTEnable, lVDSIODirectionFirst, lVDSIODirectionSecond, lVDSIODirectionThird, lVDSIODirectionFourth, lVDSIOSignalConfiguration, lVDSIONewFeaturesSelection, lVDSIOPatternLatchMode, tRGINControl, tRGINMezzanines, forceTRGOUT, tRGOUTMode, tRGOUTModeSelection, motherboardVirtualProbeSelection, motherboardVirtualProbePropagation, patternConfiguration);
    }
    /* Construct from low-level bit mask in line with docs */
    EasyFrontPanelIOControl(const uint32_t mask)
    {
        constructFromMask(mask);
    }
}; // class EasyFrontPanelIOControl


/**
 * @class EasyDPPFrontPanelIOControl
 * @brief For user-friendly configuration of Acquisition Control mask.
 *
 * NOTE: Identical to EasyFrontPanelIOControl
 */
class EasyDPPFrontPanelIOControl : public EasyFrontPanelIOControl
{
    /* Just inherit everything else from parent class */
public:
    virtual const std::string getClassName() const override { return "EasyDPPFrontPanelIOControl"; };
    EasyDPPFrontPanelIOControl(const uint8_t lEMOIOElectricalLevel, const uint8_t tRGOUTEnable, const uint8_t lVDSIODirectionFirst, const uint8_t lVDSIODirectionSecond, const uint8_t lVDSIODirectionThird, const uint8_t lVDSIODirectionFourth, const uint8_t lVDSIOSignalConfiguration, const uint8_t lVDSIONewFeaturesSelection, const uint8_t lVDSIOPatternLatchMode, const uint8_t tRGINControl, const uint8_t tRGINMezzanines, const uint8_t forceTRGOUT, const uint8_t tRGOUTMode, const uint8_t tRGOUTModeSelection, const uint8_t motherboardVirtualProbeSelection, const uint8_t motherboardVirtualProbePropagation, const uint8_t patternConfiguration) : EasyFrontPanelIOControl(lEMOIOElectricalLevel, tRGOUTEnable, lVDSIODirectionFirst, lVDSIODirectionSecond, lVDSIODirectionThird, lVDSIODirectionFourth, lVDSIOSignalConfiguration, lVDSIONewFeaturesSelection, lVDSIOPatternLatchMode, tRGINControl, tRGINMezzanines, forceTRGOUT, tRGOUTMode, tRGOUTModeSelection, motherboardVirtualProbeSelection, motherboardVirtualProbePropagation, patternConfiguration) {};
    EasyDPPFrontPanelIOControl(uint32_t mask) : EasyFrontPanelIOControl(mask) {};
}; // class EasyDPPFrontPanelIOControl


/**
 * @class EasyROCFPGAFirmwareRevision
 * @brief For user-friendly configuration of ROC FPGA Firmware Revision.
 *
 * This register contains the motherboard FPGA (ROC) firmware
 * revision information.\n
 * The complete format is:\n
 * Firmware Revision = X.Y (16 lower bits)\n
 * Firmware Revision Date = Y/M/DD (16 higher bits)\n
 * EXAMPLE 1: revision 3.08, November 12th, 2007 is 0x7B120308.\n
 * EXAMPLE 2: revision 4.09, March 7th, 2016 is 0x03070409.\n
 * NOTE: the nibble code for the year makes this information to roll
 * over each 16 years.
 *
 * @param EasyROCFPGAFirmwareRevision::minorRevisionNumber
 * ROC Firmware Minor Revision Number (Y).
 * @param EasyROCFPGAFirmwareRevision::majorRevisionNumber
 * ROC Firmware Major Revision Number (X).
 * @param EasyROCFPGAFirmwareRevision::revisionDate
 * ROC Firmware Revision Date (Y/M/DD).
 */
class EasyROCFPGAFirmwareRevision : public EasyBase
{
protected:
    /* Shared base since one constructor cannot reuse the other */
    /*
     * EasyROCFPGAFirmwareRevision fields:
     * minor revision number in [0:7], major revision number in [8:15],
     * revision date in [16:31].
     */
    /* NOTE: we split revision date into the four digits internally
     since it is just four 4-bit values clamped into 16-bit anyway.
     This makes the generic and DPP version much more similar, too. */
    void initLayout()
    {
        layout = {
                {"minorRevisionNumber", {(const uint8_t)8, (const uint8_t)0}},
                {"majorRevisionNumber", {(const uint8_t)8, (const uint8_t)8}},
                {"revisionDayLower", {(const uint8_t)4, (const uint8_t)16}},
                {"revisionDayUpper", {(const uint8_t)4, (const uint8_t)20}},
                {"revisionMonth", {(const uint8_t)4, (const uint8_t)24}},
                {"revisionYear", {(const uint8_t)4, (const uint8_t)28}}
        };
    }
    void construct(const uint8_t minorRevisionNumber, const uint8_t majorRevisionNumber, const uint8_t revisionDayLower, const uint8_t revisionDayUpper, const uint8_t revisionMonth, const uint8_t revisionYear)
    {
        initLayout();
        variables = {
                {"minorRevisionNumber", (const uint8_t)minorRevisionNumber},
                {"majorRevisionNumber", (const uint8_t)majorRevisionNumber},
                {"revisionDayLower", (const uint8_t)(revisionDayLower & 0x7)},
                {"revisionDayUpper", (const uint8_t)(revisionDayUpper & 0x7)},
                {"revisionMonth", (const uint8_t)(revisionMonth & 0x7)},
                {"revisionYear", (const uint8_t)(revisionYear & 0x7)}
        };
    }
public:
    virtual const std::string getClassName() const override { return "EasyROCFPGAFirmwareRevision"; };
    /* Construct using default values from docs */
    /* We allow both clamped and individual revision date format here */
    EasyROCFPGAFirmwareRevision(const uint8_t minorRevisionNumber, const uint8_t majorRevisionNumber, const uint16_t revisionDate)
    {
        const uint8_t revisionDayLower = (const uint8_t)(revisionDate & 0x7);
        const uint8_t revisionDayUpper = (const uint8_t)((revisionDate >> 4) & 0x7);
        const uint8_t revisionMonth = (const uint8_t)((revisionDate >> 8) & 0x7);
        const uint8_t revisionYear = (const uint8_t)((revisionDate >> 12) & 0x7);
        construct(minorRevisionNumber, majorRevisionNumber, revisionDayLower, revisionDayUpper, revisionMonth, revisionYear);
    }
    EasyROCFPGAFirmwareRevision(const uint8_t minorRevisionNumber, const uint8_t majorRevisionNumber, const uint8_t revisionDayLower, const uint8_t revisionDayUpper, const uint8_t revisionMonth, const uint8_t revisionYear)
    {
        construct(minorRevisionNumber, majorRevisionNumber, revisionDayLower, revisionDayUpper, revisionMonth, revisionYear);
    }
    /* Construct from low-level bit mask in line with docs */
    EasyROCFPGAFirmwareRevision(const uint32_t mask)
    {
        constructFromMask(mask);
    }
}; // class EasyROCFPGAFirmwareRevision


/**
 * @class EasyDPPROCFPGAFirmwareRevision
 * @brief For user-friendly configuration of ROC FPGA Firmware Revision.
 *
 * NOTE: identical to EasyROCFPGAFirmwareRevision
 */
class EasyDPPROCFPGAFirmwareRevision : public EasyBase
{
protected:
    /* Shared base since one constructor cannot reuse the other */
    /*
     * EasyDPPROCFPGAFirmwareRevision fields:
     * firmware revision number in [0:7], firmware DPP code in [8:15],
     * build day lower in [16:19], build day upper in [20:23],
     * build month in [24:27], build year in [28:31]
     */
    void initLayout()
    {
        layout = {
                {"firmwareRevisionNumber", {(const uint8_t)8, (const uint8_t)0}},
                {"firmwareDPPCode", {(const uint8_t)8, (const uint8_t)8}},
                {"buildDayLower", {(const uint8_t)4, (const uint8_t)16}},
                {"buildDayUpper", {(const uint8_t)4, (const uint8_t)20}},
                {"buildMonth", {(const uint8_t)4, (const uint8_t)24}},
                {"buildYear", {(const uint8_t)4, (const uint8_t)28}}
        };
    }
    /* NOTE: use inherited generic constructFromMask(mask) */
    void construct(const uint8_t firmwareRevisionNumber, const uint8_t firmwareDPPCode, const uint8_t buildDayLower, const uint8_t buildDayUpper, const uint8_t buildMonth, const uint8_t buildYear)
    {
        initLayout();
        variables = {
                {"firmwareRevisionNumber", (const uint8_t)firmwareRevisionNumber},
                {"firmwareDPPCode", (const uint8_t)firmwareDPPCode},
                {"buildDayLower", (const uint8_t)(buildDayLower & 0xF)},
                {"buildDayUpper", (const uint8_t)(buildDayUpper & 0xF)},
                {"buildMonth", (const uint8_t)(buildMonth & 0xF)},
                {"buildYear", (const uint8_t)(buildYear & 0xF)}
        };
    }
public:
    virtual const std::string getClassName() const override { return "EasyDPPROCFPGAFirmwareRevision"; };
    /* Construct using default values from docs */
    /* We allow both clamped and individual revision date format here */
    EasyDPPROCFPGAFirmwareRevision(const uint8_t minorRevisionNumber, const uint8_t majorRevisionNumber, const uint16_t revisionDate)
    {
        const uint8_t revisionDayLower = (const uint8_t)(revisionDate & 0x7);
        const uint8_t revisionDayUpper = (const uint8_t)((revisionDate >> 4) & 0x7);
        const uint8_t revisionMonth = (const uint8_t)((revisionDate >> 8) & 0x7);
        const uint8_t revisionYear = (const uint8_t)((revisionDate >> 12) & 0x7);
        construct(minorRevisionNumber, majorRevisionNumber, revisionDayLower, revisionDayUpper, revisionMonth, revisionYear);
    }
    EasyDPPROCFPGAFirmwareRevision(const uint8_t firmwareRevisionNumber, const uint8_t firmwareDPPCode, const uint8_t buildDayLower, const uint8_t buildDayUpper, const uint8_t buildMonth, const uint8_t buildYear)
    {
        construct(firmwareRevisionNumber, firmwareDPPCode, buildDayLower, buildDayUpper, buildMonth, buildYear);
    }
    /* Construct from low-level bit mask in line with docs */
    EasyDPPROCFPGAFirmwareRevision(const uint32_t mask)
    {
        constructFromMask(mask);
    }
}; // class EasyDPPROCFPGAFirmwareRevision


/**
 * @class EasyFanSpeedControl
 * @brief For user-friendly configuration of Fan Speed Control mask.
 *
 * This register manages the on-board fan speed in order to
 * guarantee an appropriate cooling according to the internal
 * temperature variations.\n
 * NOTE: from revision 4 of the motherboard PCB (see register 0xF04C
 * of the Configuration ROM), the automatic fan speed control has
 * been implemented, and it is supported by ROC FPGA firmware
 * revision greater than 4.4 (see register 0x8124).\n
 * Independently of the revision, the user can set the fan speed
 * high by setting bit[3] = 1. Setting bit[3] = 0 will restore the
 * automatic control for revision 4 or higher, or the low fan speed
 * in case of revisions lower than 4.\n
 * NOTE: this register is supported by Desktop (DT) boards only.
 *
 * @param EasyFanSpeedControl::fanSpeedMode
 * Fan Speed Mode. Options are:\n
 * 0 = slow speed or automatic speed tuning\n
 * 1 = high speed.
 */
class EasyFanSpeedControl : public EasyBase
{
protected:
    /* Shared helpers since one constructor cannot reuse the other */
    /*
     * EasyFanSpeedControl fields:
     * fan Speed mode in [3], (reserved forced 1) in [4:5].
     */
    void initLayout() override
    {
        layout = {
                {"fanSpeedMode", {(const uint8_t)1, (const uint8_t)3}},
                {"__reserved__0_", {(const uint8_t)2, (const uint8_t)4}}
        };
    }
    /* NOTE: use inherited generic constructFromMask(mask) */
    void construct(const uint8_t fanSpeedMode) {
        initLayout();
        variables = {
                {"fanSpeedMode", (const uint8_t)(fanSpeedMode & 0x1)},
                {"__reserved__0_", (const uint8_t)(0x1)}
        };
    };
public:
    virtual const std::string getClassName() const override { return "EasyFanSpeedControl"; };
    /* Construct using default values from docs */
    EasyFanSpeedControl(const uint8_t fanSpeedMode=0)
    {
        construct(fanSpeedMode);
    }
    /* Construct from low-level bit mask in line with docs */
    EasyFanSpeedControl(const uint32_t mask)
    {
        constructFromMask(mask);
    }
}; // class EasyFanSpeedControl


/**
 * @class EasyDPPFanSpeedControl
 * @brief For user-friendly configuration of Fan Speed Control mask.
 *
 * NOTE: identical to EasyFanSpeedControl
 */
class EasyDPPFanSpeedControl : public EasyFanSpeedControl
{
    /* Just inherit everything else from parent class */
public:
    virtual const std::string getClassName() const override { return "EasyDPPFanSpeedControl"; };
    EasyDPPFanSpeedControl(uint8_t fanSpeedMode) : EasyFanSpeedControl(fanSpeedMode) {};
    EasyDPPFanSpeedControl(uint32_t mask) : EasyFanSpeedControl(mask) {};
}; // class EasyDPPFanSpeedControl


/**
 * @class EasyReadoutControl
 * @brief For user-friendly configuration of Readout Control mask.
 *
 * This register is mainly intended for VME boards, anyway some bits
 * are applicable also for DT and NIM boards.
 *
 * @param EasyReadoutControl::vMEInterruptLevel
 * VME Interrupt Level (VME boards only). Options are:\n
 * 0 = VME interrupts are disabled\n
 * 1,..,7 = sets the VME interrupt level.\n
 * NOTE: these bits are reserved in case of DT and NIM boards.
 * @param EasyReadoutControl::opticalLinkInterruptEnable
 * Optical Link Interrupt Enable. Op ons are:\n
 * 0 = Optical Link interrupts are disabled\n
 * 1 = Optical Link interrupts are enabled.
 * @param EasyReadoutControl::vMEBusErrorEventAlignedEnable
 * VME Bus Error / Event Aligned Readout Enable (VME boards
 * only). Options are:\n
 * 0 = VME Bus Error / Event Aligned Readout disabled (the module
 * sends a DTACK signal until the CPU inquires the module)\n
 * 1 = VME Bus Error / Event Aligned Readout enabled (the module is
 * enabled either to generate a Bus Error to finish a block transfer
 * or during the empty buffer readout in D32).\n
 * NOTE: this bit is reserved (must be 1) in case of DT and NIM boards.
 * @param EasyReadoutControl::vMEAlign64Mode
 * VME Align64 Mode (VME boards only). Options are:\n
 * 0 = 64-bit aligned readout mode disabled\n
 * 1 = 64-bit aligned readout mode enabled.\n
 * NOTE: this bit is reserved (must be 0) in case of DT and NIM boards.
 * @param EasyReadoutControl::vMEBaseAddressRelocation
 * VME Base Address Relocation (VME boards only). Options are:\n
 * 0 = Address Relocation disabled (VME Base Address is set by the on-board
 * rotary switches)\n
 * 1 = Address Relocation enabled (VME Base Address is set by
 * register 0xEF0C).\n
 * NOTE: this bit is reserved (must be 0) in case of DT and NIM boards.
 * @param EasyReadoutControl::interruptReleaseMode
 * Interrupt Release mode (VME boards only). Options are:\n
 * 0 = Release On Register Access (RORA): this is the default mode,
 * where interrupts are removed by disabling them either by setting
 * VME Interrupt Level to 0 (VME Interrupts) or by setting Optical
 * Link Interrupt Enable to 0\n
 * 1 = Release On Acknowledge (ROAK). Interrupts are automatically
 * disabled at the end of a VME interrupt acknowledge cycle (INTACK
 * cycle).\n
 * NOTE: ROAK mode is supported only for VME interrupts. ROAK mode
 * is not supported on interrupts generated over Optical Link.
 * NOTE: this bit is reserved (must be 0) in case of DT and NIM boards.
 * @param EasyReadoutControl::extendedBlockTransferEnable
 * Extended Block Transfer Enable (VME boarsd only). Selects the
 * memory interval allocated for block transfers. Options are:\n
 * 0 = Extended Block Transfer Space is disabled, and the block
 * transfer region is a 4kB in the 0x0000 - 0x0FFC interval\n
 * 1 = Extended Block Transfer Space is enabled, and the block
 * transfer is a 16 MB in the 0x00000000 - 0xFFFFFFFC interval.\n
 * NOTE: in Extended mode, the board VME Base Address is only set
 * via the on-board [31:28] rotary switches or bits[31:28] of
 * register 0xEF10.\n
 * NOTE: this register is reserved in case of DT and NIM boards.
 */
class EasyReadoutControl : public EasyBase
{
protected:
    /* Shared helpers since one constructor cannot reuse the other */
    /*
     * EasyReadoutControl fields:
     * VME interrupt level in [0:2], optical link interrupt enable in [3],
     * VME bus error / event aligned readout enable in [4],
     * VME align64 mode in [5], VME base address relocation in [6],
     * Interrupt release mode in [7],
     * extended block transfer enable in [8].
     */
    void initLayout() override
    {
        layout = {
                {"vMEInterruptLevel", {(const uint8_t)3, (const uint8_t)0}},
                {"opticalLinkInterruptEnable", {(const uint8_t)1, (const uint8_t)3}},
                {"vMEBusErrorEventAlignedEnable", {(const uint8_t)1, (const uint8_t)4}},
                {"vMEAlign64Mode", {(const uint8_t)1, (const uint8_t)5}},
                {"vMEBaseAddressRelocation", {(const uint8_t)1, (const uint8_t)6}},
                {"interruptReleaseMode", {(const uint8_t)1, (const uint8_t)7}},
                {"extendedBlockTransferEnable", {(const uint8_t)1, (const uint8_t)8}}
        };
    }
    /* NOTE: use inherited generic constructFromMask(mask) */
    void construct(const uint8_t vMEInterruptLevel, const uint8_t opticalLinkInterruptEnable, const uint8_t vMEBusErrorEventAlignedEnable, const uint8_t vMEAlign64Mode, const uint8_t vMEBaseAddressRelocation, const uint8_t interruptReleaseMode, const uint8_t extendedBlockTransferEnable) {
        initLayout();
        variables = {
                {"vMEInterruptLevel", (const uint8_t)(vMEInterruptLevel & 0x7)},
                {"opticalLinkInterruptEnable", (const uint8_t)(opticalLinkInterruptEnable & 0x1)},
                {"vMEBusErrorEventAlignedEnable", (const uint8_t)(vMEBusErrorEventAlignedEnable & 0x1)},
                {"vMEAlign64Mode", (const uint8_t)(vMEAlign64Mode & 0x1)},
                {"vMEBaseAddressRelocation", (const uint8_t)(vMEBaseAddressRelocation & 0x1)},
                {"interruptReleaseMode", (const uint8_t)(interruptReleaseMode & 0x1)},
                {"extendedBlockTransferEnable", (const uint8_t)(extendedBlockTransferEnable & 0x1)}
        };
    };
public:
    virtual const std::string getClassName() const override { return "EasyReadoutControl"; };
    /* Construct using default values from docs */
    EasyReadoutControl(const uint8_t vMEInterruptLevel, const uint8_t opticalLinkInterruptEnable, const uint8_t vMEBusErrorEventAlignedEnable, const uint8_t vMEAlign64Mode, const uint8_t vMEBaseAddressRelocation, const uint8_t interruptReleaseMode, const uint8_t extendedBlockTransferEnable)
    {
        construct(vMEInterruptLevel, opticalLinkInterruptEnable, vMEBusErrorEventAlignedEnable, vMEAlign64Mode, vMEBaseAddressRelocation, interruptReleaseMode, extendedBlockTransferEnable);
    }
    /* Construct from low-level bit mask in line with docs */
    EasyReadoutControl(const uint32_t mask)
    {
        constructFromMask(mask);
    }
}; // class EasyReadoutControl


/**
 * @class EasyDPPReadoutControl
 * @brief For user-friendly configuration of Readout Control mask.
 *
 * NOTE: identical to EasyReadoutControl
 */
class EasyDPPReadoutControl : public EasyReadoutControl
{
    /* Just inherit everything else from parent class */
public:
    virtual const std::string getClassName() const override { return "EasyDPPReadoutControl"; };
    EasyDPPReadoutControl(const uint8_t vMEInterruptLevel, const uint8_t opticalLinkInterruptEnable, const uint8_t vMEBusErrorEventAlignedEnable, const uint8_t vMEAlign64Mode, const uint8_t vMEBaseAddressRelocation, const uint8_t interruptReleaseMode, const uint8_t extendedBlockTransferEnable) : EasyReadoutControl(vMEInterruptLevel, opticalLinkInterruptEnable, vMEBusErrorEventAlignedEnable, vMEAlign64Mode, vMEBaseAddressRelocation, interruptReleaseMode, extendedBlockTransferEnable){};
    EasyDPPReadoutControl(const uint32_t mask) : EasyReadoutControl(mask) {};
}; // class EasyDPPReadoutControl


/**
 * @class EasyReadoutStatus
 * @brief For user-friendly configuration of Readout Status mask.
 *
 * This register contains informa on related to the readout.
 *
 * @param EasyReadoutStatus::eventReady
 * Event Ready. Indicates if there are events stored ready for
 * readout. Options are:\n
 * 0 = no data ready\n
 * 1 = event ready.
 * @param EasyReadoutStatus::outputBufferStatus
 * Output Buffer Status. Indicates if the Output Buffer is in Full
 * condition. Options are:\n
 * 0 = the Output Buffer is not FULL\n
 * 1 = the Output Buffer is FULL.
 * @param EasyReadoutStatus::busErrorSlaveTerminated
 * Bus Error (VME boards) / Slave-Terminated (DT/NIM boards) Flag.
 * Options are:\n
 * 0 = no Bus Error occurred (VME boards) or no terminated transfer
 * (DT/NIM boards)\n
 * 1 = a Bus Error occurred (VME boards) or one transfer has been
 * terminated by the digitizer in consequence of an unsupported
 * register access or block transfer prematurely terminated in event
 * aligned readout (DT/NIM).\n
 * NOTE: this bit is reset a er register readout at 0xEF04.
 */
class EasyReadoutStatus : public EasyBase
{
protected:
    /* Shared helpers since one constructor cannot reuse the other */
    /*
     * EasyReadoutStatus fields:
     * event ready [0], output buffer status in [1],
     * bus error / slave terminated in [2].
     */
    void initLayout() override
    {
        layout = {
                {"eventReady", {(const uint8_t)1, (const uint8_t)0}},
                {"outputBufferStatus", {(const uint8_t)1, (const uint8_t)1}},
                {"busErrorSlaveTerminated", {(const uint8_t)1, (const uint8_t)2}},
        };
    }
    /* NOTE: use inherited generic constructFromMask(mask) */
    void construct(const uint8_t eventReady, const uint8_t outputBufferStatus, const uint8_t busErrorSlaveTerminated) {
        initLayout();
        variables = {
                {"eventReady", (const uint8_t)(eventReady & 0x1)},
                {"outputBufferStatus", (const uint8_t)(outputBufferStatus & 0x1)},
                {"busErrorSlaveTerminated", (const uint8_t)(busErrorSlaveTerminated & 0x1)},
        };
    };
public:
    virtual const std::string getClassName() const override { return "EasyReadoutStatus"; };
    /* Construct using default values from docs */
    EasyReadoutStatus(const uint8_t eventReady, const uint8_t outputBufferStatus, const uint8_t busErrorSlaveTerminated)
    {
        construct(eventReady, outputBufferStatus, busErrorSlaveTerminated);
    }
    /* Construct from low-level bit mask in line with docs */
    EasyReadoutStatus(const uint32_t mask)
    {
        constructFromMask(mask);
    }
}; // class EasyReadoutStatus


/**
 * @class EasyDPPReadoutStatus
 * @brief For user-friendly configuration of Readout Status mask.
 *
 * NOTE: identical to EasyReadoutStatus
 */
class EasyDPPReadoutStatus : public EasyReadoutStatus
{
    /* Just inherit everything else from parent class */
public:
    virtual const std::string getClassName() const override { return "EasyDPPReadoutStatus"; };
    /* Construct using default values from docs */
    EasyDPPReadoutStatus(const uint8_t eventReady, const uint8_t outputBufferStatus, const uint8_t busErrorSlaveTerminated) : EasyReadoutStatus(eventReady, outputBufferStatus, busErrorSlaveTerminated) {};
    EasyDPPReadoutStatus(const uint32_t mask) : EasyReadoutStatus(mask) {};
}; // class EasyDPPReadoutStatus


/**
 * @class EasyScratch
 * @brief For user-friendly configuration of Scratch mask.
 *
 * This register is used for dummy read and write testing, fields
 * are arbitrarily chosen.
 *
 * @param EasyScratch::dummy1
 * A 1-bit test.
 * @param EasyScratch::dummy2
 * A 1-bit test.
 * @param EasyScratch::dummy3
 * A 1-bit test.
 * @param EasyScratch::dummy4
 * A 3-bit test.
 * @param EasyScratch::dummy5
 * A 5-bit test.
 * @param EasyScratch::dummy6
 * A 6-bit test.
 * @param EasyScratch::dummy7
 * A 7-bit test.
 * @param EasyScratch::dummy8
 * A 8-bit test.
 */
class EasyScratch : public EasyBase
{
protected:
    /* Shared helpers since one constructor cannot reuse the other */
    /*
     * EasyScratch fields:
     * dummy1 in [0], dummy2 in [1], dummy3 in [2], dummy4 in [3:5],
     * dummy5 in [6:10], dummy6 in [11:16], dummy8 in [17:23],
     * dummy8 in [24:31].
     */
    void initLayout() override
    {
        layout = {
                {"dummy1", {(const uint8_t)1, (const uint8_t)0}},
                {"dummy2", {(const uint8_t)1, (const uint8_t)1}},
                {"dummy3", {(const uint8_t)1, (const uint8_t)2}},
                {"dummy4", {(const uint8_t)3, (const uint8_t)3}},
                {"dummy5", {(const uint8_t)5, (const uint8_t)6}},
                {"dummy6", {(const uint8_t)6, (const uint8_t)11}},
                {"dummy7", {(const uint8_t)7, (const uint8_t)17}},
                {"dummy8", {(const uint8_t)8, (const uint8_t)24}}
        };
    }
    /* NOTE: use inherited generic constructFromMask(mask) */
    void construct(const uint8_t dummy1, const uint8_t dummy2, const uint8_t dummy3, const uint8_t dummy4, const uint8_t dummy5, const uint8_t dummy6, const uint8_t dummy7, const uint8_t dummy8) {
        initLayout();
        variables = {
                {"dummy1", (const uint8_t)(dummy1 & 0x1)},
                {"dummy2", (const uint8_t)(dummy2 & 0x1)},
                {"dummy3", (const uint8_t)(dummy3 & 0x1)},
                {"dummy4", (const uint8_t)(dummy4 & 0x7)},
                {"dummy5", (const uint8_t)(dummy5 & 0xF)},
                {"dummy6", (const uint8_t)(dummy6 & 0x1F)},
                {"dummy7", (const uint8_t)(dummy7 & 0x3F)},
                {"dummy8", (const uint8_t)(dummy8 & 0xFF)},
        };
    };
public:
    virtual const std::string getClassName() const override { return "EasyScratch"; };
    /* Construct using default values from docs */
    EasyScratch(const uint8_t dummy1, const uint8_t dummy2, const uint8_t dummy3, const uint8_t dummy4, const uint8_t dummy5, const uint8_t dummy6, const uint8_t dummy7, const uint8_t dummy8)
    {
        construct(dummy1, dummy2, dummy3, dummy4, dummy5, dummy6, dummy7, dummy8);
    }
    /* Construct from low-level bit mask in line with docs */
    EasyScratch(const uint32_t mask)
    {
        constructFromMask(mask);
    }
}; // class EasyScratch


/**
 * @class EasyDPPScratch
 * @brief For user-friendly configuration of Readout Status mask.
 *
 * NOTE: identical to EasyScratch
 */
class EasyDPPScratch : public EasyScratch
{
    /* Just inherit everything else from parent class */
public:
    virtual const std::string getClassName() const override { return "EasyDPPScratch"; };
    /* Construct using default values from docs */
    EasyDPPScratch(const uint8_t dummy1, const uint8_t dummy2, const uint8_t dummy3, const uint8_t dummy4, const uint8_t dummy5, const uint8_t dummy6, const uint8_t dummy7, const uint8_t dummy8) : EasyScratch(dummy1, dummy2, dummy3, dummy4, dummy5, dummy6, dummy7, dummy8) {};
    EasyDPPScratch(const uint32_t mask) : EasyScratch(mask) {};
}; // class EasyDPPScratch


/**
 * @class EasyAMCFirmwareRevision
 * @brief For user-friendly configuration of AMC Firmware Revision.
 *
 * This register contains the channel FPGA (AMC) firmware revision
 * information. The complete format is:\n
 * Firmware Revision = X.Y (16 lower bits)\n
 * Firmware Revision Date = Y/M/DD (16 higher bits)\n
 * EXAMPLE 1: revision 1.03, November 12th, 2007 is 0x7B120103.\n
 * EXAMPLE 2: revision 2.09, March 7th, 2016 is 0x03070209.\n
 * NOTE: the nibble code for the year makes this information to roll
 * over each 16 years.
 *
 * @param EasyAMCFirmwareRevision::minorRevisionNumber
 * AMC Firmware Minor Revision Number (Y).
 * @param EasyAMCFirmwareRevision::majorRevisionNumber
 * AMC Firmware Major Revision Number (X).
 * @param EasyAMCFirmwareRevision::revisionDate
 * AMC Firmware Revision Date (Y/M/DD).
 */
class EasyAMCFirmwareRevision : public EasyBase
{
protected:
    /* Shared base since one constructor cannot reuse the other */
    /*
     * EasyAMCFirmwareRevision fields:
     * minor revision number in [0:7], major revision number in [8:15],
     * revision date in [16:31].
     */
    /* NOTE: we split revision date into the four digits internally
     since it is just four 4-bit values clamped into 16-bit anyway.
     This makes the generic and DPP version much more similar, too. */
    void initLayout()
    {
        layout = {
                {"minorRevisionNumber", {(const uint8_t)8, (const uint8_t)0}},
                {"majorRevisionNumber", {(const uint8_t)8, (const uint8_t)8}},
                {"revisionDayLower", {(const uint8_t)4, (const uint8_t)16}},
                {"revisionDayUpper", {(const uint8_t)4, (const uint8_t)20}},
                {"revisionMonth", {(const uint8_t)4, (const uint8_t)24}},
                {"revisionYear", {(const uint8_t)4, (const uint8_t)28}}
        };
    }
    void construct(const uint8_t minorRevisionNumber, const uint8_t majorRevisionNumber, const uint8_t revisionDayLower, const uint8_t revisionDayUpper, const uint8_t revisionMonth, const uint8_t revisionYear)
    {
        initLayout();
        variables = {
                {"minorRevisionNumber", (const uint8_t)minorRevisionNumber},
                {"majorRevisionNumber", (const uint8_t)majorRevisionNumber},
                {"revisionDayLower", (const uint8_t)(revisionDayLower & 0x7)},
                {"revisionDayUpper", (const uint8_t)(revisionDayUpper & 0x7)},
                {"revisionMonth", (const uint8_t)(revisionMonth & 0x7)},
                {"revisionYear", (const uint8_t)(revisionYear & 0x7)}
        };
    }
public:
    virtual const std::string getClassName() const override { return "EasyAMCFirmwareRevision"; };
    /* Construct using default values from docs */
    /* We allow both clamped and individual revision date format here */
    EasyAMCFirmwareRevision(const uint8_t minorRevisionNumber, const uint8_t majorRevisionNumber, const uint16_t revisionDate)
    {
        const uint8_t revisionDayLower = (const uint8_t)(revisionDate & 0x7);
        const uint8_t revisionDayUpper = (const uint8_t)((revisionDate >> 4) & 0x7);
        const uint8_t revisionMonth = (const uint8_t)((revisionDate >> 8) & 0x7);
        const uint8_t revisionYear = (const uint8_t)((revisionDate >> 12) & 0x7);
        construct(minorRevisionNumber, majorRevisionNumber, revisionDayLower, revisionDayUpper, revisionMonth, revisionYear);
    }
    EasyAMCFirmwareRevision(const uint8_t minorRevisionNumber, const uint8_t majorRevisionNumber, const uint8_t revisionDayLower, const uint8_t revisionDayUpper, const uint8_t revisionMonth, const uint8_t revisionYear)
    {
        construct(minorRevisionNumber, majorRevisionNumber, revisionDayLower, revisionDayUpper, revisionMonth, revisionYear);
    }
    /* Construct from low-level bit mask in line with docs */
    EasyAMCFirmwareRevision(const uint32_t mask)
    {
        constructFromMask(mask);
    }
}; // class EasyAMCFirmwareRevision


/**
 * @class EasyDPPAMCFirmwareRevision
 * @brief For user-friendly configuration of DPP AMC FPGA Firmware Revision.
 *
 * Returns the DPP firmware revision (mezzanine level).
 * To control the mother board firmware revision see register 0x8124.\n
 * For example: if the register value is 0xC3218303:\n
 * - Firmware Code and Firmware Revision are 131.3\n
 * - Build Day is 21\n
 * - Build Month is March\n
 * - Build Year is 2012.\n
 * NOTE: since 2016 the build year started again from 0.
 *
 * @param EasyDPPAMCFirmwareRevision::firmwareRevisionNumber
 * Firmware revision number
 * @param EasyDPPAMCFirmwareRevision::firmwareDPPCode
 * Firmware DPP code. Each DPP firmware has a unique code.
 * @param EasyDPPAMCFirmwareRevision::buildDayLower
 * Build Day (lower digit)
 * @param EasyDPPAMCFirmwareRevision::buildDayUpper
 * Build Day (upper digit)
 * @param EasyDPPAMCFirmwareRevision::buildMonth
 * Build Month. For example: 3 means March, 12 is December.
 * @param EasyDPPAMCFirmwareRevision::buildYear
 * Build Year. For example: 0 means 2000, 12 means 2012. NOTE: since
 * 2016 the build year started again from 0.
 */
class EasyDPPAMCFirmwareRevision : public EasyBase
{
protected:
    /* Shared base since one constructor cannot reuse the other */
    /*
     * EasyDPPAMCFirmwareRevision fields:
     * firmware revision number in [0:7], firmware DPP code in [8:15],
     * build day lower in [16:19], build day upper in [20:23],
     * build month in [24:27], build year in [28:31]
     */
    void initLayout()
    {
        layout = {
                {"firmwareRevisionNumber", {(const uint8_t)8, (const uint8_t)0}},
                {"firmwareDPPCode", {(const uint8_t)8, (const uint8_t)8}},
                {"buildDayLower", {(const uint8_t)4, (const uint8_t)16}},
                {"buildDayUpper", {(const uint8_t)4, (const uint8_t)20}},
                {"buildMonth", {(const uint8_t)4, (const uint8_t)24}},
                {"buildYear", {(const uint8_t)4, (const uint8_t)28}}
        };
    }
    /* NOTE: use inherited generic constructFromMask(mask) */
    void construct(const uint8_t firmwareRevisionNumber, const uint8_t firmwareDPPCode, const uint8_t buildDayLower, const uint8_t buildDayUpper, const uint8_t buildMonth, const uint8_t buildYear)
    {
        initLayout();
        variables = {
                {"firmwareRevisionNumber", (const uint8_t)firmwareRevisionNumber},
                {"firmwareDPPCode", (const uint8_t)firmwareDPPCode},
                {"buildDayLower", (const uint8_t)(buildDayLower & 0xF)},
                {"buildDayUpper", (const uint8_t)(buildDayUpper & 0xF)},
                {"buildMonth", (const uint8_t)(buildMonth & 0xF)},
                {"buildYear", (const uint8_t)(buildYear & 0xF)}
        };
    }
public:
    virtual const std::string getClassName() const override { return "EasyDPPAMCFirmwareRevision"; };
    EasyDPPAMCFirmwareRevision(const uint8_t firmwareRevisionNumber, const uint8_t firmwareDPPCode, const uint8_t buildDayLower, const uint8_t buildDayUpper, const uint8_t buildMonth, const uint8_t buildYear)
    {
        construct(firmwareRevisionNumber, firmwareDPPCode, buildDayLower, buildDayUpper, buildMonth, buildYear);
    }
    /* Construct from low-level bit mask in line with docs */
    EasyDPPAMCFirmwareRevision(const uint32_t mask)
    {
        constructFromMask(mask);
    }
}; // class EasyDPPAMCFirmwareRevision


/**
 * @class EasyDPPAlgorithmControl
 * @brief For user-friendly configuration of DPP Algorithm Control mask.
 *
 * Management of the DPP algorithm features.
 *
 * @param EasyDPPAlgorithmControl::chargeSensitivity
 * Charge Sensitivity: defines how many pC of charge correspond to
 * one channel of the energy spectrum. Options are:\n
 * 000: 0.16 pC\n
 * 001: 0.32 pC\n
 * 010: 0.64 pC\n
 * 011: 1.28 pC\n
 * 100: 2.56 pC\n
 * 101: 5.12 pC\n
 * 110: 10.24 pC\n
 * 111: 20.48 pC.
 * @param EasyDPPAlgorithmControl::internalTestPulse
 * Internal Test Pulse. It is possible to enable an internal test
 * pulse for debugging purposes. The ADC counts are replaced with
 * the built-in pulse emulator. Options are:\n
 * 0: disabled\n
 * 1: enabled.
 * @param EasyDPPAlgorithmControl::testPulseRate
 * Test Pulse Rate. Set the rate of the built-in test pulse
 * emulator. Options are:\n
 * 00: 1 kHz\n
 * 01: 10 kHz\n
 * 10: 100 kHz\n
 * 11: 1 MHz.
 * @param EasyDPPAlgorithmControl::chargePedestal
 * Charge Pedestal: when enabled a fixed value of 1024 is added to
 * the charge. This feature is useful in case of energies close to
 * zero.
 * @param EasyDPPAlgorithmControl::inputSmoothingFactor
 * Input smoothing factor n. In case of noisy signal it is possible
 * to apply a smoothing filter, where each sample is replaced with
 * the mean value of n previous samples. When enabled, the trigger
 * is evaluated on the smoothed samples, while the charge
 * integration will be performed on the samples corresponding to the
 * ”Analog Probe” selection (bits[13:12] of register 0x8000). In any
 * case the output data contains the smoothed samples. Options are:\n
 * 000: disabled\n
 * 001: 2 samples\n
 * 010: 4 samples\n
 * 011: 8 samples\n
 * 100: 16 samples\n
 * 101: 32 samples\n
 * 110: 64 samples\n
 * 111: Reserved.
 * @param EasyDPPAlgorithmControl::pulsePolarity
 * Pulse Polarity. Options are:\n
 * 0: positive pulse\n
 * 1: negative pulse.
 * @param EasyDPPAlgorithmControl::triggerMode
 * Trigger Mode. Options are:\n
 * 00: Normal mode. Each channel can self-trigger independently from
 * the other channels.\n
 * 01: Paired mode. Each channel of a couple ’n’ acquire the event
 * in logic OR between its self-trigger and the self-trigger of the
 * other channel of the couple. Couple n corresponds to channel n
 * and channel n+2\n
 * 10: Reserved\n
 * 11: Reserved.
 * @param EasyDPPAlgorithmControl::baselineMean
 * Baseline Mean. Sets the number of events for the baseline mean
 * calculation. Options are:\n
 * 000: Fixed: the baseline value is fixed to the value set in
 * register 0x1n38\n
 * 001: 4 samples\n
 * 010: 16 samples\n
 * 011: 64 samples.
 * @param EasyDPPAlgorithmControl::disableSelfTrigger
 * Disable Self Trigger. If disabled, the self-trigger can be still
 * propagated to the TRG-OUT front panel connector, though it is not
 * used by the channel to acquire the event. Options are:\n
 * 0: self-trigger enabled\n
 * 1: self-trigger disabled.
 * @param EasyDPPAlgorithmControl::triggerHysteresis
 * Trigger Hysteresis. The trigger can be inhibited during the
 * trailing edge of a pulse, to avoid re-triggering on the pulse
 * itself. Options are:\n
 * 0 (default value): enabled\n
 * 1: disabled.
 */
class EasyDPPAlgorithmControl : public EasyBase
{
protected:
    /* Shared base since one constructor cannot reuse the other */
    /*
     * EasyDPPAlgorithmControl fields:
     * charge sensitivity in [0:2], internal test pulse in [4],
     * test pulse rate in [5:6], charge pedestal in [8],
     * input smoothing factor in [12:14], pulse polarity in [16],
     * trigger mode in [18:19], baseline mean in [20:22],
     * disable self trigger in [24], trigger hysteresis in [30].
     */
    void initLayout()
    {
        layout = {
                {"chargeSensitivity", {(const uint8_t)3, (const uint8_t)0}},
                {"internalTestPulse", {(const uint8_t)1, (const uint8_t)4}},
                {"testPulseRate", {(const uint8_t)2, (const uint8_t)5}},
                {"chargePedestal", {(const uint8_t)1, (const uint8_t)8}},
                {"inputSmoothingFactor", {(const uint8_t)3, (const uint8_t)12}},
                {"pulsePolarity", {(const uint8_t)1, (const uint8_t)16}},
                {"triggerMode", {(const uint8_t)2, (const uint8_t)18}},
                {"baselineMean", {(const uint8_t)3, (const uint8_t)20}},
                {"disableSelfTrigger", {(const uint8_t)1, (const uint8_t)24}},
                {"triggerHysteresis", {(const uint8_t)1, (const uint8_t)30}}
        };
    }
    /* NOTE: use inherited generic constructFromMask(mask) */
    void construct(const uint8_t chargeSensitivity, const uint8_t internalTestPulse, const uint8_t testPulseRate, const uint8_t chargePedestal, const uint8_t inputSmoothingFactor, const uint8_t pulsePolarity, const uint8_t triggerMode, const uint8_t baselineMean, const uint8_t disableSelfTrigger, const uint8_t triggerHysteresis)
    {
        initLayout();
        variables = {
                {"chargeSensitivity", (const uint8_t)(chargeSensitivity & 0x7)},
                {"internalTestPulse", (const uint8_t)(internalTestPulse & 0x1)},
                {"testPulseRate", (const uint8_t)(testPulseRate & 0x3)},
                {"chargePedestal", (const uint8_t)(chargePedestal & 0x1)},
                {"inputSmoothingFactor", (const uint8_t)(inputSmoothingFactor & 0x7)},
                {"pulsePolarity", (const uint8_t)(pulsePolarity & 0x1)},
                {"triggerMode", (const uint8_t)(triggerMode & 0x3)},
                {"baselineMean", (const uint8_t)(baselineMean & 0x7)},
                {"disableSelfTrigger", (const uint8_t)(disableSelfTrigger & 0x1)},
                {"triggerHysteresis", (const uint8_t)(triggerHysteresis & 0x1)}
        };
    }
public:
    virtual const std::string getClassName() const override { return "EasyDPPAlgorithmControl"; };
    EasyDPPAlgorithmControl(const uint8_t chargeSensitivity, const uint8_t internalTestPulse, const uint8_t testPulseRate, const uint8_t chargePedestal, const uint8_t inputSmoothingFactor, const uint8_t pulsePolarity, const uint8_t triggerMode, const uint8_t baselineMean, const uint8_t disableSelfTrigger, const uint8_t triggerHysteresis)
    {
        construct(chargeSensitivity, internalTestPulse, testPulseRate, chargePedestal, inputSmoothingFactor, pulsePolarity, triggerMode, baselineMean, disableSelfTrigger, triggerHysteresis);
    }
    /* Construct from low-level bit mask in line with docs */
    EasyDPPAlgorithmControl(const uint32_t mask)
    {
        constructFromMask(mask);
    }
}; // class EasyDPPAlgorithmControl

