#include "FelixTxCore.h"

#include "logging.h"

namespace {
  auto ftlog = logging::make_log("FelixTxCore");
}

// WIP

FelixTxCore::FelixTxCore()
{

}

FelixTxCore::~FelixTxCore()
{

}

void FelixTxCore::writeFifo(uint32_t value) {}
void FelixTxCore::releaseFifo() {}
void FelixTxCore::setCmdEnable(uint32_t) {}
void FelixTxCore::setCmdEnable(std::vector<uint32_t> channels) {}
void FelixTxCore::disableCmd() {}
uint32_t FelixTxCore::getCmdEnable() {return 0;}
bool FelixTxCore::isCmdEmpty() {return false;}
void FelixTxCore::setTrigEnable(uint32_t value) {}
uint32_t FelixTxCore::getTrigEnable() {return 0;}

void FelixTxCore::maskTrigEnable(uint32_t value, uint32_t mask) {}
bool FelixTxCore::isTrigDone() {return false;}
void FelixTxCore::setTrigConfig(enum TRIG_CONF_VALUE cfg) {}
void FelixTxCore::setTrigFreq(double freq) {}
void FelixTxCore::setTrigCnt(uint32_t count) {}
void FelixTxCore::setTrigTime(double time) {}
void FelixTxCore::setTrigWordLength(uint32_t length) {}
void FelixTxCore::setTrigWord(uint32_t *words, uint32_t size) {}
void FelixTxCore::toggleTrigAbort() {}
void FelixTxCore::setTriggerLogicMask(uint32_t mask) {}
void FelixTxCore::setTriggerLogicMode(enum TRIG_LOGIC_MODE_VALUE mode) {}
void FelixTxCore::resetTriggerLogic() {}
uint32_t FelixTxCore::getTrigInCount() {return 0;}

void FelixTxCore::loadConfig(const json &j) {}
void FelixTxCore::writeConfig(json& j) {}
