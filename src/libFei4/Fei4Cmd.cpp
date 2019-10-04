// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: FE-I4 Library
// # Comment: Collection of FE-I4 commands
// ################################

#include "Fei4Cmd.h"

#include "logging.h"

namespace {
auto flog = logging::make_log("fei4_cmd");
}

Fei4Cmd::Fei4Cmd() {
    core = NULL;
}

void Fei4Cmd::setCore(TxCore *arg_core) {
    core = arg_core;
}

Fei4Cmd::Fei4Cmd(TxCore *arg_core) {
    core = arg_core;
}

Fei4Cmd::~Fei4Cmd() {

}

void Fei4Cmd::trigger() {
    flog->trace(__PRETTY_FUNCTION__);
    core->writeFifo(0x1D00);
    core->releaseFifo();
}

void Fei4Cmd::bcr() {
    flog->trace(__PRETTY_FUNCTION__);
    core->writeFifo(0x1610);
    core->releaseFifo();
}

void Fei4Cmd::ecr() {
    flog->trace(__PRETTY_FUNCTION__);
    core->writeFifo(0x1620);
    core->releaseFifo();
}

void Fei4Cmd::cal() {
    flog->trace(__PRETTY_FUNCTION__);
    core->writeFifo(0x1640);
    core->releaseFifo();
}

void Fei4Cmd::wrRegister(int chipId, int address, int value) {
    flog->trace("{}: Addr {}, 0x{:x}", __PRETTY_FUNCTION__, address, value);
    core->writeFifo(0x005A0800+((chipId<<6)&0x3C0)+(address&0x3F));
    core->writeFifo((value<<16)&0xFFFF0000);
    core->releaseFifo();
}

void Fei4Cmd::rdRegister(int chipId, int address) {
    flog->trace(__PRETTY_FUNCTION__);
    core->writeFifo(0x005A0400+((chipId<<6)&0x3C0)+(address&0x3F));
    core->releaseFifo();
}

void Fei4Cmd::wrFrontEnd(int chipId, uint32_t *bitstream) {
    flog->trace(__PRETTY_FUNCTION__);
    core->writeFifo(0x005A1000+((chipId<<6)&0x3C0));
    //Flipping the order in order to send bit 671-0, and not bit 31-0, 63-21, etc.
    for(int i = 20 ; i>=0 ; i--) {
        core->writeFifo(bitstream[i]);	
    }
    core->releaseFifo();
}

void Fei4Cmd::runMode(int chipId, bool mode) {
    flog->trace("{} chipId({}) mode({})", __PRETTY_FUNCTION__, chipId, mode);
    uint32_t modeBits = mode ? 0x38 : 0x7;
    core->writeFifo(0x005A2800+((chipId<<6)&0x3C0)+modeBits);
    core->releaseFifo();
}

void Fei4Cmd::globalReset(int chipId) {
    flog->trace(__PRETTY_FUNCTION__);
    core->writeFifo(0x005A2000+((chipId<<6)&0x3C0));
    core->releaseFifo();
}

void Fei4Cmd::globalPulse(int chipId, unsigned width) {
    flog->trace(__PRETTY_FUNCTION__);
    core->writeFifo(0x005A2400+((chipId<<6)&0x3C0)+(width&0x3F));
    core->releaseFifo();
}

void Fei4Cmd::calTrigger(int delay) {
    flog->trace(__PRETTY_FUNCTION__);
    core->writeFifo(0x00001640);
    for (int i = 0; i<delay/32; i++){
        core->writeFifo(0x00000000);
    }
    core->writeFifo(0x1D000000>>delay%32);
    core->releaseFifo();
}
