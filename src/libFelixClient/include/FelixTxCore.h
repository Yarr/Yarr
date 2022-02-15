#ifndef FELIXTXCORE_H
#define FELIXTXCORE_H

#include "TxCore.h"

#include "felix/felix_client_thread.hpp"
#include "storage.hpp"

class FelixTxCore : virtual public TxCore {

public:

  FelixTxCore();
  ~FelixTxCore() override;

  void writeFifo(uint32_t value) override; 	// append to fifo of all channels
  void releaseFifo() override; 		// release the fifo for all enabled channels

  void setCmdEnable(uint32_t) override;
  void setCmdEnable(std::vector<uint32_t> channels) override;
  void disableCmd() override;
  uint32_t getCmdEnable() override;
  bool isCmdEmpty() override; 		// check if the fifo of commands is empty
  void setTrigEnable(uint32_t value) override; 	// enable the trigger
  uint32_t getTrigEnable() override; 	// get if the trigger is enabled

  void maskTrigEnable(uint32_t value, uint32_t mask) override;
  bool isTrigDone() override; 		// check if the trigger is done
  void setTrigConfig(enum TRIG_CONF_VALUE cfg) override; 	// set the trigger config
  void setTrigFreq(double freq) override; 	// set the trigger frequency
  void setTrigCnt(uint32_t count) override; 	// set the number of desired triggers
  void setTrigTime(double time) override; 	// set the trigger time in seconds
  void setTrigWordLength(uint32_t length) override; 	// set Trigger Word Length
  void setTrigWord(uint32_t *words, uint32_t size) override; 	// set the trigger words
  void toggleTrigAbort() override; 	// abort the trigger sequence
  void setTriggerLogicMask(uint32_t mask) override; 	// set the trigger logic
  void setTriggerLogicMode(enum TRIG_LOGIC_MODE_VALUE mode) override; 	// set the trigger logic mode
  void resetTriggerLogic() override; 	// reset the trigger logic
  uint32_t getTrigInCount() override; 	// get the number of triggers in

  void loadConfig(const json &j); 		// read configuration from json
  void writeConfig(json& j); 		// write configuration to json

protected:

private:

  // Owned by FelixController
  FelixClientThread* fclient;
};

#endif
