// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: RD53B Library
// # Comment: Combines ITkPixV1 and CROCv1
// # Date: May 2020
// ################################

#include "AllChips.h"
#include "Rd53b.h"

#include "logging.h"

// Create logger
namespace {
  auto logger = logging::make_log("Rd53b");
}

bool rd53b_registred =
    StdDict::registerFrontEnd("RD53B", [](){return std::unique_ptr<FrontEnd>(new Rd53b());});

Rd53b::Rd53b() : FrontEnd(), Rd53bCfg(), Rd53bCmd(){
    txChannel = 99;
    rxChannel = 99;
    active = true;
    geo.nRow = 384;
    geo.nCol = 400;
}

Rd53b::Rd53b(HwController *core) : FrontEnd(), Rd53bCfg(), Rd53bCmd(core) {
    //m_rxcore = core;
    txChannel = 99;
    rxChannel = 99;
    active = true;
    geo.nRow = 384;
    geo.nCol = 400;
    core->setClkPeriod(6.25e-9);
}

Rd53b::Rd53b(HwController *core, unsigned arg_channel) : FrontEnd(), Rd53bCfg(), Rd53bCmd(core){
    //m_rxcore = core;
    txChannel = arg_channel;
    rxChannel = arg_channel;
    active = true;
    geo.nRow = 384;
    geo.nCol = 400;
    core->setClkPeriod(6.25e-9);
}

void Rd53b::init(HwController *core, unsigned arg_txChannel, unsigned arg_rxChannel) {
    this->setCore(core);
    //m_rxcore = arg_core;
    txChannel = arg_txChannel;
    rxChannel = arg_rxChannel;
    geo.nRow = 384;
    geo.nCol = 400;
    core->setClkPeriod(6.25e-9);
}

void Rd53b::configure() {

}

void Rd53b::configureInit() {
    logger->debug("Initiliasing chip ...");
    
    // TODO this should only be done once per TX!
    // Send low number of transitions for at least 2us to put chip in reset state 
    logger->debug(" ... asserting CMD reset via low activity");
    for (unsigned int i=0; i<30; i++) {
        core->writeFifo(0xFFFFFFFF);
        core->writeFifo(0x00000000);
    }
    core->releaseFifo();
    while(!core->isCmdEmpty());
    // Wait for at least 0.5us before chip is release from reset
    logger->debug(" ... waiting for CMD reset to be released");
    std::this_thread::sleep_for(std::chrono::microseconds(500));

    // Send a clear cmd
    logger->debug(" ... sending clear command");
    this->sendClear(m_chipId);
    while(!core->isCmdEmpty());

    // Enable register writing to do more resetting
    logger->debug(" ... set global register in writeable mode");
    this->writeRegister(&Rd53b::GcrDefaultConfig, 0xAC75);
    this->writeRegister(&Rd53b::GcrDefaultConfigB, 0x538A);
    while(!core->isCmdEmpty());

    // Send a global pulse to reset multiple things
    logger->debug(" ... send resets via global pulse");
    this->writeRegister(&Rd53b::GlobalPulseConf, 0x0FFF);
    this->writeRegister(&Rd53b::GlobalPulseWidth, 10);
    while(!core->isCmdEmpty());
    this->sendGlobalPulse(m_chipId);
    while(!core->isCmdEmpty());
    std::this_thread::sleep_for(std::chrono::microseconds(500));
    // Reset register
    this->writeRegister(&Rd53b::GlobalPulseConf, 0);

    logger->debug("Chip initialisation done!");
}

void Rd53b::configureGlobal() {
    logger->debug("Configuring all registers ...");
    for (unsigned addr=0; addr<numRegs; addr++) {
        this->sendWrReg(m_chipId, addr, m_cfg[addr]);
        if (addr % 20 == 0) // Wait every 20 regs to not overflow a buffer
            while(!core->isCmdEmpty()){;}
    }
    while(!core->isCmdEmpty());
}

void Rd53b::configurePixels() {
    logger->debug("Configure all pixel registers ...");
    // Setup pixel programming
    this->writeRegister(&Rd53b::PixAutoRow, 1);

    // Writing two columns and six rows at the same time
    for (unsigned dc=0; dc<n_DC; dc++) {
        this->writeRegister(&Rd53b::PixRegionCol, dc);
        this->writeRegister(&Rd53b::PixRegionRow, 0); 
        for (unsigned row=0; row<n_Row; row+=1) {
            this->writeRegister(&Rd53b::PixPortal, pixRegs[dc][row]);
        }
        while(!core->isCmdEmpty()){;}
    }
}

void Rd53b::writeRegister(Rd53bReg Rd53bGlobalCfg::*ref, uint16_t value) {
    logger->debug("Writing register #{} with 0x{0:x}", (this->*ref).addr(), m_cfg[(this->*ref).addr()]);
    (this->*ref).write(value);
    this->sendWrReg(m_chipId, (this->*ref).addr(), m_cfg[(this->*ref).addr()]);
}

void Rd53b::readRegister(Rd53bReg Rd53bGlobalCfg::*ref) {
    logger->debug("Reading register #{}", (this->*ref).addr());
    this->sendRdReg(m_chipId, (this->*ref).addr());
}

void Rd53b::writeNamedRegister(std::string name, uint16_t value) {
    if(regMap.find(name) != regMap.end()) {
        logger->info("Write named register {} -> 0x{0:x}", name, value);
        this->writeRegister(regMap[name], value);
    } else {
        logger->error("Trying to write named register, register not found: {}", name);
    }
}
