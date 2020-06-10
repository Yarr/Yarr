#include "BdaqTxCore.h"
#include "logging.h"

namespace {
  auto logger = logging::make_log("BdaqTxCore");
}

BdaqTxCore::BdaqTxCore() {
}

BdaqTxCore::~BdaqTxCore() {
}

void BdaqTxCore::writeFifo(uint32_t value) {
    std::stringstream d; 
    d << __PRETTY_FUNCTION__ << " : Writing 0x" << std::hex << value << std::dec;
    logger->debug(d.str());

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
        logger->critical(error.str());
        exit(-1);
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
    std::stringstream d; 
    d << __PRETTY_FUNCTION__ << " : Value 0x" << std::hex << mask << std::dec;
    logger->debug(d.str());

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
    std::stringstream d; 
    d << __PRETTY_FUNCTION__ << " : Value 0x" << std::hex << mask << std::dec;
    logger->debug(d.str());
    
    if (mask == 0x01) {
        cmd.setOutputEn(true); 
    } else {
        cmd.setOutputEn(false);
    }
    enMask = mask;
}

uint32_t BdaqTxCore::getCmdEnable() {
    std::stringstream d; 
    d << __PRETTY_FUNCTION__;
    logger->debug(d.str());
    return 0;
}

bool BdaqTxCore::isCmdEmpty() {
    std::stringstream d; 
    d << __PRETTY_FUNCTION__;
    logger->debug(d.str());
    sendCommand();
    return true;
}

//==============================================================================
// Command Repeater (Trigger) Stuff
//==============================================================================

void BdaqTxCore::setTrigWord(uint32_t *word, uint32_t length) {
    std::stringstream d; 
    d << __PRETTY_FUNCTION__;
    logger->debug(d.str());
    
    // Converting YARR format to BDAQ format
    for (uint i=0; i<length; ++i) {
        trgData.push_back(word[length-i-1] >> 24 & 0xFF);
        trgData.push_back(word[length-i-1] >> 16 & 0xFF);
        trgData.push_back(word[length-i-1] >>  8 & 0xFF);
        trgData.push_back(word[length-i-1]       & 0xFF);
    }
}

void BdaqTxCore::setTrigCnt(uint32_t count) {
    std::stringstream d; 
    d << __PRETTY_FUNCTION__ << " : Count " << count;
    logger->debug(d.str());

    hardwareTriggerCount = (uint16_t) count;
    logger->info("Trigger Count: {}", count);
}

void BdaqTxCore::setTrigEnable(uint32_t value) {
    std::stringstream d; 
    d << __PRETTY_FUNCTION__ << " : Value 0x" << std::hex << value << std::dec;
    logger->debug(d.str());
    // Emulating SPEC register
    trgEnable = value; 
    // Timed Trigger (software implemented, for noise scans for example)
    if (timedTrigger) {
        if (value == 0x0) {
            timedTriggerThread.join();
            return;
        } else {
            timedTriggerAbort = false;
            timedTriggerSet();
            timedTriggerThread = std::thread(&BdaqTxCore::timedTriggerRun, this);
            return;
        }
    // Hardware Trigger Mode
    } else {
        if (value == 0x0) {
            return;
        } else {
            hardwareTriggerSet();
            hardwareTriggerRun();
        }
    }
    logger->info("Trigger Enable: {}", value);
}

uint32_t BdaqTxCore::getTrigEnable() {
    std::stringstream d; 
    d << __PRETTY_FUNCTION__;
    logger->debug(d.str());
    // Emulating SPEC register
    return trgEnable; 
}

void BdaqTxCore::maskTrigEnable(uint32_t value, uint32_t mask) {
    std::stringstream d; 
    d << __PRETTY_FUNCTION__ << " : Value 0x" << std::hex << value << std::dec;
    logger->debug(d.str());
}

void BdaqTxCore::setTrigConfig(enum TRIG_CONF_VALUE cfg) {
    std::stringstream d; 
    d << __PRETTY_FUNCTION__ << " : Config 0x" << std::hex << cfg << std::dec;
    logger->debug(d.str());
}

