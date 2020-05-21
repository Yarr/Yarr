#define FIX_POST_DELAY

#include "BdaqTxCore.h"

BdaqTxCore::BdaqTxCore() {
    verbose = false;
}

BdaqTxCore::~BdaqTxCore() {
}

void BdaqTxCore::writeFifo(uint32_t value) {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << " : Writing 0x" << std::hex << value << std::dec << std::endl;

    //Filling the Command Data Vector
    cmdData.push_back(value >> 24 & 0xFF);
    cmdData.push_back(value >> 16 & 0xFF);
    cmdData.push_back(value >>  8 & 0xFF);
    cmdData.push_back(value       & 0xFF);

    // Checking for cmd_rd53 buffer (FPGA) overflow.
    // Should never happen if isCmdEmpty() is correctly called.
    // Commands are written thru RBCP (UDP) which has a maximum payload of
    // 255 bytes per packet. Thus, 4080 is the maxiumum integer multiple of 255
    // and 4 (we get 32-bit words) being less than 4096 (max buffer size).
    if (cmdData.size() > 4080) {
        std::stringstream error; 
        error << __PRETTY_FUNCTION__ << ": cmd_rd53 buffer > 4080 bytes!"; 
        std::string errorStr(error.str());
        throw std::runtime_error(errorStr);
    }
}

void BdaqTxCore::sendCommand() {
    if (cmdData.size() == 0) return;  
    cmd.setData(cmdData);
    cmd.setSize(cmdData.size());
    cmd.setRepetitions(1);
    cmd.start();
    while(!cmd.isDone()); //wait for completion. 
    cmdData.clear();
}

void BdaqTxCore::setCmdEnable(uint32_t value) {
    uint32_t mask = (1 << value);
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << " : Value 0x" << std::hex << mask << std::dec << std::endl;
    
    if (mask == 0x01) {
        cmd.setOutputEn(true); 
    } else {
        cmd.setOutputEn(false);
    }

    enMask = mask;
}

void BdaqTxCore::setCmdEnable(std::vector<uint32_t> channels) {
    uint32_t mask = 0;
    for (uint32_t channel : channels) {
        mask += (1 << channel);
    }
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << " : Value 0x" << std::hex << mask << std::dec << std::endl;
    
    if (mask == 0x01) {
        cmd.setOutputEn(true); 
    } else {
        cmd.setOutputEn(false);
    }
    enMask = mask;
}

uint32_t BdaqTxCore::getCmdEnable() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
}

bool BdaqTxCore::isCmdEmpty() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;

    sendCommand();
    return true;
}

//==============================================================================
// Command Repeater Stuff
//==============================================================================

void BdaqTxCore::setTrigWord(uint32_t *word, uint32_t length) {
    if (verbose) {
        std::cout << __PRETTY_FUNCTION__ << " : " << std::endl;
    }

    // Converting YARR format to BDAQ format
    for (uint i=0; i<length; ++i) {
        trgData.push_back(word[length-i-1] >> 24 & 0xFF);
        trgData.push_back(word[length-i-1] >> 16 & 0xFF);
        trgData.push_back(word[length-i-1] >>  8 & 0xFF);
        trgData.push_back(word[length-i-1]       & 0xFF);
    }
    
    // POST Delay
    #ifdef FIX_POST_DELAY
    const uint fixPostDelaySize = 400;
    std::vector<uint8_t> fixPostDelay(fixPostDelaySize*2, 0x69); 
    trgData.insert(trgData.end(), fixPostDelay.begin(), fixPostDelay.end());
    #endif 
}

void BdaqTxCore::setTrigCnt(uint32_t count) {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << " : Count " << count << std::endl;
    
    trgRepetitions = count;
}

void BdaqTxCore::setTrigEnable(uint32_t value) {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << " : Value 0x" << std::hex << value << std::dec << std::endl;

    trgEnable = value; //Emulating SPEC register
    
    if (value == 0x0) return;

    cmd.setData(trgData);
    cmd.setSize(trgData.size()); 
    cmd.setRepetitions(trgRepetitions);
    cmd.start();
    trgData.clear();
}

uint32_t BdaqTxCore::getTrigEnable() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    return trgEnable; //Emulating SPEC register
}

void BdaqTxCore::maskTrigEnable(uint32_t value, uint32_t mask) {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << " : Value 0x" << std::hex << value << std::dec << std::endl;
}

void BdaqTxCore::setTrigConfig(enum TRIG_CONF_VALUE cfg) {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << " : Config 0x" << std::hex << cfg << std::dec << std::endl;
}

void BdaqTxCore::setTrigFreq(double freq) {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << " : Frequency " << freq/1.0e3 << " kHz" <<std::endl;
    //uint32_t tmp = 1.0/((double)m_clk_period * freq);
    
    // Change the number of generated NOOP frames to emulate Frequency setting?
}
  
void BdaqTxCore::setTrigTime(double time) {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << " : Time " << time << " s, period " << m_clk_period <<std::endl;
}

void BdaqTxCore::toggleTrigAbort() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << " : Toggling Trigger abort!" << std::endl;
}

void BdaqTxCore::setTrigWordLength(uint32_t length) {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << " : Length " << length << " bit" <<std::endl;
}

bool BdaqTxCore::isTrigDone() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    
    return cmd.isDone();
}
