// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: ITkPixV2 Library
// # Comment: Collection of ITkPixV2 commands
// # Date: Jul 2023
// ################################

#include "Itkpixv2Cmd.h"
#include "logging.h"

namespace {
  auto logger = logging::make_log("Itkpixv2Cmd");
}

Itkpixv2Cmd::Itkpixv2Cmd() : core( nullptr ) {}

Itkpixv2Cmd::Itkpixv2Cmd(TxCore *arg_core) : core(arg_core) {}

Itkpixv2Cmd::~Itkpixv2Cmd() = default;

constexpr uint16_t Itkpixv2Cmd::enc5to8[];
constexpr uint16_t Itkpixv2Cmd::encTrigger[];
constexpr uint16_t Itkpixv2Cmd::encTag[];

std::array<uint16_t, 1> Itkpixv2Cmd::genPllLock() {
    return {0xAAAA};
}

std::array<uint16_t, 1> Itkpixv2Cmd::genSync() {
    return {0x817E};
}

std::array<uint16_t, 1> Itkpixv2Cmd::genTrigger(uint8_t bc, uint8_t tag) {
    return {static_cast<uint16_t>((encTrigger[bc]<<8) | (bc > 0 ? encTag[tag] : 0xAA))};
}

std::array<uint16_t, 2> Itkpixv2Cmd::genReadTrigger(uint8_t chipId, uint8_t  etag) {
    return {static_cast<uint16_t>(0x6900 | enc5to8[chipId&0x1F]),
        static_cast<uint16_t>((enc5to8[(etag&0xE0)>>5]<<8) | enc5to8[etag&0x1F])};
}

std::array<uint16_t, 1> Itkpixv2Cmd::genClear(uint8_t chipId) {
    return {static_cast<uint16_t>(0x5A00 | enc5to8[chipId&0x1F])};
}

std::array<uint16_t, 1> Itkpixv2Cmd::genGlobalPulse(uint8_t chipId) {
    return {static_cast<uint16_t>(0x5C00 | enc5to8[chipId&0x1F])};
}

std::array<uint16_t, 3> Itkpixv2Cmd::genCal(uint8_t chipId, uint8_t mode, uint8_t edgeDelay, uint8_t edgeDuration, uint8_t auxPar, uint8_t auxDelay) {
    return {static_cast<uint16_t>(0x6300 | enc5to8[chipId&0x1F]),
        static_cast<uint16_t>((enc5to8[((mode&0x1)<<4) | ((edgeDelay&0x1E)>>1)]<<8) | enc5to8[((edgeDelay&0x1)<<4) | ((edgeDuration&0xF0)>>4)]),
        static_cast<uint16_t>((enc5to8[((edgeDuration&0xF)<<1) | (auxPar&0x1)]<<8) | enc5to8[auxDelay&0x1F])};
}

std::array<uint16_t, 4> Itkpixv2Cmd::genWrReg(uint8_t chipId, uint16_t address, uint16_t data) {
    return {static_cast<uint16_t>(0x6600 | enc5to8[chipId&0x1F]),
        static_cast<uint16_t>((enc5to8[(address&0x1E0)>>5]<<8) | enc5to8[address&0x1F]),
        static_cast<uint16_t>((enc5to8[(data&0xF800)>>11]<<8) | enc5to8[(data&0x7C0)>>6]),
        static_cast<uint16_t>((enc5to8[(data&0x3E)>>1]<<8) | enc5to8[(data&0x1)<<4])};
}

std::array<uint16_t, 2> Itkpixv2Cmd::genRdReg(uint8_t chipId, uint16_t address) {
    return {static_cast<uint16_t>(0x6500 | enc5to8[chipId&0x1F]),
        static_cast<uint16_t>((enc5to8[(address&0x1E0)>>5]<<8) | enc5to8[address&0x1F])};
}

void Itkpixv2Cmd::sendPllLock() {
    SPDLOG_LOGGER_TRACE(logger, "Sending PllLock");
    core->writeFifo(0xAAAA0000 | Itkpixv2Cmd::genPllLock()[0]);
    core->releaseFifo();
}

void Itkpixv2Cmd::sendSync() {
    SPDLOG_LOGGER_TRACE(logger, "Sending Sync");
    core->writeFifo(0xAAAA0000 | Itkpixv2Cmd::genSync()[0]);
    core->releaseFifo();
}

void Itkpixv2Cmd::sendReadTrigger(uint8_t chipId, uint8_t etag) {
    SPDLOG_LOGGER_TRACE(logger, "Sending ReadTrigger(id({}), etag({}))", chipId, etag);
    std::array<uint16_t, 2> readTrig = Itkpixv2Cmd::genReadTrigger(chipId, etag);
    core->writeFifo((readTrig[0] << 16) | readTrig[1]);
    core->releaseFifo();
}

void Itkpixv2Cmd::sendClear(uint8_t chipId) {
    SPDLOG_LOGGER_TRACE(logger, "Sending Clear(id({}))", chipId);
    core->writeFifo(0xAAAA0000 | Itkpixv2Cmd::genClear(chipId)[0]);
    core->releaseFifo();
}

void Itkpixv2Cmd::sendGlobalPulse(uint8_t chipId) {
    SPDLOG_LOGGER_TRACE(logger, "Sending GlobalPulse(id({}))", chipId);
    core->writeFifo(0xAAAA0000 | Itkpixv2Cmd::genGlobalPulse(chipId)[0]);
    core->releaseFifo();
}

void Itkpixv2Cmd::sendWrReg(uint8_t chipId, uint16_t address, uint16_t data) {
    logger->debug("Sending WrReg(id({}),addr({}),data({}))", chipId, address, data);
    std::array<uint16_t, 4> wrReg = Itkpixv2Cmd::genWrReg(chipId, address, data);
    core->writeFifo(((uint32_t)wrReg[0] << 16) | wrReg[1]);
    core->writeFifo(((uint32_t)wrReg[2] << 16) | wrReg[3]);
    core->writeFifo(0x817eAAAA);
    core->releaseFifo();
}

uint16_t Itkpixv2Cmd::conv10Bit(uint16_t value) {
    return uint16_t(enc5to8[(value>>5)&0x1F]<<8 | enc5to8[value&0x1F]);
}

void Itkpixv2Cmd::sendPixRegBlock(uint8_t chipId, std::array<uint16_t, 384> &data) {
    logger->debug("Sending PixRegBlock(id({}))", chipId);
    std::array<uint16_t, 4> wrReg = Itkpixv2Cmd::genWrReg(chipId, 0, 0);
    core->writeFifo(((uint32_t)wrReg[0] << 16) | this->conv10Bit(0x0200));
    for (unsigned i=0; i<data.size(); i+=2) {
        core->writeFifo(this->conv10Bit(data[i]) << 16 | this->conv10Bit(data[i+1]));
    }
    core->writeFifo(0x817eAAAA);
    core->releaseFifo();
}

void Itkpixv2Cmd::sendRdReg(uint8_t chipId, uint16_t address) {
    logger->debug("Sending RdReg(id({}),addr({}))", chipId, address);
    std::array<uint16_t, 2> rdReg = Itkpixv2Cmd::genRdReg(chipId, address);
    core->writeFifo(((uint32_t)rdReg[0] << 16) | rdReg[1]);
    core->writeFifo(0x817eAAAA);
    core->releaseFifo();
}