void BdaqTxCore::setTrigFreq(double freq) {
    std::stringstream d; 
    d << __PRETTY_FUNCTION__ << " : Frequency " << freq/1.0e3 << " kHz" <<std::endl;
    logger->debug(d.str());

    // One RD53A command (16-bit) spans 4 BCs = 100 ns.
    // The idea is converting the period into NOOP commands (taking 100 ns each)
    // for the POST DELAY in the command buffer (trigger buffer).
    // There might be a slight offset to the real period due to the other 
    // commands in the command buffer.
    hardwareTriggerNoop = (1.0f/freq)/100e-9; // Only for hardware trigger mode.
    // For Timed trigger
    timedTriggerFreq = freq;
    logger->info("Trigger Frequency: {}, NOOP Number: {}", freq, hardwareTriggerNoop);
}
  
void BdaqTxCore::setTrigTime(double time) {
    std::stringstream d; 
    d << __PRETTY_FUNCTION__ << " : Time " << time << " s, period " << m_clk_period <<std::endl;
    logger->debug(d.str());
    
    // Hardware Trigger
    if (time == 0) {
        timedTrigger = false; // No timed trigger, enables Hardware trigger
    }
    // Timed trigger mode (with time > 0)   
    else {
        timedTrigger = true;
        timedTriggerTime = time;
    }
    logger->info("Trigger Time: {}", time);
}

void BdaqTxCore::toggleTrigAbort() {
    std::stringstream d; 
    d << __PRETTY_FUNCTION__ << " : Toggling Trigger abort!";
    logger->debug(d.str());
    // Stop timed trigger loop
    timedTriggerAbort = true;
}

void BdaqTxCore::setTrigWordLength(uint32_t length) {
    std::stringstream d; 
    d << __PRETTY_FUNCTION__ << " : Length " << length << "-bit";
    logger->debug(d.str());
}

bool BdaqTxCore::isTrigDone() {
    std::stringstream d; 
    d << __PRETTY_FUNCTION__;
    logger->debug(d.str());
    // Timed Trigger
    if (timedTrigger) {
        return timedTriggerDone;
    // Normal Trigger
    } else {
        return cmd.isDone();
    }
}

//------------------------------------------------------------------------------
// Hardware Trigger
//------------------------------------------------------------------------------

void BdaqTxCore::hardwareTriggerSet() {
    // POST Delay
    std::vector<uint8_t> fixPostDelay(hardwareTriggerNoop*2, 0x69); 
    trgData.insert(trgData.end(), fixPostDelay.begin(), fixPostDelay.end());
    // Setting hardware registers/buffers
    cmd.setData(trgData);
    cmd.setSize(trgData.size()); 
    cmd.setRepetitions(hardwareTriggerCount);
    logger->info("Hardware Trigger Size (in bytes): {}", trgData.size());
    trgData.clear();
}

void BdaqTxCore::hardwareTriggerRun() {
    cmd.start(); 
}

//------------------------------------------------------------------------------
// Timed Trigger (software) Emulation 
//------------------------------------------------------------------------------

void BdaqTxCore::timedTriggerSet() {
    // Setting hardware registers/buffers
    cmd.setData(trgData);
    cmd.setSize(trgData.size()); 
    cmd.setRepetitions(1);
    logger->info("Timed Trigger Size (in bytes): {}", trgData.size());
    trgData.clear();
}

void BdaqTxCore::timedTriggerRun() {
    timedTriggerDone = false;
    auto start = std::chrono::system_clock::now();
    auto cur = std::chrono::system_clock::now();
    while (std::chrono::duration_cast<std::chrono::seconds>(cur - start).count() < timedTriggerTime) {
        cmd.start();
        // Frequency is implemented by software: 
        std::this_thread::sleep_for(std::chrono::microseconds((int)(1000/timedTriggerFreq))); 
        // Wait for trigger completion (which is likely shorter than the period 
        // above). Here for peace of mind.
        while(!cmd.isDone()); //wait for completion 
        cur = std::chrono::system_clock::now();
        if (timedTriggerAbort) break;
    }
    timedTriggerDone = true;
}
