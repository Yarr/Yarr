// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: FE-I4 Library
// # Comment: FEI4 Base class
// ################################

#include "AllChips.h"
#include "Fei4.h"

bool fei4_registered =
StdDict::registerFrontEnd("FEI4B",
        []() { return std::unique_ptr<FrontEnd>(new Fei4());});

Fei4::Fei4() : Fei4Cfg(), Fei4Cmd(), FrontEnd() {
    txChannel = 99;
    rxChannel = 99;
    active = false;
    geo.nRow = 336;
    geo.nCol = 80;
}

Fei4::Fei4(TxCore *core) : Fei4Cfg(), Fei4Cmd(core), FrontEnd() {
    txChannel = 99;
    rxChannel = 99;
    //histogrammer = NULL;
    //ana = NULL;
    active = true;
    geo.nRow = 336;
    geo.nCol = 80;
}

Fei4::Fei4(TxCore *core, unsigned arg_channel) : Fei4Cfg(), Fei4Cmd(core), FrontEnd() {
    txChannel = arg_channel;
    rxChannel = arg_channel;
    //histogrammer = NULL;
    //ana = NULL;
    active = true;
    geo.nRow = 336;
    geo.nCol = 80;
}

Fei4::Fei4(TxCore *core, unsigned arg_txChannel, unsigned arg_rxChannel) : Fei4Cfg(), Fei4Cmd(core), FrontEnd() {
    txChannel = arg_txChannel;
    rxChannel = arg_rxChannel;
    //histogrammer = NULL;
    //ana = NULL;
    active = true;
    geo.nRow = 336;
    geo.nCol = 80;
}

Fei4::~Fei4() {	

}

void Fei4::init(TxCore *arg_core, unsigned arg_txChannel, unsigned arg_rxChannel) {
    this->setCore(arg_core);
    txChannel = arg_txChannel;
    rxChannel = arg_rxChannel;
    active = true;
}

void Fei4::configure() {
    this->configureGlobal();
    this->configurePixels();
}

void Fei4::configureGlobal() {
    runMode(chipId, false);

    // Increase threshold
    uint16_t tmp = getValue(&Fei4::Vthin_Coarse);
    writeRegister(&Fei4::Vthin_Coarse, 255);

    for (unsigned i=0; i<numRegs; i++) {
        wrRegister(chipId, i, cfg[i]);
    }

    // Request all Service Records
    writeRegister(&Fei4::ReadErrorReq, 0x1);
    globalPulse(chipId, 10);
    writeRegister(&Fei4::ReadErrorReq, 0x0);


    // Set actual threshold
    setValue(&Fei4::Vthin_Coarse, tmp);
    writeRegister(&Fei4::Vthin_Coarse);
}

void Fei4::configurePixels(unsigned lsb, unsigned msb) {
    // Increase threshold
    uint16_t tmp = getValue(&Fei4::Vthin_Coarse);
    writeRegister(&Fei4::Vthin_Coarse, 255);

    // Write Pixel Mask
    writeRegister(&Fei4::Colpr_Mode, 0x0);
    for (unsigned dc=0; dc<Fei4PixelCfg::n_DC; dc++) {
        writeRegister(&Fei4::Colpr_Addr, dc);
        for (unsigned bit=lsb; bit<msb; bit++) {
            wrFrontEnd(chipId, getCfg(bit, dc));
            loadIntoPixel(1 << bit);
            while(core->isCmdEmpty() == 0);
        }
    }
    // Set actual threshold
    setValue(&Fei4::Vthin_Coarse, tmp);
    writeRegister(&Fei4::Vthin_Coarse);
}

void Fei4::initMask(enum MASK_STAGE mask) {
    uint32_t bitstream[21];
    for(unsigned i=0; i<21; i++)
        bitstream[i] = mask;
    wrFrontEnd(chipId, bitstream);
}

void Fei4::initMask(uint32_t mask) {
    uint32_t bitstream[21];
    for(unsigned i=0; i<21; i++)
        bitstream[i] = mask;
    wrFrontEnd(chipId, bitstream);
}

void Fei4::shiftMask() {
    this->loadIntoShiftReg(0x1);
    this->loadIntoPixel(0x1);
    this->loadIntoShiftReg(0x1);
    this->shiftByOne();
}

