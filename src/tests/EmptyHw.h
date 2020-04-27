#ifndef YARR_TEST_EMPTY_HARDWARE_H
#define YARR_TEST_EMPTY_HARDWARE_H

#include "HwController.h"

class EmptyTxCore : public virtual TxCore {
public:
  EmptyTxCore() {}
  ~EmptyTxCore() {}

  void writeFifo(uint32_t) override {}
  void releaseFifo() override {}
  void setCmdEnable(uint32_t) override {}
  void setCmdEnable(std::vector<uint32_t>) override {}
  void disableCmd() override {}
  uint32_t getCmdEnable() override { return 0; }
  bool isCmdEmpty() override { return true; }

  void setTrigEnable(uint32_t value) override {}
  uint32_t getTrigEnable() override { return 0; }
  void maskTrigEnable(uint32_t value, uint32_t mask) override {}
  bool isTrigDone() override { return true; }

  void setTrigConfig(enum TRIG_CONF_VALUE cfg) override {}
  void setTrigFreq(double freq) override {}
  void setTrigCnt(uint32_t count) override {}
  void setTrigTime(double time) override {}
  void setTrigWordLength(uint32_t length) override {}
  void setTrigWord(uint32_t *word, uint32_t length) override {}
  void toggleTrigAbort() override {}

  void setTriggerLogicMask(uint32_t mask) override {}
  void setTriggerLogicMode(enum TRIG_LOGIC_MODE_VALUE mode) override {}
  void resetTriggerLogic() override {}
  uint32_t getTrigInCount() override { return 0; }
};

class EmptyRxCore : public virtual RxCore {
 public:
  EmptyRxCore() {}
  ~EmptyRxCore() {}

  void setRxEnable(uint32_t val) override {}
  void setRxEnable(std::vector<uint32_t>) override {}
  void maskRxEnable(uint32_t val, uint32_t mask) override {}
  void disableRx() override {}

  RawData* readData() override { return nullptr; }
  void flushBuffer() override {}

  uint32_t getDataRate() override { return 40; }
  uint32_t getCurCount() override { return 0; }
  bool isBridgeEmpty() override { return true; }
};

class EmptyHw : public HwController, public EmptyRxCore, public EmptyTxCore {
 public:
  void loadConfig(json &j) override {}
};

#endif
