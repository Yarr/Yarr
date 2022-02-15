#include "FelixRxCore.h"

#include "logging.h"

namespace {
  auto frlog = logging::make_log("FelixRxCore");
}

// WIP

FelixRxCore::FelixRxCore()
{

}

FelixRxCore::~FelixRxCore()
{

}

void FelixRxCore::setRxEnable(uint32_t val) {}
void FelixRxCore::setRxEnable(std::vector<uint32_t> channels) {}
void FelixRxCore::disableRx() {}
void FelixRxCore::maskRxEnable(uint32_t val, uint32_t mask) {}

void FelixRxCore::flushBuffer() {}
RawData* FelixRxCore::readData() {return nullptr;}

uint32_t FelixRxCore::getDataRate() {return 0;}
uint32_t FelixRxCore::getCurCount() {return 0;}
bool FelixRxCore::isBridgeEmpty() {return false;}

void FelixRxCore::writeConfig(json &j) {}
void FelixRxCore::loadConfig(const json &j) {}
