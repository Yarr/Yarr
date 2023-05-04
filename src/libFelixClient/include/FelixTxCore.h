#ifndef FELIXTXCORE_H
#define FELIXTXCORE_H

#include "TxCore.h"
#include "FelixTools.h"

#include "felix/felix_client_thread.hpp"
#include "storage.hpp"

#include <cstdint>
#include <vector>
#include <thread>
#include <atomic>

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

  bool readFelixRegister(const std::string&, uint64_t&);
  bool writeFelixRegister(const std::string&, const std::string&);

protected:

  void loadConfig(const json &j); 		     // read configuration from json
  void writeConfig(json& j); 		         // write configuration to json
  void setClient(std::shared_ptr<FelixClientThread> client); // set Felix client

  using FelixID_t = FelixTools::FelixID_t;

  // Channel control
  void enableChannel(FelixID_t fid);
  void disableChannel(FelixID_t fid);
  bool checkChannel(FelixID_t fid);

  void fillFifo(std::vector<uint8_t>& fifo, uint32_t value);
  void prepareFifo(std::vector<uint8_t>& fifo);

  // Triggers
  void trigger();
  void doTriggerCnt(); // send a defined number of triggers
  void doTriggerTime(); // send triggers for a period of time
  void prepareTrigger();

  // FELIX register access
  FelixClientThread::Reply accessFelixRegister(FelixClientThread::Cmd, const std::vector<std::string>&);
  bool checkReply(const FelixClientThread::Reply&);

  std::map<FelixID_t, bool> m_enables; // enable flag for each elink
  std::map<FelixID_t, std::vector<uint8_t> > m_fifo;     // data buffer
  std::map<FelixID_t, std::vector<uint8_t> > m_trigFifo; // buffers for trigger

  std::thread m_trigProc;                  // trigger thread
  enum TRIG_CONF_VALUE m_trigCfg;          // trigger config
  std::vector<uint32_t> m_trigWords;       // the trigger words
  std::atomic<bool> m_trigEnabled {false}; // trigger is enabled
  uint32_t m_trigCnt {0};                  // number of triggers
  uint32_t m_trigTime {0};                 // trigger time
  uint32_t m_trigFreq {1};                 // trigger frequency
  uint32_t m_trigWordLength {4};           // number of trigger words

  bool m_flip {false};

  // For Felix ID
  FelixID_t fid_from_channel(uint32_t chn);
  uint8_t m_did {0};  // detector ID; 0x00 reserved for local IDs
  uint16_t m_cid {0}; // connector ID; 0x0000 reserved for local IDs
  uint8_t m_protocol {0}; // protocol ID

  std::shared_ptr<FelixClientThread> fclient;
};

#endif