// Inverts pixel latch
void Fei4::loadIntoShiftReg(unsigned pixel_latch) {
    // Select Pixel latch to copy into SR
    writeRegister(&Fei4::Pixel_latch_strobe, pixel_latch);
    // Select SR in Parallel Input Mode
    writeRegister(&Fei4::S1, 0x1);
    writeRegister(&Fei4::S0, 0x1);
    writeRegister(&Fei4::SR_Clock, 0x1);

    // Copy from Latches into SR
    globalPulse(chipId, 10);

    // Reset SR regs
    writeRegister(&Fei4::S1, 0x0);
    writeRegister(&Fei4::S0, 0x0);
    writeRegister(&Fei4::SR_Clock, 0x0);
    writeRegister(&Fei4::Pixel_latch_strobe, 0x0);
}

void Fei4::loadIntoPixel(unsigned pixel_latch) {
    // Select Pixel latch to copy into SR
    writeRegister(&Fei4::Pixel_latch_strobe, pixel_latch);

    // Enable Latches
    writeRegister(&Fei4::Latch_Enable, 0x1);

    // Copy from SR into Latches
    globalPulse(chipId, 10);

    // Reset SR regs
    writeRegister(&Fei4::Latch_Enable, 0x0);
    writeRegister(&Fei4::Pixel_latch_strobe, 0x0);
}

void Fei4::readPixelRegister(unsigned colpr_addr, unsigned latch) {
    // Select DC(s) for operations
    writeRegister(&Fei4::Colpr_Mode, 0x0);
    writeRegister(&Fei4::Colpr_Addr, colpr_addr);
    // Select SR in Parallel Input Mode
    writeRegister(&Fei4::S1, 0x1);
    writeRegister(&Fei4::S0, 0x1);
    writeRegister(&Fei4::HitLD, 0x0);
    // Select Pixel latch to copy into SR
    writeRegister(&Fei4::Pixel_latch_strobe, latch);

    writeRegister(&Fei4::SR_Clock, 0x1);

    // Copy from Latches into SR
    globalPulse(chipId, 10);

    // Reset SR regs
    writeRegister(&Fei4::S1, 0x0);
    writeRegister(&Fei4::S0, 0x0);
    writeRegister(&Fei4::HitLD, 0x0);
    writeRegister(&Fei4::Pixel_latch_strobe, 0x0);

    // Read back SR values
    writeRegister(&Fei4::SR_Clock, 0x0);
    writeRegister(&Fei4::SRRead, 0x1);
    uint32_t bitstream[21] = {0};
    wrFrontEnd(chipId, bitstream);

    writeRegister(&Fei4::SRRead, 0x0);
}

void Fei4::dummyCmd() {
    writeRegister(&Fei4::Colpr_Mode, 0x0);
    writeRegister(&Fei4::Colpr_Addr, Fei4PixelCfg::n_DC-1);
    uint32_t bitstream[21] = {0};
    wrFrontEnd(chipId, bitstream);
    while(core->isCmdEmpty() == 0);
}

void Fei4::shiftByOne() {
    // Normal Shift Mode
    writeRegister(&Fei4::S1, 0x0);
    writeRegister(&Fei4::S0, 0x0);
    writeRegister(&Fei4::SR_Clock, 0x1);

    // Shift By One Clock Cycle
    globalPulse(chipId, 10);

    writeRegister(&Fei4::SR_Clock, 0x0);
}



void Fei4::wrGR16(unsigned int mOffset, unsigned int bOffset, unsigned int mask, bool msbRight, uint16_t cfgBits) {
    //First, bring local representation of GR to new state.
    //This is equivalent to what the template function "setValue" does
    //Fetches a (value of a) 16 bit bar, sets the bits that correspond to the current GR to 0,
    //reverses the bit-order of the new value for the GR if necessary, cuts digits that are incompatible
    //with the size of the current GR, shifts it to the right position within the 16 bit bar,
    //then ORs both values
    unsigned maskBits = (1 << mask) - 1;
    cfg[mOffset]=(cfg[mOffset]&(~(maskBits<<bOffset))) |
        (((msbRight?BitOps::reverse_bits(cfgBits, mask):cfgBits)&maskBits)<<bOffset);
    //Now actually write the new value to the FE-I4 Global Register
    wrRegister(chipId, mOffset, cfg[mOffset]);

    return;
}

void Fei4::writeNamedRegister(std::string name, uint16_t reg_value) {
    std::cout << __PRETTY_FUNCTION__ << " : " << name << " -> " << reg_value << std::endl;
    writeRegister(regMap[name], reg_value);
}
