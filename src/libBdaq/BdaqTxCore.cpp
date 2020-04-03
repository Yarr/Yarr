//#define FIX_TRIGGER 
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

    //Inserting a SYNC frame every 32 frames
    /*if (cmdData.size() % 32 == 0) {
        cmdData.push_back(0x81);
        cmdData.push_back(0x7E);
    }*/

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

void BdaqTxCore::setManualDigitalTrig() {

    std::vector<uint8_t> test;
    
    //  
    for (uint i=0; i<10; ++i) {
        test.push_back(0x69);
        test.push_back(0x69);
    }
    // cal (chip_id: 0)
    test.push_back(0x63);
    test.push_back(0x63);
    test.push_back(0xa9);
    test.push_back(0x71);
    test.push_back(0xa6);
    test.push_back(0x6a);
    //
    // 
    // Trigger Delay: should range from 117 to 124
    // BDAQ setting: 121
    // BDAQ LatencyConfig: 500
    // 121*4 = 484
    // 
    // ITkSw setting: 14 (14*4 = 56)
    // ITkSw LatencyConfig: 58
    //
    for (uint i=0; i<124; ++i) { 
        test.push_back(0x69);
        test.push_back(0x69);
    }
    // trigger -- Something here that BDAQ does NOT like.
    // Replace with BDAQ trigger and it should work
    /*test.push_back(0x56);
    test.push_back(0x6a);
    test.push_back(0x56);
    test.push_back(0x6c);
    test.push_back(0x56);
    test.push_back(0x71);
    test.push_back(0x56);
    test.push_back(0x72);*/
    // BDAQ Trigger
    for (uint i=0; i<8; ++i) {
        test.push_back(0x56);
        test.push_back(0x6a);
    }
    // noop
    for (uint i=0; i<7; ++i) { 
        test.push_back(0x69);
        test.push_back(0x69);
    }
    // Cal Arm
    test.push_back(0x63);
    test.push_back(0x63);
    test.push_back(0xa9);
    test.push_back(0x6a);
    test.push_back(0x6a);
    test.push_back(0x6a);
    // Post delay
    // BDAQ setting: 800
    // Reducing to 100 yielded a flawless digital scan.
    for (uint i=0; i<100; ++i) {
        test.push_back(0x69);
        test.push_back(0x69);
    }

    trgData = test;    
}

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
    
    //-------------------------------------------------------------------------
    // "Adapting" YARR commands
    //-------------------------------------------------------------------------

    #ifdef FIX_TRIGGER
    // For a standard digital scan, trigger command begins at index=96
    const uint triggerIndex = 96;

    // Erasing original trigger command
    trgData.erase(trgData.begin()+triggerIndex, trgData.begin()+triggerIndex+8);

    // Inserting BDAQ trigger command
    std::vector<uint8_t> fixTrigger;
    for (uint i=0; i<8; ++i) {
        fixTrigger.push_back(0x56);
        fixTrigger.push_back(0x6a);
    }
    trgData.insert(trgData.begin()+triggerIndex, fixTrigger.begin(), fixTrigger.end());
    #endif 

    #ifdef FIX_POST_DELAY
    // Although there are already 2 NoOp commands in the end of the 
    // command buffer (for a standard digital scan with trigDelay=56),
    // BDAQ needs more "post delay". Likely to avoid overrunning the
    // readout FIFO. Original value in BDAQ code is 800. I could go
    // down to 200 for faster scans. It might not work if the TCP 
    // connection is somehow slower. Also, there is still no dedicated
    // thread for the readout. It might help getting things faster.
    const uint fixPostDelaySize = 200;

    std::vector<uint8_t> fixPostDelay(fixPostDelaySize*2, 0x69); 
    trgData.insert(trgData.end(), fixPostDelay.begin(), fixPostDelay.end());
    #endif 

    //setManualDigitalTrig(); // Replace generated commands with manual commands

    if (once && verbose) {
        once = false;
        uint index = 0;
        std::cout << __PRETTY_FUNCTION__ << " : " << std::endl;
        for (const auto& t : trgData) {
            std::cout << "[" << index << "]: 0x" << std::hex << +t << std::dec << std::endl; 
            ++index;
        }
        std::cin.get();
    }
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

    //std::cout << "trgData.size() = " << trgData.size() << std::endl;
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
    
    //std::this_thread::sleep_for(std::chrono::milliseconds(200));
    return cmd.isDone();
}
