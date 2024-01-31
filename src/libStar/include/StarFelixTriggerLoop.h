#ifndef STARFELIXTRIGGERLOOP_H
#define STARFELIXTRIGGERLOOP_H

#include <array>
#include "LoopActionBase.h"
#include "StdTriggerAction.h"

class StarFelixTriggerLoop: public LoopActionBase, public StdTriggerAction {

public:

  StarFelixTriggerLoop();

  void setTrigDelay(uint32_t delay) {m_trigDelay = delay;}
  uint32_t getTrigDelay() const {return m_trigDelay;}

  void setTrigFreq(double freq) {m_trigFreq = freq;}
  double getTrigFreq() const {return m_trigFreq;}

  void setTrigTime(double time){m_trigTime = time;}
  double getTrigTime() const{return m_trigTime;}

  void setDigital(bool digital){m_digital = digital;}
  bool getDigital() const{return m_digital;}

  void setTrigWord();

  void writeConfig(json &config) override;
  void loadConfig(const json &config) override;

private:

  uint32_t m_trigDelay {45};
  double m_trigFreq {1e6};
  double m_trigTime {10};

  double m_trickleFreq {10}; // frequency to send trickle pulse

  // How many words of pattern buffer to use
  uint32_t m_trigWordLength;
  // This matches the pattern buffer in TxCore
  std::array<uint32_t, 32> m_trigWord;

  bool m_noInject {true};
  bool m_digital {false};
  bool m_useHitCount {true};

  unsigned m_nTrigsTrickle; // number of triggers stored in the trickle memory

  std::tuple<std::vector<uint8_t>, unsigned> getTriggerSegment(unsigned max_trigs = -1);
  void addChargeInjection(std::vector<uint8_t>&);
  std::vector<uint8_t> getHitCounterSegment();
  std::vector<uint8_t> makeTrickleSequence();

  void init() override;
  void end() override;
  void execPart1() override;
  void execPart2() override;
};

#endif
