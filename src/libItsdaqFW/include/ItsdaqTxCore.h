#ifndef ITSDAQ_TXCORE_H
#define ITSDAQ_TXCORE_H

#include "TxCore.h"

#include <atomic>
#include <cstdint>
#include <thread>
#include <vector>

#include "storage.hpp"

class ItsdaqHandler;

/**
 * Implementation of the RxCore for communication with ITSDAQ FW.
 **/
class ItsdaqTxCore : virtual public TxCore {
 public:
  ItsdaqTxCore(ItsdaqHandler &h);
  ~ItsdaqTxCore();

  void writeFifo(uint32_t value) override;
  void releaseFifo() override;

  void setCmdEnable(uint32_t) override;
  void setCmdEnable(std::vector<uint32_t> channels) override;
  void disableCmd() override;
  uint32_t getCmdEnable() override;
  bool isCmdEmpty() override;
  void setTrigEnable(uint32_t value) override;
  uint32_t getTrigEnable() override;

  void maskTrigEnable(uint32_t value, uint32_t mask) override;
  bool isTrigDone() override;
  void setTrigConfig(enum TRIG_CONF_VALUE cfg) override;
  void setTrigFreq(double freq) override;
  void setTrigCnt(uint32_t count) override;
  void setTrigTime(double time) override;
  void setTrigWordLength(uint32_t length) override;
  void setTrigWord(uint32_t *words, uint32_t size) override;
  void toggleTrigAbort() override;
  void setTriggerLogicMask(uint32_t mask) override;
  void setTriggerLogicMode(enum TRIG_LOGIC_MODE_VALUE mode) override;
  void resetTriggerLogic() override;
  uint32_t getTrigInCount() override;
  void fromFileJson(json& j);
  void toFileJson(json& j);

private:
  // Run n triggers in the background
  void doTriggerCnt();

  // Run triggers for some time
  void doTriggerTime();

  // Send triggerFifo
  void trigger();

  // Extend buffer with appropriate bits for sending sequence of LCB/L1R3
  void buildSequenceWord(std::vector<uint16_t> &buffer,
                         uint32_t LCB, uint32_t L1R3);

  void buildTriggerSequence();

  std::thread m_trigProc;                    //! trigger processor

  enum TRIG_CONF_VALUE m_trigCfg;            //! trigger config

  std::atomic<bool> m_trigEnabled;           //! trigger is enabled */
  uint32_t m_trigMode;                       //! trigger mode
  uint32_t m_trigCnt;                        //! number of triggers
  uint32_t m_trigTime;                       //! trigger time
  uint32_t m_trigFreq;                       //! trigger frequency
  uint32_t m_trigWordLength;                 //! number of trigger words

  std::vector<uint32_t> m_trigWords;         //! the trigger words

  std::vector<uint16_t> m_buffer;
  std::vector<uint16_t> m_trigBuffer;

  ItsdaqHandler &m_h;
};

#endif
